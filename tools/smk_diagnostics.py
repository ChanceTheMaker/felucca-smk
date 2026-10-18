"""SMKU v1 loader failure reports. Pure parsing; no device access."""
# SPDX-License-Identifier: GPL-3.0-only
REASONS = {
    1: 'Package header transfer failed', 2: 'Package header or device profile rejected',
    3: 'Protected boot or calibration validation failed',
    4: 'Package sector preflight failed', 5: 'Whole-package checksum failed',
    6: 'Package changed or transfer failed during programming',
    7: 'Application sector read failed', 8: 'Flash sector erase failed',
    9: 'Flash page programming failed', 10: 'Flash readback failed',
    11: 'Flash sector verification failed', 12: 'Application verification failed',
    13: 'Final acknowledgement failed', 14: 'Unsupported flash ID',
    15: 'Protected boot or calibration changed during verification',
}

def parse_status(packet):
    if not isinstance(packet, (bytes, bytearray)) or len(packet)<2 or packet[0]!=0xf0 or packet[-1]!=0xf7:
        return None
    raw=bytearray();acc=bits=0
    for value in packet[1:-1]:
        if value>127:return None
        acc|=value<<bits;bits+=7
        if bits>=8:raw.append(acc&255);acc>>=8;bits-=8
    if len(raw)!=19 or raw[:6]!=bytes([0,0x59,0x7e,12,0,0]) or raw[6:11]!=b'SMKU\x01':return None
    if (sum(raw[6:])&255)!=255 or raw[13]!=0 or raw[12]>1 or not 1<=raw[11]<=15:return None
    return dict(code=raw[11],writing=bool(raw[12]),flash_id=int.from_bytes(raw[14:18],'little'))

def status_error(status):
    reason=REASONS[status['code']]
    action=('Programming may have started. Keep power and USB connected; save this error. No automatic retry was made.'
            if status['writing'] else 'Stopped before any application erase or program. Save this error; do not bypass the device checks.')
    return f"SMK loader error {status['code']}: {reason}. Flash ID {status['flash_id']:06X}. {action}"
