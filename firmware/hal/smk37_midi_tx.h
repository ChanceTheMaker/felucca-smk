/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Bounded USB-MIDI packet -> DIN byte queue. Audio is the only producer;
 * a lower-priority UART service is the only consumer. No pins or UART enabled.
 * Clock/transport use the same event stream, without running-status ambiguity.
 * Overflow drops pending events and sends sustain-off/all-sound-off/all-notes-off
 * on every channel, then explicit Note Off for notes actually transmitted before
 * accepting new notes. Some receivers ignore channel-mode controller messages. */
#define SMK37_MIDI_TX_QUEUE 128u
typedef struct {
    uint32_t event[SMK37_MIDI_TX_QUEUE];
    volatile uint32_t read,write,recover;
    uint32_t dropped,sent;
    uint8_t bytes[3],length,position,panic;
    uint8_t active[16][16]; /* transmitted notes, retained across dropped queue events */
    uint16_t release;
} smk37_midi_tx_t;
#define SMK37_TX_BARRIER() __asm__ volatile("" ::: "memory")
static unsigned smk37_midi_tx_decode(uint32_t packet,uint8_t bytes[3]){
    unsigned cin=packet&15u,s=(packet>>8)&255u;
    bytes[0]=s;bytes[1]=packet>>16;bytes[2]=packet>>24;
    if(cin>=8 && cin<=14 && (s>>4)==cin && bytes[1]<128 && ((cin==12||cin==13)||bytes[2]<128))return cin==12||cin==13?2:3;
    if(cin==15 && (s==0xf8||s==0xfa||s==0xfb||s==0xfc||s==0xfe||s==0xff))return 1;
    if(cin==3 && s==0xf2 && bytes[1]<128 && bytes[2]<128)return 3;
    if(cin==2 && (s==0xf1||s==0xf3) && bytes[1]<128)return 2;
    if(cin==5 && s==0xf6)return 1;
    return 0; /* editor/update SysEx is deliberately not bridged to TRS */
}
static int smk37_midi_tx_push(smk37_midi_tx_t *q,uint32_t packet){
    uint8_t b[3];if(!smk37_midi_tx_decode(packet,b))return 0;
    if(q->recover || q->write-q->read>=SMK37_MIDI_TX_QUEUE){++q->dropped;q->recover=1;return -1;}
    q->event[q->write%SMK37_MIDI_TX_QUEUE]=packet;SMK37_TX_BARRIER();++q->write;return 1;
}
/* Return the next byte only when the UART can accept it. An in-flight message
 * finishes before the recovery sequence; never splice a status into its data. */
static int smk37_midi_tx_next(smk37_midi_tx_t *q,uint8_t *byte){
    if(q->position==q->length){
        q->position=q->length=0;
        if(q->recover){
            q->read=q->write;
            if(q->panic<48){
                q->bytes[0]=0xb0+q->panic/3;q->bytes[1]=(const uint8_t[]){64,120,123}[q->panic%3];q->bytes[2]=0;
                q->length=3;++q->panic;
            }else{
                /* Never rely solely on CC120/123: release each transmitted note
                 * on its original channel. No queued-but-unsent note is counted. */
                while(q->release<2048u){
                    unsigned key=q->release++,ch=key/128u,note=key%128u;
                    if(!(q->active[ch][note/8u]&(1u<<(note%8u))))continue;
                    q->bytes[0]=0x80u+ch;q->bytes[1]=note;q->bytes[2]=0;q->length=3;break;
                }
                if(!q->length){q->panic=0;q->release=0;SMK37_TX_BARRIER();q->recover=0;return 0;}
            }
        }else{
            if(q->read==q->write)return 0;SMK37_TX_BARRIER();uint32_t p=q->event[q->read%SMK37_MIDI_TX_QUEUE];
            SMK37_TX_BARRIER();++q->read;q->length=smk37_midi_tx_decode(p,q->bytes);if(!q->length)return 0;
        }
    }
    *byte=q->bytes[q->position++];++q->sent;
    if(q->position==q->length && q->length==3){
        unsigned type=q->bytes[0]>>4,ch=q->bytes[0]&15u,note=q->bytes[1];
        if(type==8 || type==9){
            uint8_t bit=(uint8_t)(1u<<(note%8u));
            if(type==9 && q->bytes[2])q->active[ch][note/8u]|=bit;
            else q->active[ch][note/8u]&=(uint8_t)~bit;
        }
    }
    return 1;
}
