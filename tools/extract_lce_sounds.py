#!/usr/bin/env python3
"""Pulls sounds out of the Console Edition's resident.xwb into the .pcm C arrays
client/sound/data holds (header: channels, bytes per sample, rate, frames).

The wave bank is an Xbox 360 XACT3 file (big-endian, version 46) whose entries are
raw XMA2 streams. Each one is wrapped in a RIFF/WAVE header, which is the form
ffmpeg's xma2 decoder reads, and decoded to mono 16-bit at RATE.

    extract_lce_sounds.py <resident.xwb> <out dir> [name ...]

With no names it extracts the ones listed in WANTED. Prints the #include,
SoundDesc and sounds.add lines to paste into Sound.cpp / SoundEngine.cpp.
"""
import struct, subprocess, sys, os, tempfile

RATE = 22050   # effects only; half the size of 44100 and no audible loss

# bank entry name -> (our file name, registry key)
def group(prefix, key, names):
    return [(n, n.replace('mob_', '').replace('_', ''), key) for n in names]

WANTED = []
for i in range(1, 8): WANTED.append(('mob_ghast_moan%d' % i, 'ghastmoan%d' % i, 'mob.ghast.moan'))
for i in range(1, 6): WANTED.append(('mob_ghast_scream%d' % i, 'ghastscream%d' % i, 'mob.ghast.scream'))
WANTED += [('mob_ghast_death', 'ghastdeath', 'mob.ghast.death'),
           ('mob_ghast_fireball4', 'ghastfireball', 'mob.ghast.fireball'),
           ('mob_ghast_charge', 'ghastcharge', 'mob.ghast.charge')]
for i in range(1, 5): WANTED.append(('mob_blaze_breathe%d' % i, 'blazebreathe%d' % i, 'mob.blaze.breathe'))
for i in range(1, 5): WANTED.append(('mob_blaze_hit%d' % i, 'blazehit%d' % i, 'mob.blaze.hit'))
WANTED.append(('mob_blaze_death', 'blazedeath', 'mob.blaze.death'))
for i in range(1, 5): WANTED.append(('mob_magmacube_big%d' % i, 'magmacubebig%d' % i, 'mob.magmacube.big'))
for i in range(1, 5): WANTED.append(('mob_magmacube_jump%d' % i, 'magmacubejump%d' % i, 'mob.magmacube.jump'))
for i in range(1, 6): WANTED.append(('mob_magmacube_small%d' % i, 'magmacubesmall%d' % i, 'mob.magmacube.small'))

def main():
    src, out = sys.argv[1], sys.argv[2]
    d = open(src, 'rb').read()
    segs = [struct.unpack('>II', d[12 + 8 * i:20 + 8 * i]) for i in range(5)]
    n = struct.unpack('>I', d[segs[0][0] + 4:segs[0][0] + 8])[0]
    names = [d[segs[3][0] + 64 * i:segs[3][0] + 64 * (i + 1)].split(b'\0')[0].decode('latin1') for i in range(n)]
    index = dict((nm, i) for i, nm in enumerate(names))

    inc, desc, add = [], [], []
    for bank_name, file_name, key in WANTED:
        i = index[bank_name]
        m = struct.unpack('>6I', d[segs[1][0] + 24 * i:segs[1][0] + 24 * (i + 1)])
        duration, fmt, off, length = m[0] >> 4, m[1], m[2], m[3]
        rate, ch = (fmt >> 5) & 0x3ffff, (fmt >> 2) & 7
        raw = d[segs[4][0] + off:segs[4][0] + off + length]
        blocks = max(1, len(raw) // 2048)
        ext = struct.pack('<HIIIIIIIBBH', 1, 1, duration, 2048, 0, duration, 0, 0, 0, 4, blocks)
        f = struct.pack('<HHIIHHH', 0x166, ch, rate, rate * 2, 2 * ch, 16, len(ext)) + ext
        body = b'WAVE' + b'fmt ' + struct.pack('<I', len(f)) + f + b'data' + struct.pack('<I', len(raw)) + raw
        wav = b'RIFF' + struct.pack('<I', len(body)) + body
        with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as t:
            t.write(wav)
        pcm = subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', t.name, '-ar', str(RATE), '-ac', '1', '-f', 's16le', '-'],
                             capture_output=True, check=True).stdout
        os.unlink(t.name)
        frames = len(pcm) // 2
        header = struct.pack('<4i', 1, 2, RATE, frames)
        data = header + pcm
        lines = []
        for k in range(0, len(data), 32):
            lines.append(','.join(str(b) for b in data[k:k + 32]))
        with open(os.path.join(out, file_name + '.pcm'), 'w') as o:
            o.write('unsigned char PCM_%s[%d] = {\n%s};\n' % (file_name, len(data), ',\n'.join(lines)))
        inc.append('#include "data/%s.pcm"' % file_name)
        desc.append('SoundDesc SA_%s((char*)PCM_%s);' % (file_name, file_name))
        add.append('\tsounds.add("%s", SA_%s);' % (key, file_name))
        print('%-24s %6d frames' % (bank_name, frames), file=sys.stderr)
    print('\n'.join(inc)); print(); print('\n'.join(desc)); print(); print('\n'.join(add))

main()
