
/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2018 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */
#ifndef _USER_FLASH_API_H_
#define _USER_FLASH_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "board.h"
#include <stdbool.h>
#include <stdint.h>
#include "user_flash_driver.h"
/**
 * @addtogroup USER_FLASH
 * @{
 */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * @defgroup User_Flash_Exported_Functions User Flash Exported Functions
 * @brief
 * @{
 */
bool unlock_flash_all(void);
void lock_flash(void);
uint32_t flash_erase_sector(uint32_t addr);
uint32_t sil_dfu_update(uint16_t signature, uint32_t offset, uint32_t length, uint32_t *p_void);
uint32_t sil_dfu_flash_erase(uint16_t signature, uint32_t offset);
/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif
