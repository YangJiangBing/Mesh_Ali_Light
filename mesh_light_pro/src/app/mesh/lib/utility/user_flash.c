/**
*********************************************************************************************************
*               Copyright(c) 20185, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     user_flash.c
* @brief    This is the entry of user code which the user flash function resides in.
* @details
* @author   chenjie
* @date     2018-08-22
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "board.h"
#include "flash_device.h"
#include "user_flash.h"
#include "flash_adv_cfg.h"
#include "user_flash_driver.h"
#include "trace.h"
#include "os_sched.h"
#include "patch_header_check.h"
#include "platform_types.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/
#define  BIT_STATUS_WIP  BIT(0)

/*============================================================================*
 *                              Local Variables
 *============================================================================*/
static uint8_t prev_bp_lv = 0xfe;

/*============================================================================*
 *                              Local Functions
 *============================================================================*/
DATA_RAM_FUNCTION static bool app_flash_wait_busy(void)
{
    uint8_t status = 0;
    uint32_t ctr = 0;
    bool ret;

    while (ctr++ <= 0x10000)
    {
        ret = flash_cmd_rx(0x05, 1, &status);
        spic_enable(DISABLE);
        DBG_DIRECT("ret is %x,status is %x", ret, status);
        if (!ret)
        {
            goto wait_busy_fail;
        }

        if (!(status & BIT_STATUS_WIP))
        {
            return true;
        }
        DBG_DIRECT("CNT is %x", ctr);
        os_delay(1);
    }

wait_busy_fail:
    return false;
}

DATA_RAM_FUNCTION static bool app_flash_cmd_tx(uint8_t cmd, uint8_t data_len, uint8_t *data_buf)
{
    bool retval = true;
    DBG_DIRECT("app_flash_cmd_tx");
    uint32_t ctrlr0 = SPIC->ctrlr0;
    uint32_t addr_len = SPIC->addr_length;

    spic_enable(DISABLE);
    spic_clr_multi_ch();
    spic_set_tx_mode();

    SPIC->addr_length = data_len;

    spic_set_dr(DATA_BYTE, cmd);

    while (data_len--)
    {
        spic_set_dr(DATA_BYTE, *data_buf++);
    }

    spic_enable(ENABLE);

    if (!spic_wait_busy())
    {
        retval = false;
    }

    spic_enable(DISABLE);
    DBG_DIRECT("spic_wait_busy ..%x", retval);

    if (retval == true && !app_flash_wait_busy())
    {
        retval = false;
    }
    DBG_DIRECT("app_flash_wait_busy ..%x", retval);

    //restore ctrl0 and addr_len register
    SPIC->ctrlr0 = ctrlr0;
    SPIC->addr_length = addr_len;

    return retval;
}

/*============================================================================*
*                              Gloabal Functions
*============================================================================*/
/**
*  @brief: unlock flash is need when erase or write flash.
*/
DATA_RAM_FUNCTION bool unlock_flash_all(void)
{
    prev_bp_lv = 0;
    DFU_PRINT_TRACE0("**********[Flash Set] Flash unlock ***********");
    if (FLASH_SUCCESS == flash_sw_protect_unlock_by_addr_locked((0x00800000), &prev_bp_lv))
    {
        DFU_PRINT_TRACE1("[Flash Set] Flash unlock address = 0x800000, prev_bp_lv = %d", prev_bp_lv);
        return true;
    }
    return false;
}

/**
*  @brief: lock flash after erase or write flash.
*/
DATA_RAM_FUNCTION void lock_flash(void)
{
    if (prev_bp_lv != 0xfe)
    {
        flash_set_block_protect_locked(prev_bp_lv);
    }
}

DATA_RAM_FUNCTION uint32_t flash_erase_sector(uint32_t addr)
{

    static uint8_t address[3];
    DFU_PRINT_INFO1("==> flash_erase_sector :%x \r\n", addr);
    address[0] = (addr >> 16) & 0xff;
    address[1] = (addr >> 8) & 0xff;
    address[2] = addr & 0xff;

    flash_lock(FLASH_LOCK_USER_MODE_ERASE);
    flash_write_enable();
    app_flash_cmd_tx(0x20, 3, address);
    flash_unlock(FLASH_LOCK_USER_MODE_ERASE);
    return 0;
}

