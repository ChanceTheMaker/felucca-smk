/* SPDX-License-Identifier: GPL-3.0-only
 * MENU_DESC/SET format from editor_menu.c, Copyright (C) 2026
 * Leo Kuroshita (@kurogedelic), Hugelton Instruments. */
static const struct {uint8_t id,row,tab;} smk37_menu[]={
 {0,0,0},{1,18,0},{2,19,0},{3,10,0},{5,5,1},{6,3,1},{7,11,1},{8,4,1},
 {10,13,2},{12,14,2},{13,15,2},{14,16,2},{15,17,4},{17,7,0},{18,12,0},{23,6,0},{24,8,4},{19,20,1},{22,255,2},{9,21,2},{21,255,3},{4,9,0},{26,255,3},{27,255,3},{28,255,3},{29,255,3}
};
#define SMK37_MENU_COUNT (sizeof smk37_menu/sizeof smk37_menu[0])
static unsigned smk37_menu_value(unsigned i){if(smk37_menu[i].id>=26 && smk37_menu[i].id<=29)return mch_get(smk37_menu[i].id-26);if(smk37_menu[i].id==21)return song.g[G_ROUTE];if(smk37_menu[i].id==22)return (unsigned)(int)song.g[G_TUNE];unsigned v=smk37_pref_get(smk37_menu[i].row);return smk37_menu[i].id==3 || smk37_menu[i].id==23 ? !v:v;}
static unsigned smk37_editor_menu(unsigned cmd,const uint8_t *a,unsigned n){
    if(n!=(cmd==72?1u:3u))return 0;
    smk37_ed_begin(cmd);
    if(cmd==72){
        unsigned i=a[0];smk37_ed_byte(i);if(i>=SMK37_MENU_COUNT){smk37_ed_byte(127);return smk37_ed_finish();}
        unsigned row=smk37_menu[i].row,max=smk37_pref_max(row);
        smk37_ed_byte(smk37_menu[i].id);
        if(smk37_menu[i].id==21){
            const param_desc_t *d=&GP[G_ROUTE];
            smk37_ed_byte(0);smk37_ed_value(song.g[G_ROUTE]);smk37_ed_value(d->min);smk37_ed_value(d->max);smk37_ed_string("MIDI IN",12);
            for(int v=d->min;v<=d->max;++v)smk37_ed_string(d->names[v-d->min],12);
            smk37_ed_byte(3);smk37_ed_string("MIDI",12);return smk37_ed_finish();
        }
        if(smk37_menu[i].id>=26 && smk37_menu[i].id<=29){
            unsigned k=smk37_menu[i].id-26;char name[16]="T1 CHANNEL";name[1]+=(char)k;
            smk37_ed_byte(0);smk37_ed_value(mch_get(k));smk37_ed_value(0);smk37_ed_value(MCH_N-1);smk37_ed_string(name,12);
            for(unsigned v=0;v<MCH_N;++v){if(v<16){fmt_int(name,v+1);smk37_ed_string(name,12);}else smk37_ed_string(v==MCH_OMNI?"OMNI":"OFF",12);}
            smk37_ed_byte(3);smk37_ed_string("MIDI",12);return smk37_ed_finish();
        }
        if(smk37_menu[i].id==22){
            smk37_ed_byte(1);smk37_ed_value(song.g[G_TUNE]);smk37_ed_value(-50);smk37_ed_value(50);
            smk37_ed_string("TUNE",12);smk37_ed_string("ct",12);smk37_ed_byte(2);smk37_ed_string("AUDIO",12);return smk37_ed_finish();
        }
        smk37_ed_byte(0);smk37_ed_value(smk37_menu_value(i));smk37_ed_value(0);smk37_ed_value(max);
        smk37_ed_string(smk37_pref_names[row],12);
        /* Describe choices without applying settings as a side effect. */
        for(unsigned v=0;v<=max;++v){const char *s=v?"ON":"OFF";
            if(row==0)s=palette_name(v);
            if(row==18)s=v?"LINE":"FLAT";
            if(row==21)s=(const char *const[]){"FLAT","LOWCUT","BASS+"}[v];
            if(row==9)s=(const char *const[]){"OFF","1","2","3","4","5","6","7","8"}[v];
            if(row==10)s=v?"OFF":"ON";
            if(row==5)s=(const char *const[]){"300 ms","400 ms","500 ms","600 ms"}[v];
            if(row==6)s=v?"TRACKS":"SCOPE";
            if(row==7)s=(const char *const[]){"NEVER","5 MIN","15 MIN","30 MIN","60 MIN"}[v];
            if(row==12)s=v?"MIX":"OUT";
            if(row==13)s=v?"FIXED":"MASTER";
            if(row==14)s=(const char *const[]){"OFF","REC","ON"}[v];
            if(row==15)s=(const char *const[]){"LOW","MID","HIGH"}[v];
            if(row==16)s=(const char *const[]){"OFF","1 BAR","2 BARS"}[v];
            smk37_ed_string(s,12);
        }
        smk37_ed_byte(smk37_menu[i].tab);smk37_ed_string((const char *const[]){"DISPLAY","CONTROL","AUDIO","MIDI","SYSTEM"}[smk37_menu[i].tab],12);
    }else{
        unsigned i;int v=smk37_ed_read(a+1);for(i=0;i<SMK37_MENU_COUNT && smk37_menu[i].id!=a[0];++i);
        unsigned rc=1;
        if(i<SMK37_MENU_COUNT && a[0]>=26 && a[0]<=29){v=clamp(v,0,MCH_N-1);fm1_irq_off();mch_set(a[0]-26,v);midi_route=MIDI_ROUTE_AGAIN;fm1_irq_on();rc=0;}
        else if(i<SMK37_MENU_COUNT && a[0]==21){v=clamp(v,GP[G_ROUTE].min,GP[G_ROUTE].max);fm1_irq_off();song.g[G_ROUTE]=v;fm1_irq_on();rc=0;}
        else if(i<SMK37_MENU_COUNT && a[0]==22){v=clamp(v,-50,50);fm1_irq_off();song.g[G_TUNE]=v;fm1_irq_on();rc=0;}
        else if(i<SMK37_MENU_COUNT){v=clamp(v,0,smk37_pref_max(smk37_menu[i].row));
            fm1_irq_off();smk37_pref_set(smk37_menu[i].row,a[0]==3 || a[0]==23 ? !v:v);fm1_irq_on();rc=smk37_ed_ui_save();}
        smk37_ed_byte(rc);smk37_ed_byte(a[0]);smk37_ed_value(v);
    }
    return smk37_ed_finish();
}
