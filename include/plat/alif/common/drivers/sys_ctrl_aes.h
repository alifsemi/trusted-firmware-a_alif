/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/******************************************************************************
 * @file     sys_ctrl_aes.h
 * @author   Silesh C V
 * @email    silesh@alifsemi.com
 * @version  V1.0.0
 * @date     30-May-2023
 * @brief    AES control.
 * @bug      None.
 * @Note     None
 ******************************************************************************/

#ifndef SYS_CTRL_AES_H
#define SYS_CTRL_AES_H

#include <stdint.h>
#include "ospi_private.h"
//#include "peripheral_types.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define     __IOM    volatile                   /*! Defines 'read / write' structure member permissions */

/**
  * @brief AES (AES)
  */

typedef struct {                                /*!<  AES Structure                                                            */
  __IOM uint32_t  AES_CONTROL;                  /*!< (@ 0x00000000) AES Control Register                                       */
  __IOM uint32_t  AES_INTERRUPT;                /*!< (@ 0x00000004) AES Interrupt Control Register                             */
  __IOM uint32_t  AES_INTERRUPT_MASK;           /*!< (@ 0x00000008) AES Interrupt Mask Register                                */
  __IOM uint32_t  AES_KEY_0;                    /*!< (@ 0x0000000C) AES Key 0 RegisterNOTE: Internal register!                 */
  __IOM uint32_t  AES_KEY_1;                    /*!< (@ 0x00000010) AES Key 1 RegisterNOTE: Internal register!                 */
  __IOM uint32_t  AES_KEY_2;                    /*!< (@ 0x00000014) AES Key 2 RegisterNOTE: Internal register!                 */
  __IOM uint32_t  AES_KEY_3;                    /*!< (@ 0x00000018) AES Key 3 RegisterNOTE: Internal register!                 */
  __IOM uint32_t  AES_TIMEOUT_VAL;              /*!< (@ 0x0000001C) ReservedNOTE: Internal register!                           */
  __IOM uint32_t  AES_RXDS_DELAY;               /*!< (@ 0x00000020) AES RXDS Delay Register                                    */
} AES_Type;

static inline void aes_set_rxds_delay(AES_Type *aes, uint8_t rxds_delay)
{
    aes->AES_RXDS_DELAY = rxds_delay;
}

static inline void aes_enable_xip(AES_Type *aes)
{
    aes->AES_CONTROL |= AES_CONTROL_XIP_EN;
}

static inline void aes_disable_xip(AES_Type *aes)
{
    aes->AES_CONTROL &= ~AES_CONTROL_XIP_EN;
}
#ifdef  __cplusplus
}
#endif
#endif /* SYS_CTRL_AES_H */