uint32_t get_temp_ota_bank_size_by_img_id(T_IMG_ID image_id)
{
    uint32_t image_size = 0;

    bool enable_old_ota = !is_ota_support_bank_switch();
    if (enable_old_ota)
    {
#if (SUPPORT_OTA_APP_DATA_EXTENSION == 1)
        if (image_id == SecureBoot || image_id == RomPatch || image_id == AppPatch
            || image_id == AppData1 || image_id == AppData2 || image_id == AppData3
            || image_id == AppData4 || image_id == AppData5 || image_id == AppData6)
#else
        if (image_id == SecureBoot || image_id == RomPatch || image_id == AppPatch
            || image_id == AppData1 || image_id == AppData2)
#endif
        {
            image_size = flash_get_bank_size(FLASH_OTA_TMP);
        }
        //others will return 0
    }
    else
    {
        uint32_t ota_bank0_addr = flash_get_bank_addr(FLASH_OTA_BANK_0);
        uint32_t temp_bank_addr;
        if (ota_bank0_addr == get_active_ota_bank_addr())
        {
            temp_bank_addr = flash_get_bank_addr(FLASH_OTA_BANK_1);
        }
        else
        {
            temp_bank_addr = ota_bank0_addr;
        }

        if (image_id == OTA)
        {
            image_size = OTA_HEADER_SIZE;
        }
#if (SUPPORT_OTA_APP_DATA_EXTENSION == 1)
        else if (image_id == SecureBoot || image_id == RomPatch || image_id == AppPatch
                 || image_id == AppData1 || image_id == AppData2 || image_id == AppData3
                 || image_id == AppData4 || image_id == AppData5 || image_id == AppData6)
#else
        else if (image_id == SecureBoot || image_id == RomPatch || image_id == AppPatch
                 || image_id == AppData1 || image_id == AppData2)
#endif
        {
            // auth ota temp bank and get address
            // image_authencation will fail after secure boot, so remove it
            if (!check_header_valid(temp_bank_addr, OTA))
            {
                image_size = 0;
            }
            else
            {
                image_size = HAL_READ32((uint32_t) & ((T_OTA_HEADER_FORMAT *)temp_bank_addr)->secure_boot_size,
                                        (image_id - SecureBoot) * 8);

                //attention: if use old ota header generate tool, app data3-6 addr will be default value 0xffffffff
                if (OTA_HEADER_DEFAULT_VALUE == image_size)
                {
                    image_size = 0;
                }
            }
        }
        else //others will return 0
        {
        }
    }

    return image_size;
}

bool check_dfu_update_image_length(uint16_t signature, uint32_t offset, uint32_t length,
                                   void *p_void, uint32_t *ret)
{
    uint32_t temp_bank_size = 0;
    *ret = 0;

    if (p_void == NULL)
    {
        *ret = __LINE__;
        return false;
    }

    //temp_bank_size = flash_ioctl(flash_ioctl_get_temp_bank_size_by_image_id, signature, 0); //if patch support
    temp_bank_size = get_temp_ota_bank_size_by_img_id((T_IMG_ID)signature);

    if (offset == 0)
    {
        T_IMG_CTRL_HEADER_FORMAT *p_header = (T_IMG_CTRL_HEADER_FORMAT *) p_void;
        uint32_t total_length = p_header->payload_len + 1024;

        if (total_length > temp_bank_size)
        {
            DFU_PRINT_ERROR2("New Image too large! total_length = %d, temp_bank_size = %d", total_length,
                             temp_bank_size);
            *ret = __LINE__;
            return false;
        }
    }

    if (offset + length > temp_bank_size)
    {
        DFU_PRINT_ERROR3("New Image single packet too large! offset = %d, length = %d, temp_bank_size = %d",
                         offset, length, temp_bank_size);
        *ret = __LINE__;
        return false;
    }

    //check pass
    return true;
}

