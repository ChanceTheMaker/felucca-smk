/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Portable payload only. Requires upstream up_rec_t / favorites_t. No flash IO.
 * Keeps the original eight-slot payload available for migration and fallback. */
typedef struct {uint32_t magic;up_rec_t record[8];uint8_t fm6[8][FM6_PACKED];favorites_t favorites;uint8_t prefs[32];} smk37_legacy_presets_t;
typedef struct {uint32_t magic;up_rec_t record[32];uint8_t fm6[32][FM6_PACKED];favorites_t favorites;uint8_t prefs[32];} smk37_expanded_presets_t;
_Static_assert(sizeof(smk37_legacy_presets_t)==3116,"Test18/19 bank format");
_Static_assert(sizeof(smk37_expanded_presets_t)==10796,"32-slot bank fits three-sector object");
static int smk37_expand_presets(smk37_expanded_presets_t *out,const void *data,uint32_t n){
    if(!data||(n!=2564&&n!=3116))return -1;
    uint32_t magic;memcpy(&magic,data,4);if(magic!=0x31505553u)return -1;
    /* Caller retains the source; this converter never writes a flash address. */
    const uint8_t *old=data;memset(out,0,sizeof *out);out->magic=0x32505553u;
    memcpy(out->record,old+4,8*sizeof(up_rec_t));
    memcpy(out->fm6,old+4+8*sizeof(up_rec_t),8*FM6_PACKED);
    for(unsigned i=0;i<8;++i)up_migrate(&out->record[i]);
    if(n==3116){memcpy(&out->favorites,old+2564,sizeof(favorites_t));memcpy(out->prefs,old+2564+sizeof(favorites_t),32);out->favorites.user&=255u;}
    return 0;
}
