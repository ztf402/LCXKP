/**
 ******************************************************************************
 * @file        lcxkp_cfg.h
 * @version     v1.0
 * @date        2026-01-07
 * @author      ztf402
 * @brief       Switch the hal library
 * @copyright   (C) Copyright 2026, ztf402.
 *              All Rights Reserved.
 ******************************************************************************
 * @details
 * 
 * @par Function List
 * 
 * @par Change Log
 * | Version | Date | Author | Description |
 * |----------|------|---------|-------------|
 * | v1.0 | 2026-01-07 | ztf402 | Initial version |
 ******************************************************************************
 */


#ifndef __LCXKP_CFG_H_
#define __LCXKP_CFG_H_


#endif


 

#ifndef __LCXKP_CFG_H
#define __LCXKP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MODE_SPI 1
#define MODE_I2S 2

#define SPI2_OR_I2S2 MODE_SPI //You need to switch here to use hal library


#if SPI2_OR_I2S2==MODE_SPI
    #define HAL_SPI_MODULE_ENABLED
#endif
#if SPI2_OR_I2S2==MODE_I2S
    #define HAL_I2S_MODULE_ENABLED
#endif


#endif