/**
*  @brief: write specific image data feceived from host into flash.
*/
DATA_RAM_FUNCTION uint32_t sil_dfu_update(uint16_t signature, uint32_t offset, uint32_t length,
                                          uint32_t/*void*/ *p_void)
{
    uint32_t result = 0;
    uint32_t dfu_base_addr;
    uint32_t start_addr;
    uint32_t s_val;

    DFU_PRINT_INFO1("==> dfu_update length:%d \r\n", length);
    /*ASSERT((length % 4) == 0);*/
    /*ASSERT(p_void);*/

    if (length % 4)
    {
        result = __LINE__;
        goto L_Return;
    }

    if (p_void == 0)
    {
        result = __LINE__;
        goto L_Return;
    }
    /*get back up area address*/
    dfu_base_addr = get_temp_ota_bank_addr_by_img_id((T_IMG_ID)signature);
    if (dfu_base_addr == 0)
    {
        result = __LINE__;
        goto L_Return;
    }

    //before erase temp image or write image to flash temp, check access length depend flash layout
    if (!check_dfu_update_image_length(signature, offset, length, p_void, &result))
    {
        goto L_Return;
    }

    /*if it's start_packet*/
    if (offset == 0)
    {
        /*ASSERT(length>=sizeof(image_header_t));*/
        T_IMG_CTRL_HEADER_FORMAT *p_header = (T_IMG_CTRL_HEADER_FORMAT *) p_void;
        p_header->ctrl_flag.flag_value.not_ready = 0x1; /*make sure image is not ready, will use it later*/
        DFU_PRINT_TRACE3("dfu_update New Image Header:0x%08x, Signature:0x%08x, dfu_base_addr:0x%08x",
                         length, signature, dfu_base_addr);
    }

    if ((offset % FMC_SEC_SECTION_LEN) == 0)   //new page starts
    {
        flash_erase_sector(dfu_base_addr + offset);
    }
    else  // cross page
    {
        if ((offset / FMC_SEC_SECTION_LEN) != ((offset + length) / FMC_SEC_SECTION_LEN))
        {
            flash_erase_sector((dfu_base_addr + offset + length) & ~(FMC_SEC_SECTION_LEN - 1));
        }
    }
    start_addr = dfu_base_addr + offset;
    DFU_PRINT_TRACE1("*p_void:0x%08x", *p_void);
    s_val = flash_auto_read(start_addr | FLASH_OFFSET_TO_NO_CACHE);
    DFU_PRINT_TRACE1("*s_val:0x%08x", s_val);
    for (int i = 0; i < length; i = i + 4)
    {
        flash_auto_write(start_addr + i, *(uint32_t *)p_void);

        s_val = flash_auto_read(start_addr + i | FLASH_OFFSET_TO_NO_CACHE);
        DFU_PRINT_TRACE1("*s_val:0x%08x", s_val);
        if (s_val != *(uint32_t *)p_void)
        {
            DFU_PRINT_TRACE3("s_val:0x%08x, *p_void:0x%08x, i:0x%08x",
                             s_val, *(uint32_t *)p_void, i);
            result = __LINE__;
            os_delay(1000);
            goto L_Return;
        }
        else
        {
            p_void++;
        }

    }

//   ota_offset = offset + length; //for re-ota

L_Return:

    DFU_PRINT_INFO1("<==dfu_update result:%d \r\n", result);
    return result;
}

DATA_RAM_FUNCTION uint32_t sil_dfu_flash_erase(uint16_t signature, uint32_t offset)
{
    uint32_t result = 0;
    uint32_t dfu_base_addr;

    dfu_base_addr = get_temp_ota_bank_addr_by_img_id((T_IMG_ID)signature);
    if (dfu_base_addr == 0)
    {
        result = __LINE__;
        goto L_Return;
    }

    flash_erase_sector(dfu_base_addr + offset);
L_Return:
    DFU_PRINT_INFO1("<==sil_dfu_flash_erase result:%d \r\n", result);
    return result;
}

