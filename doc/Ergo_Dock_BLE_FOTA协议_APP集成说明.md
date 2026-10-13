# Ergo_Dock BLE FOTA 协议（APP 集成说明）

> 适用固件：`AIP_v1_1/ble/ble_fota_server`（`fota_svc_server.c`）  
> 协议来源：凌思微（LinkedSemi）LE501X FOTA  
> 与业务协议关系：**独立通道**。业务走端口 `0x01/0x06`（见 `Ergo_Dock_BLE通信协议_2.0.md`）；OTA 走 GATT 服务 `0x2600`，**不要**混进 Ergo 帧格式。

小程序能升级，说明设备侧已按本协议工作。自有 APP 只需实现**同一套客户端流程**即可。

---

## 一、集成思路（给产品 / APP）

| 步骤 | 内容 |
|------|------|
| 1 | APP 扫描并连接设备（广播名含 `Ergo_dock:`） |
| 2 | 发现 GATT 服务 **`0x2600`**，拿到控制特征 `0x7000`、数据特征 `0x7001` |
| 3 | 开启 `0x7000` 的 **Indication（CCCD）** |
| 4 | 协商较大 MTU（建议 ≥ 200，固件最大支持 519） |
| 5 | 按下文流程：Digest/Signature → Start → 按扇区传数据 → Integrity → Finish |
| 6 | 设备复位后重新连接，读软件版本确认升级成功 |

**不需要改固件**，也不需要在 Ergo 业务协议里增加「OTA 指令」。

参考实现：

- 原厂手机 APP 源码：<https://github.com/linkedsemi/BLE_FOTA_APP>
- 原厂 OTA 说明：<https://ls-doc.readthedocs.io/zh_CN/latest/src/app-note/ota/le5010-ota.html>

---

## 二、GATT 服务定义

### 2.1 UUID（16-bit，Little-Endian 广播/发现时注意端序）

| 角色 | UUID | 属性 | 说明 |
|------|------|------|------|
| Service | `0x2600` | — | FOTA 服务 |
| Characteristic CTRL | `0x7000` | Write Request + Indicate | 控制命令 / 设备应答 |
| CCCD | `0x2902` | Write | 必须写 `0x0002` 打开 Indication |
| Characteristic DATA | `0x7001` | Write Command（无应答）+ Read | 固件数据分包；读回 ACK 位图 |

固件声明（摘要）：

```c
// Service  0x2600
// CTRL     0x7000  : Write Request + Indication
// DATA     0x7001  : Write Command + Read
```

### 2.2 常量

| 宏 | 值 | 含义 |
|----|-----|------|
| `USER_MAX_MTU` | 519 | 固件支持的最大 MTU |
| ATT 单包有效载荷 | MTU − 3 | 实际 DATA 分包上限 |
| `FOTAS_SECTOR_SIZE` | 4096 | 每扇区字节数 |
| `FOTAS_PAGE_SIZE` | 256 | Flash 页大小 |
| `FOTAS_SIGNATURE_CMD_DATA_LENGTH` | 16 | 签名每片长度 |
| `FOTAS_DIGEST_CMD_DATA_LENGTH` | 16 | Digest 每片长度 |
| `FOTAS_ACK_BUF_LENGTH_MAX` | 27 | 读 DATA 返回的 ACK 位图长度 |

---

## 三、控制特征（0x7000）报文格式

所有控制包均为：

```text
[type 1B] [payload ...]
```

`type` 枚举（`enum fotas_ctrl_type`）：

| type | 名称 | 方向 | 说明 |
|------|------|------|------|
| 0 | `FOTAS_SIGNATURE_CMD` | APP → 设备 | 下发 ECC 签名片段 |
| 1 | `FOTAS_DIGEST_CMD` | APP → 设备 | 下发 SHA256 digest 片段 |
| 2 | `FOTAS_START_REQ` | APP → 设备 | 请求开始升级 |
| 3 | `FOTAS_START_RSP` | 设备 → APP（Indicate） | 接受/拒绝 |
| 4 | `FOTAS_NEW_SECTOR_CMD` | APP → 设备 | 进入新扇区 |
| 5 | `FOTAS_INTEGRITY_CHECK_REQ` | APP → 设备 | 请求完整性校验 |
| 6 | `FOTAS_INTEGRITY_CHECK_RSP` | 设备 → APP（Indicate） | 校验结果 |
| 7 | `FOTAS_FINISH_CMD` | APP → 设备 | 结束并重启 |

多字节字段均为 **小端（LE）**。

### 3.1 SIGNATURE_CMD（type=0）

```text
[0x00] [idx 1B] [data 16B]
```

