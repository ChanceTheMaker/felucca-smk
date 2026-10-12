/* SPDX-License-Identifier: GPL-3.0-only
 * Commands 34..38 match editor_preferences.c (Salt FM-1 protocol).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments.
 * SMK persistence uses the existing preset A/B sectors in the main loop. */
static void smk37_ed_u28(unsigned n){for(unsigned i=0;i<4;++i)smk37_ed_byte(n>>(7*i));}
static void smk37_ed_ui_state(void){
    smk37_ed_byte(15);smk37_ed_byte(smk37_palette);smk37_ed_byte(smk37_bold);smk37_ed_byte(monitor_mode);smk37_ed_byte(favorites.filter);
    uint32_t h=2166136261u;for(unsigned i=0;i<sizeof favorites;++i)h=(h^((const uint8_t *)&favorites)[i])*16777619u;
    smk37_ed_u28(h);smk37_ed_u28(smk37_store_crc(&smk37_up,sizeof smk37_up));
}
static unsigned smk37_ed_ui_save(void){
    if(!smk37_up_writable)return 3;
    if(!smk37_up_idle()){smk37_prefs_request=1;return 4;}
    return smk37_preferences_save()?3:0;
}
static unsigned smk37_editor_preferences(unsigned cmd,const uint8_t *a,unsigned n){
    smk37_ed_begin(cmd);unsigned rc=0;
    switch(cmd){
    case 34:if(n)return 0;smk37_ed_ui_state();break;
    case 35:
        if(n!=2 || a[0]>3 || a[1]>=(a[0]==0?UI_NPALETTES:a[0]==2?3:2))rc=1;
        else {fm1_irq_off();if(a[0]<3)smk37_pref_set(a[0],a[1]);else favorites.filter=a[1];smk37_browser_fix();fm1_irq_on();rc=smk37_ed_ui_save();}
        smk37_ed_byte(rc);smk37_ed_byte(n?a[0]:127);smk37_ed_byte(n>1?a[1]:127);smk37_ed_ui_state();break;
    case 36:if(n)return 0;smk37_ed_byte(UI_NPALETTES);for(unsigned i=0;i<UI_NPALETTES;++i)smk37_ed_string(UI_PALETTES[i].name,12);break;
    case 37:case 38:{
        int p=n==4?smk37_ed_read(a+1):-1;unsigned limit=n==4 && a[0]<=NENGINES?(a[0]==NENGINES?smk37_user_capacity:ENGINES[a[0]]->npresets):0;
        if(n!=4 || p<0 || !limit || (cmd==37?(!a[3] || a[3]>32 || (unsigned)p+a[3]>limit):((unsigned)p>=limit || a[3]>1 || (a[0]==NENGINES && a[3] && !smk37_up_used(p))))){smk37_ed_byte(1);break;}
        if(cmd==38){fm1_irq_off();favorite_set(a[0],p,a[3]);smk37_browser_fix();fm1_irq_on();rc=smk37_ed_ui_save();}
        smk37_ed_byte(rc);smk37_ed_byte(a[0]);smk37_ed_value(p);smk37_ed_byte(a[3]);
        if(cmd==37)for(unsigned i=0;i<a[3];++i)smk37_ed_byte(favorite_has(a[0],p+i));break;}
    default:return 0;
    }
    return smk37_ed_finish();
}