#if SUPPORT_ERASE_SUSPEND
bool erase_in_progress = false;
bool is_suspend = false;
DATA_RAM_FUNCTION void app_flash_erase_suspend()
{
    if (erase_in_progress && !is_suspend)
    {
        bool ret;
        uint8_t status1, status2;
        uint32_t ctrlr0 = SPIC->ctrlr0;
        uint32_t addr_len = SPIC->addr_length;

        signal = os_lock();

        spic_enable(DISABLE);
        spic_clr_multi_ch();
        spic_set_tx_mode();

        SPIC->addr_length = 0;
        spic_set_dr(DATA_BYTE, 0x75);

        spic_enable(ENABLE);
        spic_wait_busy();
        spic_enable(DISABLE);

        while (1)
        {
            ret = flash_cmd_rx(0x35, 1, &status1);
            ret &= flash_cmd_rx(0x05, 1, &status2);

            DFU_PRINT_INFO3("ret is %x,status1 is %x,status2 is %x", ret, status1, status2);

            if ((!(status1 & BIT_STATUS_WIP) && (status2 & BIT_STATUS_SUSPEND)) ||
                (!(status1 & BIT_STATUS_WIP) && !(status1 & BIT_STATUS_WEL)))
            {
                DFU_PRINT_INFO0("SUSPEND OK");
                platform_delay_us(1);
                is_suspend = true;
                break;
            }
        }
        //restore ctrl0 and addr_len register
        SPIC->ctrlr0 = ctrlr0;
        SPIC->addr_length = addr_len;
        os_unlock(signal);
    }
}
DATA_RAM_FUNCTION void app_flash_erase_resume()
{
    if (erase_in_progress && is_suspend)
    {
        uint32_t ctrlr0 = SPIC->ctrlr0;
        uint32_t addr_len = SPIC->addr_length;

        signal = os_lock();
        spic_enable(DISABLE);
        spic_clr_multi_ch();
        spic_set_tx_mode();

        SPIC->addr_length = 0;

        spic_set_dr(DATA_BYTE, 0x7A);
        spic_enable(ENABLE);
        spic_wait_busy();
        spic_enable(DISABLE);
        is_suspend = false;
        SPIC->ctrlr0 = ctrlr0;
        SPIC->addr_length = addr_len;
        os_unlock(signal);
        DFU_PRINT_INFO0("RESUME OK");
    }
}

DATA_RAM_FUNCTION bool app_flash_wait_busy(void)
{
    uint8_t status1 = 0, status2 = 0;
//    uint8_t flg = 1;
    uint32_t ctr = 0;
    bool ret;

    while (ctr++ <= 0x10000)
    {
        signal = os_lock();
        ret = flash_cmd_rx(0x05, 1, &status1);
        ret &= flash_cmd_rx(0x35, 1, &status2);

        DFU_PRINT_INFO3("ret is %x,status1 is %x,status2 is %x", ret, status1, status2);
        if (!ret)
        {
            os_unlock(signal);
            goto wait_busy_fail;
        }

        if (!(status1 & BIT_STATUS_WIP) && !(status2 & BIT_STATUS_SUSPEND))
        {
            erase_in_progress = false;
            os_unlock(signal);
            return true;
        }
        os_unlock(signal);
        DFU_PRINT_INFO1("CNT is %x", ctr);
        os_delay(1);
    }

wait_busy_fail:
    return false;
}

//app_flash_wait_busy()
DATA_RAM_FUNCTION bool app_flash_cmd_tx(uint8_t cmd, uint8_t data_len, uint8_t *data_buf)
{
    bool retval = true;
    DFU_PRINT_INFO0("app_flash_cmd_tx");
    uint32_t ctrlr0 = SPIC->ctrlr0;
    uint32_t addr_len = SPIC->addr_length;

    spic_enable(DISABLE);
    spic_clr_multi_ch();
    spic_set_tx_mode();

    SPIC->addr_length = data_len;

    spic_set_dr(DATA_BYTE, cmd);

    while (data_len--)
    {
        spic_set_dr(DATA_BYTE, *data_buf++);
    }

    spic_enable(ENABLE);

    if (!spic_wait_busy())
    {
        retval = false;
    }

    spic_enable(DISABLE);

    os_unlock(signal);

    if (retval == true && !app_flash_wait_busy())
    {
        retval = false;
    }
    //restore ctrl0 and addr_len register
    SPIC->ctrlr0 = ctrlr0;
    SPIC->addr_length = addr_len;

    return retval;
}
extern bool allowedDfuEnterDlps;
DATA_RAM_FUNCTION uint32_t flash_erase_sector(uint32_t addr)
{

    static uint8_t address[3];
    DFU_PRINT_INFO1("==> flash_erase_sector :%x \r\n", addr);
    address[0] = (addr >> 16) & 0xff;
    address[1] = (addr >> 8) & 0xff;
    address[2] = addr & 0xff;

    allowedDfuEnterDlps = false;//dlps io driver store/restore in xip
    signal = os_lock();
    erase_in_progress = true;
    app_flash_erase_resume();
    flash_write_enable();
    app_flash_cmd_tx(CMD_SECTOR_ERASE, 3, address);
    allowedDfuEnterDlps = true;
    //os_unlock(signal);
    return 0;
}
#endif