- 签名总长 64B，分 4 片：`idx = 0..3`
- 当前工程 `FW_ECC_VERIFY` 关闭时，设备侧校验恒通过；仍建议按标准流程发送（与小程序一致）

### 3.2 DIGEST_CMD（type=1）

```text
[0x01] [idx 1B] [data 16B]
```

- SHA256 digest 共 32B，分 2 片：`idx = 0..1`
- Digest = 对 **`.ota.bin` 全文件** 做 SHA256

### 3.3 START_REQ（type=2）

```text
[0x02]
[new_image_base  4B LE]
[new_image_size  4B LE]
[segment_data_max_length 2B LE]
```

| 字段 | 说明 |
|------|------|
| `new_image_base` | 新固件写入 Flash 绝对地址（须与工程/小程序一致） |
| `new_image_size` | `.ota.bin` 字节数 |
| `segment_data_max_length` | 每个 DATA 段的数据长度（不含 `segment_id`），通常取 `min(MTU-3-1, 协商值)` |

设备收到后若空闲：擦除目标区域，并通过 Indicate 回 `START_RSP`。

### 3.4 START_RSP（type=3，Indicate）

```text
[0x03] [status 1B]
```

| status | 含义 |
|--------|------|
| `0` (`FOTA_REQ_ACCEPTED`) | 接受，可开始传数据 |
| `1` (`FOTA_REQ_REJECTED`) | 拒绝（如签名失败） |

若设备忙，Write Request 可能返回 Busy，APP 应重试或中止。

### 3.5 NEW_SECTOR_CMD（type=4）

```text
[0x04] [sector_idx 2B LE]
```

- `sector_idx`：从 0 开始的扇区序号
- 每进入新扇区，设备清空 ACK 位图
- 每扇区固定 **4096 字节**

### 3.6 INTEGRITY_CHECK_REQ / RSP（type=5 / 6）

请求：

```text
[0x05] [dummy 1B]   // 可填 0
```

应答（Indicate）：

```text
[0x06] [status 1B]
```

| status | 含义 |
|--------|------|
| `0x00` | Digest 校验通过 |
| `0x80` | 校验失败 |

### 3.7 FINISH_CMD（type=7）

```text
[0x07]
[status_and_size 4B LE]   // bit0~7: status；bit8~31: fw_copy_size
[fw_copy_src_addr 4B LE]
[fw_copy_dst_addr 4B LE]
[boot_addr 4B LE]
```

`status` 位域（低 8 位）：

| 位掩码 | 含义 |
|--------|------|
| `0x01` (`FOTA_STATUS_MASK`) | 升级成功标记 |
| `0x02` (`FOTA_REBOOT_MASK`) | 需要重启 |
| `0x04` (`FOTA_SETTINGS_ERASE_MASK`) | 擦除 OTA settings（按需） |

固件逻辑：当 `(status & 0x01) && (status & 0x02)` 时，写入 boot/拷贝信息并 `platform_reset`。

典型双分区场景（以小程序/原厂 APP 参数为准，**务必与现网一致**）：

| 参数 | 常见取值（需实测核对） |
|------|------------------------|
| `new_image_base` | 下载区地址（如 `0x18020000`） |
| `fw_copy_src_addr` | 同下载区 |
| `fw_copy_dst_addr` | App 运行区（如 `0x18034000`） |
| `fw_copy_size` | 镜像大小或拷贝长度 |
| `boot_addr` | 启动地址（可为 0 表示不改） |
| `status` | 至少 `0x03`（成功 + 重启） |

> **重要：** `base/size/拷贝地址` 必须与当前小程序使用的参数、以及 `after_build.bat` / Flash 布局一致。建议用抓包工具抓一次小程序 OTA 的 `START_REQ` / `FINISH_CMD`，把数值固化到自有 APP。

---

## 四、数据特征（0x7001）报文格式

### 4.1 写固件数据（Write Command）

```text
[segment_id 1B] [payload N B]
```

- `N` = `segment_data_max_length`（最后一段可不足）
- 每个扇区的段数：

```text
segments_per_sector = ceil(4096 / segment_data_max_length)
segment_id = 0 .. segments_per_sector-1
```

设备按地址写入：

```text
addr = new_image_base - FLASH_BASE
     + sector_idx * 4096
     + segment_id * segment_data_max_length
```

### 4.2 读 ACK（Read）

读 `0x7001` 返回最多 **27 字节**位图：

- bit `(segment_id)` 置 1 表示该段已写入
- APP 应在扇区结束后检查 ACK，缺段则重传

---

## 五、推荐状态机（APP 侧）

