/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/**************************************************************************//**
 * @file     ospi_xip_user.h
 * @author   Khushboo Singh
 * @email    khushboo.singh@alifsemi.com
 * @version  V1.0.0
 * @date     05-Dec-2022
 * @brief    User configuration parameters for flash XIP application.
 * @bug      None.
 * @Note     None
 ******************************************************************************/

#ifndef OSPI_XIP_USER_H_
#define OSPI_XIP_USER_H_

#ifdef  __cplusplus
extern "C"
{
#endif
//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h> OSPI XIP Configuration
// =========================================

#define OSPI0                                    0U
#define OSPI1                                    1U


/**
  \def OSPI_XIP_INSTANCE
  \brief OSPI instance configuration. Can be set to either \ref OSPI0 or \ref OSPI1.
*/

//   <o OSPI_XIP_INSTANCE> OSPI instance selection
//      <OSPI0=>  0
//      <OSPI1=>  1
//   <i> OSPI instance selection

#define OSPI_XIP_INSTANCE                        OSPI1

/**
  \def OSPI_CLOCK_MHZ
  \brief Required ospi clock output (sclk_out).
*/

//   <o> OSPI Clock (in MHz)
//   <i> Defines the frequency of sclk_out of SPI controller.
//   <i> Default:50

#define OSPI_CLOCK_MHZ                           100
#define OSPI_CLOCK                               (OSPI_CLOCK_MHZ * 1000000)

/**
  \def OSPI_XIP_ENABLE_AES_DECRYPTION
  \brief OSPI AES Decryption support. Can be set to either 0(disable) or 1(enable).
*/

//   <o OSPI_XIP_ENABLE_AES_DECRYPTION> OSPI AES Decryption support
//      <0=>  Disable AES Decryption
//      <1=>  Enable AES Decryption
//   <i> OSPI AES Decryption support

#define OSPI_XIP_ENABLE_AES_DECRYPTION           0

// </h>
//------------- <<< end of configuration section >>> ---------------------------

#ifdef  __cplusplus
}
#endif

#endif /* OSPI_XIP_USER_H_ */


