# -*- coding: utf-8 -*-
"""Parse 串口协议 xlsx and dump to txt."""
import os
import zipfile
import xml.etree.ElementTree as ET
import re

NS = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}


def col_row_from_ref(ref):
    m = re.match(r"([A-Z]+)(\d+)", ref.upper())
    if not m:
        return 0, 0
    col_s, row_s = m.group(1), m.group(2)
    col = 0
    for c in col_s:
        col = col * 26 + (ord(c) - ord("A") + 1)
    return col - 1, int(row_s) - 1


def load_shared_strings(z):
    out = []
    try:
        data = z.read("xl/sharedStrings.xml")
    except KeyError:
        return out
    root = ET.fromstring(data)
    for si in root.findall(".//m:si", NS):
        parts = []
        for t in si.iter():
            tag = t.tag.split("}")[-1]
            if tag == "t" and t.text is not None:
                parts.append(t.text)
        if not parts:
            for t in si.findall(".//m:t", NS):
                if t.text:
                    parts.append(t.text)
        out.append("".join(parts))
    return out


def parse_sheet(z, path, shared):
    try:
        data = z.read(path)
    except KeyError:
        return []
    root = ET.fromstring(data)
    rows_dict = {}
    for c in root.findall(".//m:c", NS):
        ref = c.get("r")
        if not ref:
            continue
        col, row = col_row_from_ref(ref)
        ct = c.get("t")
        v = c.find("m:v", NS)
        if v is None or v.text is None:
            val = ""
        else:
            val = v.text
            if ct == "s":
                try:
                    val = shared[int(val)]
                except (ValueError, IndexError):
                    pass
        if row not in rows_dict:
            rows_dict[row] = {}
        rows_dict[row][col] = val
    if not rows_dict:
        return []
    max_row = max(rows_dict.keys())
    max_col = 0
    for d in rows_dict.values():
        if d:
            max_col = max(max_col, max(d.keys()))
    out = []
    for r in range(max_row + 1):
        line = []
        for c in range(max_col + 1):
            line.append(rows_dict.get(r, {}).get(c, ""))
        if any(str(x).strip() for x in line):
            out.append(line)
    return out


def main():
    base = os.path.dirname(os.path.abspath(__file__))
    # 优先 .xlsx，否则旧名 .csv（实为 xlsx）
    candidates = [
        os.path.join(base, "离线语言_协议", "串口协议需求_V0_20260312 .xlsx"),
        os.path.join(base, "离线语言_协议", "串口协议需求_V0_20260312 .csv"),
    ]
    p = None
    for c in candidates:
        if os.path.isfile(c):
            p = c
            break
    if not p:
        print("未找到协议文件，请将 xlsx 放在: 离线语言_协议/")
        return
    print("使用文件:", p)
    try:
        z = zipfile.ZipFile(p, "r")
    except zipfile.BadZipFile as e:
        print("ZIP 无法打开（文件损坏或不完整）:", e)
        return
    shared = load_shared_strings(z)
    print("sharedStrings:", len(shared))
    out_path = os.path.join(os.path.dirname(p), "串口协议需求_V0_20260312_extracted.txt")
    n = 0
    with open(out_path, "w", encoding="utf-8") as fo:
        for name in sorted(z.namelist()):
            if name.startswith("xl/worksheets/sheet") and name.endswith(".xml"):
                rows = parse_sheet(z, name, shared)
                if not rows:
                    continue
                fo.write(f"\n===== {name} ({len(rows)} rows) =====\n")
                for i, row in enumerate(rows):
                    line = " | ".join(str(x).replace("\n", " ")[:200] for x in row)
                    fo.write(f"{i+1}\t{line}\n")
                n += len(rows)
                print(name, "rows:", len(rows))
    z.close()
    print("已写入:", out_path, "总行约:", n)


if __name__ == "__main__":
    main()
