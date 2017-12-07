/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* TODO: High-level file comment. */

#ifndef __MGOS_ILI9341_HAL_H
#define __MGOS_ILI9341_HAL_H

#include "mgos.h"
#include "mgos_spi.h"

// ILI9341 Display orientation bits
#define ILI9341_FLIP_X        0x01
#define ILI9341_FLIP_Y        0x02
#define ILI9341_SWITCH_XY     0x04

// ILI9341 Commands
#define ILI9341_NOP           0x00
#define ILI9341_SWRESET       0x01
#define ILI9341_RDDID         0x04
#define ILI9341_RGB_BGR       0x08
#define ILI9341_RDDST         0x09
#define ILI9341_RDMODE        0x0A
#define ILI9341_RDMADCTL      0x0B
#define ILI9341_RDPIXFMT      0x0C
#define ILI9341_RDIMGFMT      0x0D
#define ILI9341_RDSELFDIAG    0x0F

#define ILI9341_SLPIN         0x10
#define ILI9341_SLPOUT        0x11
#define ILI9341_PTLON         0x12
#define ILI9341_NORON         0x13

#define ILI9341_INVOFF        0x20
#define ILI9341_INVON         0x21

#define ILI9341_GAMMASET      0x26
#define ILI9341_DISPOFF       0x28
#define ILI9341_DISPON        0x29
#define ILI9341_CASET         0x2A
#define ILI9341_PASET         0x2B
#define ILI9341_RAMWR         0x2C
#define ILI9341_RAMRD         0x2E

#define ILI9341_PTLAR         0x30
#define ILI9341_MADCTL        0x36
#define ILI9341_PIXFMT        0x3A

#define ILI9341_FRMCTR1       0xB1
#define ILI9341_FRMCTR2       0xB2
#define ILI9341_FRMCTR3       0xB3
#define ILI9341_INVCTR        0xB4
#define ILI9341_DFUNCTR       0xB6
#define ILI9341_ENTRYM        0xB7
#define ILI9341_VCOMS         0xBB

#define ILI9341_PWCTR1        0xC0
#define ILI9341_PWCTR2        0xC1
#define ILI9341_PWCTR3        0xC2
#define ILI9341_PWCTR4        0xC3
#define ILI9341_PWCTR5        0xC4
#define ILI9341_VMCTR1        0xC5
#define ILI9341_FRCTRL2       0xC6
#define ILI9341_VMCTR2        0xC7
#define ILI9341_POWERA        0xCB
#define ILI9341_POWERB        0xCF

//#define ILI9341_PWCTR1        0xD0
#define ILI9341_RDID1         0xDA
#define ILI9341_RDID2         0xDB
#define ILI9341_RDID3         0xDC
#define ILI9341_RDID4         0xDD

#define ILI9341_GMCTRP1       0xE0
#define ILI9341_GMCTRN1       0xE1
#define ILI9341_POWER_SEQ     0xED
#define ILI9341_DTCA          0xE8
#define ILI9341_DTCB          0xEA

#define ILI9341_PRC           0xF7
#define ILI9341_3GAMMA_EN     0xF2

#define MADCTL_MY             0x80
#define MADCTL_MX             0x40
#define MADCTL_MV             0x20
#define MADCTL_ML             0x10
#define MADCTL_MH             0x04

#define ILI9341_DELAY   0x80

static const uint8_t ILI9341_init[] = {
  24,                                                                // 24 commands in list
  ILI9341_SWRESET, ILI9341_DELAY,                                    //  1: Software reset, no args, w/delay
  250,                                                               //     200 ms delay
  ILI9341_POWERA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  ILI9341_POWERB, 3, 0x00, 0XC1, 0X30,
  0xEF, 3, 0x03, 0x80, 0x02,
  ILI9341_DTCA, 3, 0x85, 0x00, 0x78,
  ILI9341_DTCB, 2, 0x00, 0x00,
  ILI9341_POWER_SEQ, 4, 0x64, 0x03, 0X12, 0X81,
  ILI9341_PRC, 1, 0x20,
  ILI9341_PWCTR1, 1, 0x23,                                           // Power control VRH[5:0]
  ILI9341_PWCTR2, 1, 0x10,                                           // Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1, 2, 0x3e, 0x28,                                     // VCM control
  ILI9341_VMCTR2, 1, 0x86,                                           // VCM control2
  ILI9341_MADCTL, 1,                                                 // Memory Access Control (orientation)
  (MADCTL_MX | ILI9341_RGB_BGR),
  // *** INTERFACE PIXEL FORMAT: 0x66 -> 18 bit; 0x55 -> 16 bit
  ILI9341_PIXFMT, 1, 0x55,
  ILI9341_INVOFF, 0,
  ILI9341_FRMCTR1, 2, 0x00, 0x18,
  ILI9341_DFUNCTR, 4, 0x08, 0x82, 0x27, 0x00,                        // Display Function Control
  ILI9341_PTLAR, 4, 0x00, 0x00, 0x01, 0x3F,
  ILI9341_3GAMMA_EN, 1, 0x00,                                        // 3Gamma Function: Disable (0x02), Enable (0x03)
  ILI9341_GAMMASET, 1, 0x01,                                         // Gamma curve selected (0x01, 0x02, 0x04, 0x08)
  ILI9341_GMCTRP1, 15,                                               // Positive Gamma Correction
  0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1, 15,                                               // Negative Gamma Correction
  0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  ILI9341_SLPOUT, ILI9341_DELAY,                                     // Sleep out
  200,                                                               // 120 ms delay
  ILI9341_DISPON, ILI9341_DELAY, 200,
};

#endif // __MGOS_ILI9341_HAL_H
