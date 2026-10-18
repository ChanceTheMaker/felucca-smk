/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "smk37_flash_profile.h"
int main(void){
    assert(smk37_flash_supported_id(0x856014));
    assert(smk37_flash_supported_id(0xeb6014));
    const uint32_t rejected[]={0,0xffffff,0xeb6015,0x856015,0xef4014,0x016014,0x01856014};
    for(unsigned i=0;i<sizeof rejected/sizeof *rejected;++i)
        assert(!smk37_flash_supported_id(rejected[i]));
    return 0;
}
