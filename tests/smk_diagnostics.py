"""Checksummed loader diagnostics; no hardware access."""
import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from smk_diagnostics import parse_status, status_error

def packet(code=14,writing=0,flash=0xeb6014):
    body=bytearray(b'SMKU\x01')+bytes([code,writing,0])+flash.to_bytes(4,'little')
    raw=bytes([0,0x59,0x7e,12,0,0])+body+bytes([~sum(body)&255])
    out=bytearray([0xf0]);acc=bits=0
    for value in raw:
        acc|=value<<bits;bits+=8
        while bits>=7:out.append(acc&127);acc>>=7;bits-=7
    if bits:out.append(acc&127)
    out.append(0xf7);return out

class DiagnosticsTests(unittest.TestCase):
    def test_prewrite_rejection(self):
        status=parse_status(packet())
        self.assertEqual(status,dict(code=14,writing=False,flash_id=0xeb6014))
        self.assertIn('EB6014',status_error(status))
        self.assertIn('before any application erase',status_error(status))
    def test_write_failure_preserves_power_instruction(self):
        status=parse_status(packet(9,1,0x856014))
        self.assertIn('Keep power and USB connected',status_error(status))
        self.assertNotIn('before any',status_error(status))
    def test_bad_checksum_and_truncation_ignored(self):
        p=packet();p[8]^=1
        self.assertIsNone(parse_status(p))
        for n in range(len(packet())):self.assertIsNone(parse_status(packet()[:n]))
    def test_unrelated_or_unsupported_messages_ignored(self):
        for p in [b'\xf0\x01\xf7',packet(0),packet(16),packet(14,2),None,(0,1,2)]:
            self.assertIsNone(parse_status(p))

if __name__=='__main__':unittest.main()
