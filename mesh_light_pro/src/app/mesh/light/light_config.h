/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     light_config.h
* @brief    Head file for light config.
* @details  Data structs and external functions declaration.
* @author   hector_huang
* @date     2018-11-16
* @version  v0.1
* *************************************************************************************
*/

#ifndef _LIGHT_CONFIG_H_
#define _LIGHT_CONFIG_H_

#include "group_light_app.h"

/** @brief light types
  * @{
  */
#define LIGHT_UNKNOWN       0
#define LIGHT_LIGHTNESS     1
#define LIGHT_CW            2
#define LIGHT_RGB           3
#define LIGHT_CWRGB         4
/** @} */

/** set value to 0 if not use demo dongle */
#define DEMO_BOARD                         1

#if GROUP_LIGHT_SUPPORTED
/** group light use cw or cwrgb light */
#define LIGHT_TYPE                         LIGHT_CWRGB
#else
/** set light types */
#define LIGHT_TYPE                         LIGHT_CWRGB
#endif

#define LIGHT_FLASH_PARAMS_APP_OFFSET      1900 //!< Shall be bigger than or equal to the size of mesh stack flash usage
#define LIGHT_POWER_ON_COUNT               5    //!< close the light LIGHT_POWER_ON_COUNT times to reset
#define LIGHT_POWER_ON_TIME                8000 //!< millisecond

/** set this value to 1 if pin value low means light on */
#define PIN_REVERSE                        0

#endif /** _LIGHT_CONFIG_H_ */


