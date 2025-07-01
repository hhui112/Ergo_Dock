#include "OTA_server.h"
#include "log.h"

#define FW_ECC_VERIFY  (0)  //=1,ÆôÓÃOTAÉý¼¶ÃÜÔ¿

#if UART_SERVER_WITH_OTA == 1
#if FW_ECC_VERIFY//
#include "uECC.h"
const uint8_t fotas_pub_key[64] = {0};
bool fw_signature_check(struct fw_digest *digest,struct fota_signature *signature)
{
    return uECC_verify(fotas_pub_key, digest->data, sizeof(digest->data), signature->data, uECC_secp256r1());
}
#else
bool fw_signature_check(struct fw_digest *digest,struct fota_signature *signature)
{
    return true;
}
#endif


void prf_fota_server_callback(enum fotas_evt_type type,union fotas_evt_u *evt)
{
    static uint32_t new_image_size_bytes = 0;
    static uint16_t segment_data_max_length = 0;
    switch(type)
    {
    case FOTAS_START_REQ_EVT:
    {
        enum fota_start_cfm_status status;
        if(fw_signature_check(evt->fotas_start_req.digest, evt->fotas_start_req.signature))
        {
            status = FOTA_REQ_ACCEPTED;
            new_image_size_bytes = evt->fotas_start_req.new_image->size;
            segment_data_max_length = evt->fotas_start_req.segment_data_max_length;
        }else
        {
            status = FOTA_REQ_REJECTED;
        }
        prf_fotas_start_confirm(status);
    }break;
    case FOTAS_PROGRESS_EVT:
    {
        uint32_t bytes_transfered = evt->fotas_progress.current_sector * 4096 + evt->fotas_progress.current_segment * segment_data_max_length;
        uint8_t percentage =  bytes_transfered*100/new_image_size_bytes >= 100 ? 100 : bytes_transfered*100/new_image_size_bytes;
        LOG_I("ota progress ---------> %d%%", percentage);
    }
    break;
    case FOTAS_FINISH_EVT:
        if(evt->fotas_finish.status & FOTA_STATUS_MASK && evt->fotas_finish.status & FOTA_REBOOT_MASK)
        {
            if(evt->fotas_finish.boot_addr)
            {
                ota_boot_addr_set(evt->fotas_finish.boot_addr);
            }
            if(evt->fotas_finish.status & FOTA_SETTINGS_ERASE_MASK)
            {
                ota_settings_erase_req_set();
            }
            if(evt->fotas_finish.copy.fw_copy_size)
            {
                ota_copy_info_set(&evt->fotas_finish.copy);
            }
            platform_reset(RESET_OTA_SUCCEED);
        }
    break;
    default:
        LS_ASSERT(0);
    break;
    }
}
#endif

#if UART_SERVER_WITH_OTA == 1
void prf_added_handler(struct profile_added_evt *evt)
{
    LOG_I("profile:%d, start handle:0x%x\n",evt->id,evt->start_hdl);
    switch(evt->id)
    {
    case PRF_FOTA_SERVER:
        prf_fota_server_callback_init(prf_fota_server_callback);
        create_adv_obj();
    break;
    default:

    break;
    }
}
#endif
