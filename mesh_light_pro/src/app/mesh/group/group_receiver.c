/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     group_reciever.c
  * @brief    Source file for group reciever.
  * @details  Data types and external functions declaration.
  * @author   bill
  * @date     2018-11-06
  * @version  v1.0
  * *************************************************************************************
  */

/* Add Includes here */
#include <string.h>
#include "group.h"
#include "platform_diagnose.h"
#include "ftl.h"

#define GROUP_RECEIVER_MAX_TRANSMITTER_NUM                  3
#define GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER       4
#define GROUP_FLASH_PARAMS_OFFSET                           1948

#define GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE            (((1 + 1 + 6 + GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER + 4 + 3)/4)*4)

typedef struct
{
    bool valid;
    uint8_t bt_addr[6];
    uint8_t group[GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER];
    uint32_t rank;
    uint8_t tid;
} group_transmitter_info_t;

typedef struct
{
    group_transmitter_info_t gti[GROUP_RECEIVER_MAX_TRANSMITTER_NUM];
    group_receiver_state_t state;
    pf_group_receiver_receive_cb_t cfg_cb;
    pf_group_receiver_receive_cb_t ctl_cb;
} group_receiver_ctx_t;

static group_receiver_ctx_t grc;

static bool group_receiver_group_find(int index, uint8_t group)
{
    int loop;
    for (loop = 0; loop < GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER; loop++)
    {
        if (grc.gti[index].group[loop] == group)
        {
            return true;
        }
    }
    return false;
}

static bool group_receiver_group_delete(int index)
{
    bool ret = false;
    for (int loop = 0; loop < GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER; loop++)
    {
        if (grc.gti[index].group[loop] != GROUP_INVALID)
        {
            grc.gti[index].group[loop] = GROUP_INVALID;
            ret = true;
        }
    }
    return ret;
}

static bool group_receiver_group_add(int index, uint8_t group)
{
    if (group_receiver_group_find(index, group))
    {
        return false;
    }

    for (int loop = 0; loop < GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER; loop++)
    {
        if (grc.gti[index].group[loop] == GROUP_INVALID)
        {
            grc.gti[index].group[loop] = group;
            return true;
        }
    }
    printw("group_receiver_group_add: no space left for the new group");
    return false;
}

static int group_receiver_allocate(uint8_t bt_addr[], uint8_t tid, uint32_t rank)
{
    int loop;
    for (loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        if (false == grc.gti[loop].valid)
        {
            grc.gti[loop].valid = true;
            memcpy(grc.gti[loop].bt_addr, bt_addr, 6);
            memset(grc.gti[loop].group, GROUP_INVALID, GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER);
            grc.gti[loop].rank = rank;
            grc.gti[loop].tid = tid;
            return loop;
        }
    }
    return -1;
}

static bool group_receiver_free(int index)
{
    if (false == grc.gti[index].valid)
    {
        return false;
    }
    grc.gti[index].valid = false;
    return true;
}

static int group_receiver_find(uint8_t bt_addr[], uint8_t tid)
{
    int loop;
    for (loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        if (grc.gti[loop].valid)
        {
            if (0 == memcmp(bt_addr, grc.gti[loop].bt_addr, 6))
            {
                if (tid == grc.gti[loop].tid)
                {
                    return -2;
                }
                else
                {
                    grc.gti[loop].tid = tid;
                }
                return loop;
            }
        }
    }
    return -1;
}

bool group_receiver_check(void)
{
    for (int loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        if (grc.gti[loop].valid)
        {
            return true;
        }
    }
    return false;
}

static int group_receiver_rank(bool order)
{
    int ret = -1;
    uint32_t rank = order ? 0x00000000 : 0xffffffff;
    for (int loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        if (grc.gti[loop].valid)
        {
            if (order)
            {
                if (rank <= grc.gti[loop].rank)
                {
                    ret = loop;
                    rank = grc.gti[loop].rank;
                }
            }
            else
            {
                if (rank >= grc.gti[loop].rank)
                {
                    ret = loop;
                    rank = grc.gti[loop].rank;
                }
            }
        }
    }
    return ret;
}

static bool group_receiver_nvm_save(int index)
{
    uint8_t flash_data[GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE] = {0};
    flash_data[0] = grc.gti[index].valid;
    memcpy(flash_data + 1, grc.gti[index].bt_addr, 6);
    memcpy(flash_data + 7, grc.gti[index].group, GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER);
    memcpy(flash_data + 7 + GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER, &grc.gti[index].rank, 4);
    return 0 == ftl_save((void *)flash_data,
                         GROUP_FLASH_PARAMS_OFFSET + index * GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE,
                         grc.gti[index].valid ? GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE : 4);
}

static bool group_receiver_nvm_load(void)
{
    uint32_t ret;
    int loop;
    for (loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        uint8_t flash_data[GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE] = {0};
        ret = ftl_load((void *)flash_data,
                       GROUP_FLASH_PARAMS_OFFSET + loop * GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE,
                       GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE);
        if (ret == 0 && flash_data[0] == 1)
        {
            grc.gti[loop].valid = true;
            memcpy(grc.gti[loop].bt_addr, flash_data + 1, 6);
            memcpy(grc.gti[loop].group, flash_data + 7, GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER);
            memcpy(&grc.gti[loop].rank, flash_data + 7 + GROUP_RECEIVER_MAX_GROUP_NUM_EACH_TRANSMITTER, 4);
        }
    }
    return true;
}

