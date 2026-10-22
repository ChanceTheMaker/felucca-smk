/* SPDX-License-Identifier: GPL-3.0-only
 * Commands 34..38 match editor_preferences.c (Salt FM-1 protocol).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments.
 * SMK persistence uses the existing preset A/B sectors in the main loop. */
static void smk37_ed_u28(unsigned n){for(unsigned i=0;i<4;++i)smk37_ed_byte(n>>(7*i));}
static void smk37_ed_ui_state(void){
    smk37_ed_byte(15);smk37_ed_byte(palette_cur());smk37_ed_byte(smk37_bold);smk37_ed_byte(monitor_mode);smk37_ed_byte(favorites.filter);
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
        if(n!=2 || a[0]>3 || a[1]>=(a[0]==0?palette_n():a[0]==2?3:2))rc=1;
        else {fm1_irq_off();if(a[0]<3)smk37_pref_set(a[0],a[1]);else favorites.filter=a[1];smk37_browser_fix();fm1_irq_on();rc=smk37_ed_ui_save();}
        smk37_ed_byte(rc);smk37_ed_byte(n?a[0]:127);smk37_ed_byte(n>1?a[1]:127);smk37_ed_ui_state();break;
    case 36:if(n)return 0;smk37_ed_byte(palette_n());for(unsigned i=0;i<palette_n();++i)smk37_ed_string(palette_name(i),12);break;
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

/* Upstream commands 80/81, same RGB565 wire format and save return codes. */
static unsigned smk37_editor_palette(unsigned cmd,const uint8_t *a,unsigned n){
    if((cmd==80 && n) || (cmd!=80 && cmd!=81))return 0;
    unsigned rc=1;uint16_t c[5];
    if(cmd==81){
        if(n==16 && a[0]<=1){rc=0;for(unsigned i=0;i<5;++i){if(a[1+3*i]>31 || a[2+3*i]>63 || a[3+3*i]>31)rc=1;c[i]=(uint16_t)(a[1+3*i]<<11|a[2+3*i]<<5|a[3+3*i]);}
            if(!rc){fm1_irq_off();if(!a[0]){memcpy(smk37_custom_preview,c,sizeof c);smk37_preview_palette=1;}else{cpal_store(c);ui_cpal_flags|=CPAL_ACTIVE;smk37_preview_palette=0;}++smk37_palette_generation;fm1_irq_on();if(a[0])rc=smk37_ed_ui_save();}}
        else if(n==1 && (a[0]==2 || a[0]==3)){fm1_irq_off();if(a[0]==2){memset(ui_cpal,0,10);ui_cpal_flags=0;}smk37_preview_palette=0;++smk37_palette_generation;fm1_irq_on();rc=a[0]==2?smk37_ed_ui_save():0;}
    }
    smk37_ed_begin(cmd);if(cmd==81)smk37_ed_byte(rc);
    smk37_ed_byte(ui_cpal_flags&(CPAL_STORED|CPAL_ACTIVE));smk37_ed_byte(5);
    const ui_pal_t *p=&UI_PALETTES[smk37_palette%UI_NPALETTES];
    uint16_t builtin[5]={p->bg,p->surf,p->text,p->theme,p->accent};
    for(unsigned i=0;i<5;++i){unsigned v=(ui_cpal_flags&CPAL_STORED)?ui_cpal[2*i]|ui_cpal[2*i+1]<<8:smk37_preview_palette?smk37_custom_preview[i]:builtin[i];smk37_ed_byte(v>>11);smk37_ed_byte((v>>5)&63);smk37_ed_byte(v&31);}
    return smk37_ed_finish();
}
