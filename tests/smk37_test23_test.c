/* SPDX-License-Identifier: GPL-3.0-only */
#define SMK37_EXPANDED_PRESETS 1
#define SMK37_ENABLE_TRS 1
#define main smk37_legacy_baseline
#include "smk37_completion_test.c"
#undef main
static void protected_expanded(void){
    for(unsigned i=0;i<sizeof flash;++i)if(!SMK37_EXPANDED_RANGE(i,1))assert(flash[i]==saved[i]);
    for(unsigned a=0xb1000;a<0xb7000;a+=4096)for(unsigned j=3840;j<4096;++j)assert(flash[a+j]==255);
}
static void legacy_bank(unsigned length){
    memset(flash,255,sizeof flash);smk37_up_writable=1;
    smk37_legacy_presets_t old={0},previous={0};old.magic=0x31505553;
    up_rec_t *r=&old.record[7];r->used=UP_USED;r->ver=UP_VER;r->engine=ENGI_FM6;r->np=P_COUNT;
    memcpy(r->name,"OLD EIGHT",9);for(unsigned i=0;i<P_COUNT;++i)up_set_value(r,i,param_desc_of(ENGI_FM6,i)->def);
    up_set_value(r,P_ATK,23);memcpy(old.fm6[7],FM6_FACTORY[0],FM6_PACKED);
    old.favorites.user=128;old.prefs[30]=0x21;old.prefs[31]=0x18;old.prefs[22]=1;
    assert(!smk37_store_save(&smk37_up_io,0,&old,length,&previous));
}
int main(void){
    smk37_synth_init();smk37_storage_ready=smk37_storage_writable=1;
    legacy_bank(3116);memcpy(saved,flash,sizeof flash);smk37_trs_enabled=0;
    assert(!smk37_up_boot()&&smk37_user_capacity==32&&smk37_up_used(7));
    assert(favorites.user==128&&!smk37_trs_enabled);assert(!memcmp(flash,saved,sizeof flash));
    assert(!smk37_up_load(7)&&TSEL->p[P_ATK]==23);
    /* First migration retains the old bank at every interruption point. */
    for(int cut=0;cut<47;++cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());allowance=cut;
        int rc=smk37_up_store(31,"MIGRATED");allowance=-1;assert(!smk37_up_boot());
        assert(smk37_up_used(7));if(!rc)assert(smk37_up_used(31));protected_expanded();}
    for(header_cut=0;header_cut<32;++header_cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());
        assert(smk37_up_store(31,"TORN"));assert(!smk37_up_boot()&&smk37_up_used(7));protected_expanded();}header_cut=-1;
    memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());
    for(unsigned k=0;k<32;++k){TSEL->p[P_ATK]=k;assert(!smk37_up_store(k,"BANK32"));}
    assert(smk37_user_used==0xffffffffu);assert(smk37_up_store(32,"INVALID")==1);
    favorites.user=0x80000001u;smk37_pref_set(22,1);assert(!smk37_preferences_save());
    smk37_trs_enabled=0;favorites.user=0;assert(!smk37_up_boot()&&smk37_trs_enabled&&favorites.user==0x80000001u);
    for(unsigned k=0;k<32;++k){assert(!smk37_up_load(k));assert(TSEL->p[P_ATK]==(int)k);}
    /* Each failed replacement leaves a complete prior or new generation. */
    memcpy(saved,flash,sizeof flash);
    for(int cut=0;cut<47;++cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());TSEL->p[P_ATK]=60;allowance=cut;
        int rc=smk37_up_store(31,"NEW");allowance=-1;assert(!smk37_up_boot()&&smk37_user_capacity==32);
        assert(!smk37_up_load(31)&&TSEL->p[P_ATK]==(rc?31:60));protected_expanded();}
    for(header_cut=0;header_cut<32;++header_cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());
        assert(smk37_up_store(31,"TORN"));assert(!smk37_up_boot());assert(!smk37_up_load(31)&&TSEL->p[P_ATK]==31);protected_expanded();}header_cut=-1;
    uint8_t list[]={24,8};assert(req(16,list,2)); /* upper page of browser slots */
    uint8_t store[]={31,'W','E','B',0};assert(req(19,store,5)&&!smk37_ed_out[6]);
    store[0]=32;assert(req(19,store,5)&&smk37_ed_out[6]==1);
    uint8_t erase[]={31};assert(req(21,erase,1)&&!smk37_ed_out[6]&&!smk37_up_used(31));assert(!smk37_up_boot()&&!smk37_up_used(31));
    /* Unknown expansion never grants erase; the original bank stays usable. */
    legacy_bank(3116);flash[0xb1000]=42;assert(!smk37_up_boot()&&smk37_user_capacity==8);
    assert(smk37_up_store(8,"NO")==1);assert(!smk37_up_store(0,"LEGACY"));assert(flash[0xb1000]==42);
    legacy_bank(2564);assert(!smk37_up_boot()&&smk37_up_used(7)&&smk37_user_capacity==32);assert(!smk37_up_load(7)&&TSEL->p[P_ATK]==23);
    smk37_pref_set(22,0);assert(!smk37_preferences_save());smk37_trs_enabled=1;assert(!smk37_up_boot()&&!smk37_trs_enabled);
    assert(!SMK37_EXPANDED_RANGE(0xb0fff,1)&&!SMK37_EXPANDED_RANGE(0xb6fff,2)&&!SMK37_EXPANDED_RANGE(0xb1000,UINT32_MAX));
    puts("Test23: 32 presets, legacy migration, all interrupted commits, protected flash, browser bounds and TRS persistence passed");
}