bool group_receiver_nvm_clear(void)
{
    uint32_t ret;
    int loop;
    for (loop = 0; loop < GROUP_RECEIVER_MAX_TRANSMITTER_NUM; loop++)
    {
        uint8_t flash_data[GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE] = {0};
        ret = ftl_load((void *)flash_data,
                       GROUP_FLASH_PARAMS_OFFSET + loop * GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE,
                       GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE);
        if (ret == 0 && flash_data[0] == 1)
        {
            flash_data[0] = 0;
            ftl_save((void *)flash_data, GROUP_FLASH_PARAMS_OFFSET + loop *
                     GROUP_FLASH_PARAMS_EACH_TRANSMITTER_SIZE, 4);
        }
    }
    return true;
}

void group_receiver_receive(T_LE_SCAN_INFO *ple_scan_info)
{
    group_msg_t *pmsg;
    uint8_t *pbuffer = ple_scan_info->data;
    uint8_t len = ple_scan_info->data_len;

    /*
    if (ple_scan_info->adv_type != GAP_ADV_EVT_TYPE_NON_CONNECTABLE)
    {
        return;
    }
    */

    if (pbuffer[0] + 1 != len || len < 5 + MEMBER_OFFSET(group_msg_t, cfg))
    {
        return;
    }

    if (pbuffer[1] != GAP_ADTYPE_MANUFACTURER_SPECIFIC)
    {
        return;
    }

    uint16_t company_id = LE_EXTRN2WORD(pbuffer + 2);
    if (company_id != MANUFACTURE_ADV_DATA_COMPANY_ID)
    {
        return;
    }

    uint8_t manual_adv_type = pbuffer[4];
    if (manual_adv_type != MANUFACTURE_ADV_DATA_TYPE_GROUP)
    {
        return;
    }

    pmsg = (group_msg_t *)(pbuffer + 5);
    if (pmsg->rfu != 0)
    {
        return;
    }

    int index = group_receiver_find(ple_scan_info->bd_addr, pmsg->tid);
    if (index <= -2)
    {
        return;
    }

    len -= 5;
    printi("group_receiver_receive: state %d, tid %d, type %d, len %d", grc.state, pmsg->tid,
           pmsg->type, len);
    dprinti((uint8_t *)pmsg, len);
    len -= MEMBER_OFFSET(group_msg_t, cfg);

    if (grc.state == GROUP_RECEIVER_STATE_CFG && pmsg->type == GROUP_MSG_TYPE_CFG)
    {
        bool save_flash = false;
        if (pmsg->cfg.opcode == GROUP_CFG_OPCODE_GROUP &&
            len == MEMBER_OFFSET(group_cfg_t, group) + sizeof(uint8_t))
        {
            if (pmsg->cfg.group == GROUP_INVALID)
            {
                if (index >= 0)
                {
#if 1
                    /* delete all group */
                    if (group_receiver_group_delete(index))
                    {
                        save_flash = true;
                    }
#else
                    /* delete the transmitter */
                    group_receiver_free(index);
                    save_flash = true;
#endif
                }
            }
            else
            {
                if (index == -1)
                {
                    index = group_receiver_rank(true);
                    uint32_t rank = index >= 0 ? grc.gti[index].rank + 1 : 0x00000000;
                    index = group_receiver_allocate(ple_scan_info->bd_addr, pmsg->tid, rank);
                    if (index < 0)
                    {
#if GROUP_RECEIVER_PREEMPTIVE_MODE
                        index = group_receiver_rank(false);
                        printw("group_receiver_receive: space %d emptived by rank %d", index, rank);
                        group_receiver_free(index);
                        index = group_receiver_allocate(ple_scan_info->bd_addr, pmsg->tid, rank);
                        save_flash = true;
#else
                        printw("group_receiver_receive: no space left for the new transmitter");
                        return;
#endif
                    }
                    else
                    {
                        save_flash = true;
                    }
                }

                if (pmsg->cfg.group != GROUP_ALL)
                {
                    /* add one group */
                    if (group_receiver_group_add(index, pmsg->cfg.group))
                    {
                        save_flash = true;
                    }
                }
            }
        }

        if (save_flash)
        {
            group_receiver_nvm_save(index);
            if (grc.cfg_cb)
            {
                grc.cfg_cb(NULL, 0);
            }
        }
    }

    //&& grc.state == GROUP_RECEIVER_STATE_NORMAL
    if (pmsg->type == GROUP_MSG_TYPE_CTL && pmsg->ctl.group != GROUP_INVALID)
    {
        if (index >= 0)
        {
            if (pmsg->ctl.group == GROUP_ALL || group_receiver_group_find(index, pmsg->ctl.group))
            {
                if (grc.ctl_cb)
                {
                    grc.ctl_cb((uint8_t *)&pmsg->ctl, len);
                }
            }
        }
#if GROUP_RECEIVER_RX_EVEN_NOT_CFG
        else
        {
            if (pmsg->ctl.group == GROUP_ALL && false == group_receiver_check())
            {
                static uint8_t tid = 0x3f;
                if (tid != pmsg->tid)
                {
                    tid = pmsg->tid;
                    if (grc.ctl_cb)
                    {
                        grc.ctl_cb((uint8_t *)&pmsg->ctl, len);
                    }
                }
            }
        }
#endif
    }
}

void group_receiver_init(void)
{
    group_receiver_nvm_load();
}

void group_receiver_state_set(group_receiver_state_t state)
{
    grc.state = state;
}

group_receiver_state_t group_receiver_state_get(void)
{
    return grc.state;
}

void group_receiver_reg_cb(group_msg_type_t type, pf_group_receiver_receive_cb_t pf)
{
    if (type == GROUP_MSG_TYPE_CTL)
    {
        grc.ctl_cb = pf;
    }
    else if (type == GROUP_MSG_TYPE_CFG)
    {
        grc.cfg_cb = pf;
    }
}
