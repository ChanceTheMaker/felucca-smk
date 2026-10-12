/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_midi_tx.h"
int main(void){
    smk37_midi_tx_t q={0};uint8_t b[1024];unsigned n=0;
    uint32_t packet[]={0x643c9009,0x0040b00b,0x4000e00e,0x0007c00c,0x0000fa0f,0x0000f80f,0x0000fb0f,0x0000fc0f,0x003c8008};
    for(unsigned i=0;i<sizeof packet/sizeof *packet;++i)assert(smk37_midi_tx_push(&q,packet[i])==1);
    while(smk37_midi_tx_next(&q,b+n))++n;
    const uint8_t want[]={0x90,60,100,0xb0,64,0,0xe0,0,64,0xc0,7,0xfa,0xf8,0xfb,0xfc,0x80,60,0};
    assert(n==sizeof want&&!memcmp(b,want,n));
    assert(!smk37_midi_tx_push(&q,0x007df004));assert(!smk37_midi_tx_push(&q,0xffffff09));
    /* Overrun with a partly transmitted note must finish then panic. */
    assert(smk37_midi_tx_push(&q,packet[0])==1);assert(smk37_midi_tx_next(&q,b)&&b[0]==0x90);
    for(unsigned i=0;i<SMK37_MIDI_TX_QUEUE;++i)assert(smk37_midi_tx_push(&q,packet[0])==1);
    assert(smk37_midi_tx_push(&q,packet[8])==-1);assert(q.recover);
    n=0;while(smk37_midi_tx_next(&q,b+n))++n;assert(n==149&&b[0]==60&&b[1]==100&&!q.recover);
    const uint8_t panic_cc[]={64,120,123};
    for(unsigned ch=0;ch<16;++ch)for(unsigned cc=0;cc<3;++cc){unsigned at=2+ch*9+cc*3;assert(b[at]==0xb0+ch&&b[at+1]==panic_cc[cc]&&!b[at+2]);}
    assert(!memcmp(b+146,(uint8_t[]){0x80,60,0},3));
    assert(smk37_midi_tx_push(&q,packet[0])==1);n=0;while(smk37_midi_tx_next(&q,b+n))++n;assert(n==3);
    /* Receiver deliberately ignores CC120/123. Held notes on different channels
     * still receive individual offs, including a partly drained message. */
    memset(&q,0,sizeof q);
    uint32_t notes[]={0x643c9009,0x64409209,0x647f9f09,0x00409209};
    for(unsigned i=0;i<4;i++)assert(smk37_midi_tx_push(&q,notes[i])==1);
    n=0;while(smk37_midi_tx_next(&q,b+n))++n;assert(n==12);
    /* Note 64 ch3 was released by velocity-zero; only 60 ch1 / 127 ch16 remain. */
    assert(smk37_midi_tx_push(&q,0x64489009)==1); /* queued, never transmitted */
    q.recover=1;n=0;while(smk37_midi_tx_next(&q,b+n))++n;
    assert(n==150&&!memcmp(b+144,(uint8_t[]){0x80,60,0,0x8f,127,0},6));
    for(unsigned ch=0;ch<16;ch++)for(unsigned i=0;i<16;i++)assert(!q.active[ch][i]);
    q.recover=1;n=0;while(smk37_midi_tx_next(&q,b+n))++n;assert(n==144);
    memset(&q,0,sizeof q);q.read=q.write=UINT32_MAX-2;
    for(unsigned i=0;i<10;++i)assert(smk37_midi_tx_push(&q,packet[0])==1);n=0;while(smk37_midi_tx_next(&q,b+n))++n;assert(n==30);
    puts("SMK MIDI TX: note/expression/transport bytes, SysEx isolation, overflow panic, partial message and index wrap passed");
}
