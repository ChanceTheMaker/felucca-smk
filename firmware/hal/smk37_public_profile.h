/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "smk37_trim.h"
/* Identical protected boot region in official Pro016 and Elite016 files.
 * A model/layout fingerprint, never an owner's calibration fingerprint. */
#define SMK37_PUBLIC_HEAD_CRC 0x3a04ad23u
#ifndef SMK37_PUBLIC_BUILD
#define SMK37_PUBLIC_BUILD 0
#endif
static uint32_t smk37_profile_crc(const uint8_t *p,unsigned n,uint32_t c){
    while(n--){c^=*p++;for(unsigned b=0;b<8;++b)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return c;
}
