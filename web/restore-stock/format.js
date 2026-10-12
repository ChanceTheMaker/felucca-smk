// SPDX-License-Identifier: GPL-3.0-only
export const HEADER=1024, LO=0x4000, HI=0x9d000;
export function crc32(data){let c=0xffffffff;for(const b of data){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((0-(c&1))&0xedb88320);}return (c^0xffffffff)>>>0;}