```text
连接成功
  → 发现 0x2600 / 0x7000 / 0x7001
  → 写 CCCD 打开 Indication
  → 协商 MTU
  → 计算 ota.bin 的 SHA256 → DIGEST_CMD (idx=0,1)
  → （可选）SIGNATURE_CMD (idx=0..3)
  → START_REQ  → 等待 START_RSP(status=0)
  → for sector = 0 .. sector_count-1:
        NEW_SECTOR_CMD(sector)
        for seg = 0 .. segs-1:
            WriteCommand DATA [seg|payload]
        Read DATA → 检查 ACK，失败则重发缺段
  → INTEGRITY_CHECK_REQ → 等待 RSP(status=0)
  → FINISH_CMD(status|reboot|...)
  → 等待断连 / 超时后重连校验版本
```

进度显示可用：

```text
progress ≈ (已完成扇区 * 4096 + 当前扇区已写字节) / new_image_size
```

---

## 六、升级文件准备

### 6.1 生成 `.ota.bin`

Keil 编译后脚本 `tools/le501x/after_build.bat` 会输出：

```text
UVBuild\<工程名>.ota.bin
```

测试目录示例：`Dock测试固件/*.ota.bin`。

### 6.2 Digest

对整个 `.ota.bin` 做 SHA256，得到 32 字节，分两包：

```text
idx=0 : digest[0..15]
idx=1 : digest[16..31]
```

### 6.3 签名（若开启 ECC）

仓库工具：

```bash
python tools/signing/signing.py <xxx.ota.bin> signing_key.pem
# 生成 signature.bin（64B）
```

当前 Ergo_Dock 固件 `FW_ECC_VERIFY` 关闭时，签名可按小程序习惯仍发送，或与小程序保持一致即可。

---

## 七、与业务 BLE 的边界

| 项目 | 业务协议 | FOTA |
|------|----------|------|
| 服务 | SRV0020（透传/通知） | `0x2600` |
| 帧格式 | `[帧长][端口][数据域]` | GATT 特征二进制 |
| 用途 | 键值 / 语音 / 打鼾 / 测试 | 仅固件升级 |
| 同时进行 | OTA 期间建议暂停业务写，避免抢带宽 | — |

APP 架构建议：

```text
BleManager
  ├─ ErgoProtocol   // x_ble_com 业务
  └─ FotaClient     // 本文协议（可参考 BLE_FOTA_APP）
```

---

## 八、自有 APP 协议章节建议写法（可直接放进产品文档）

可按如下目录写入贵司《APP 通信协议》：

```text
第 X 章  固件空中升级（BLE FOTA）
  X.1  概述（独立于业务协议）
  X.2  GATT 服务与特征 UUID
  X.3  控制命令字与载荷定义（type 0~7）
  X.4  数据通道与 ACK
  X.5  升级完整时序
  X.6  地址参数表（以量产小程序抓包为准）
  X.7  异常处理（拒绝、校验失败、断连续传策略）
  X.8  版本兼容说明
```

其中 **X.6 地址参数表** 必须由你们用小程序实抓一次填实，例如：

| 参数 | 量产值 | 备注 |
|------|--------|------|
| new_image_base | `0x????????` | 抓 START_REQ |
| segment_data_max_length | `0x????` | 抓 START_REQ |
| FINISH status | `0x??` | 抓 FINISH_CMD |
| fw_copy_src / dst / size / boot | … | 抓 FINISH_CMD |

---

## 九、调试建议

1. **先对齐小程序**：同一台设备、同一 `.ota.bin`，小程序成功则说明固件/地址无问题。  
2. **抓包对比**：nRF Connect / Ellisys / 手机 HCI，对比小程序与自研 APP 的 `START_REQ`、`NEW_SECTOR`、`FINISH`。  
3. **日志**：固件侧 `OTA_SERVER` 有 `ctrl type`、`segment`、`start_addr` 等 RTT/串口日志。  
4. **断连**：`DISCONNECTED` 时固件会 `fota_clean_state()`，需从头开始。  
5. **版本兼容**：凌思微 SDK v1.1 起 FOTA 协议与旧 APP 不兼容；客户端需与当前固件同代。

---

## 十、一句话结论

- **业务协议**写「端口 + 段类型」（`Ergo_Dock_BLE通信协议_2.0.md`）。  
- **FOTA 协议**写「GATT `0x2600` + CTRL/DATA 二进制状态机」（本文）。  
- 自有 APP 集成 = 复用凌思微 FOTA 客户端逻辑 + 填入与小程序相同的 `base/size/finish` 参数；**无需改床控业务帧**。

若需要，可在抓到小程序一帧 `START_REQ`/`FINISH_CMD` 后，把「量产地址参数表」补进本文第八章。
