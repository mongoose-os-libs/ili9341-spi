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

#ifndef __MGOS_ILI9341_HAL_H
#define __MGOS_ILI9341_HAL_H

#include "mgos.h"
#include "mgos_spi.h"

// ILI9341 Commands
#define ILI9341_NOP            0x00
#define ILI9341_SWRESET        0x01
#define ILI9341_RDDID          0x04
#define ILI9341_RGB_BGR        0x08
#define ILI9341_RDDST          0x09
#define ILI9341_RDMODE         0x0A
#define ILI9341_RDMADCTL       0x0B
#define ILI9341_RDPIXFMT       0x0C
#define ILI9341_RDIMGFMT       0x0D
#define ILI9341_RDSELFDIAG     0x0F

#define ILI9341_SLPIN          0x10
#define ILI9341_SLPOUT         0x11
#define ILI9341_PTLON          0x12
#define ILI9341_NORON          0x13

#define ILI9341_INVOFF         0x20
#define ILI9341_INVON          0x21

#define ILI9341_GAMMASET       0x26
#define ILI9341_DISPOFF        0x28
#define ILI9341_DISPON         0x29
#define ILI9341_CASET          0x2A
#define ILI9341_PASET          0x2B
#define ILI9341_RAMWR          0x2C
#define ILI9341_RAMRD          0x2E

#define ILI9341_PTLAR          0x30
#define ILI9341_MADCTL         0x36
#define ILI9341_PIXFMT         0x3A

#define ILI9341_FRMCTR1        0xB1
#define ILI9341_FRMCTR2        0xB2
#define ILI9341_FRMCTR3        0xB3
#define ILI9341_INVCTR         0xB4
#define ILI9341_DFUNCTR        0xB6
#define ILI9341_ENTRYM         0xB7
#define ILI9341_VCOMS          0xBB

#define ILI9341_PWCTR1         0xC0
#define ILI9341_PWCTR2         0xC1
#define ILI9341_PWCTR3         0xC2
#define ILI9341_PWCTR4         0xC3
#define ILI9341_PWCTR5         0xC4
#define ILI9341_VMCTR1         0xC5
#define ILI9341_FRCTRL2        0xC6
#define ILI9341_VMCTR2         0xC7
#define ILI9341_POWERA         0xCB
#define ILI9341_POWERB         0xCF

//#define ILI9341_PWCTR1        0xD0
#define ILI9341_RDID1          0xDA
#define ILI9341_RDID2          0xDB
#define ILI9341_RDID3          0xDC
#define ILI9341_RDID4          0xDD

#define ILI9341_GMCTRP1        0xE0
#define ILI9341_GMCTRN1        0xE1
#define ILI9341_POWER_SEQ      0xED
#define ILI9341_DTCA           0xE8
#define ILI9341_DTCB           0xEA

#define ILI9341_PRC            0xF7
#define ILI9341_3GAMMA_EN      0xF2

#define ILI9341_INVALID_CMD    0xFF

#define MADCTL_MY              0x80
#define MADCTL_MX              0x40
#define MADCTL_MV              0x20
#define MADCTL_ML              0x10
#define MADCTL_MH              0x04

#define ILI9341_DELAY          0x80

#endif // __MGOS_ILI9341_HAL_H
