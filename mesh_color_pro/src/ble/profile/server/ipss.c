/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ipss.c
  * @brief    Source file for using internet protocol support service.
  * @details  Global data and function implement.
  * @author   Jeff_Zheng
  * @date     2017-12-05
  * @version  v1.0
  * *************************************************************************************
  */

#include <string.h>
#include "trace.h"
#include "profile_server.h"
#include "ipss.h"



static const T_ATTRIB_APPL ipss_attr_tbl[] =
{
    /* <<Primary Service>>      0*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_IPSS),               /* service UUID */
            HI_WORD(GATT_UUID_IPSS)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    }

};

uint16_t ipss_attr_tbl_len = sizeof(ipss_attr_tbl);
//static P_FUN_SERVER_GENERAL_CB p_fn_ipss_cb = NULL;

const T_FUN_GATT_SERVICE_CBS ipss_cbs =
{
    NULL,       // Read callback function pointer
    NULL,       // Write callback function pointer
    NULL,       // Authorization callback function pointer
};

/**
  * @brief add ipss service to application.
  *
  * @param[in] p_func   pointer of app callback function called by profile.
  * @return service id auto generated by profile layer.
  * @retval service id
  */
uint8_t ipss_add_service(void *p_func)
{
    uint8_t service_id;
    if (false == server_add_service(&service_id, (uint8_t *)ipss_attr_tbl, ipss_attr_tbl_len, ipss_cbs))
    {
        service_id = SERVICE_PROFILE_GENERAL_ID;
    }
    else
    {
        //p_fn_ipss_cb = (P_FUN_SERVER_GENERAL_CB)p_func;
    }

    PROFILE_PRINT_ERROR1("ipss_add_service: service_id %d", service_id);

    return service_id;
}



