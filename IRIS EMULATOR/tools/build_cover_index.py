#!/usr/bin/env python3
"""
build_cover_index.py

Scans a ROM folder recursively, computes SHA1 for each ROM, and builds
an index file `covers.json` in the covers output directory pointing
each ROM SHA1 to a saved cover image if present.

Usage:
  python tools/build_cover_index.py --roms D:/path/to/roms --out %USERPROFILE%/.ataricovers
"""

import argparse
import os
import hashlib
import json

def sha1_of_file(path):
    h = hashlib.sha1()
    with open(path, 'rb') as f:
        while True:
            data = f.read(8192)
            if not data:
                break
            h.update(data)
    return h.hexdigest()

def normalize_name(name):
    name = os.path.splitext(os.path.basename(name))[0]
    name = name.replace('_', ' ').replace('-', ' ').replace('+', ' ')
    name = ' '.join(name.split())
    name = name.strip()
    return name

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--roms', '-r', required=True, help='ROMs root folder')
    parser.add_argument('--out', '-o', default=os.path.expanduser('~/.ataricovers'), help='Covers folder')
    args = parser.parse_args()
    roms_root = args.roms
    outdir = args.out
    exts = ('.bin', '.rom', '.a26', '.zip', '.7z')

    if not os.path.isdir(roms_root):
        print('ROMs folder not found:', roms_root)
        return
    os.makedirs(outdir, exist_ok=True)

    covers = {}
    roms = []
    for root, dirs, files in os.walk(roms_root):
        for f in files:
            if f.lower().endswith(exts):
                roms.append(os.path.join(root, f))

    print('Found', len(roms), 'ROMs. Building index...')
    for rom in roms:
        try:
            base = os.path.splitext(os.path.basename(rom))[0]
            sha1 = sha1_of_file(rom)
            title = normalize_name(base)
            # prefer sha1-named image
            img = None
            cand1 = os.path.join(outdir, sha1 + '.jpg')
            cand2 = os.path.join(outdir, base + '.jpg')
            cand3 = os.path.join(outdir, base + '.png')
            if os.path.exists(cand1):
                img = cand1
            elif os.path.exists(cand2):
                img = cand2
            elif os.path.exists(cand3):
                img = cand3
            covers[sha1] = {'base': base, 'title': title, 'image': img}
        except Exception as e:
            print('Error', rom, e)

    outjson = os.path.join(outdir, 'covers.json')
    with open(outjson, 'w', encoding='utf-8') as f:
        json.dump(covers, f, indent=2, ensure_ascii=False)
    import argparse
    import hashlib
    import json
    import os
    from pathlib import Path
    from PIL import Image


    def compute_sha1(path: Path) -> str:
        h = hashlib.sha1()
        with path.open("rb") as f:
            while True:
                data = f.read(8192)
                if not data:
                    break
                h.update(data)
        return h.hexdigest()


    def build_index(roms_dir: Path, covers_dir: Path) -> dict:
        index = {}
        for root, dirs, files in os.walk(roms_dir):
            for fname in files:
                if not fname.lower().endswith(('.bin', '.rom', '.a26', '.zip')):
                    continue
                rompath = Path(root) / fname
                try:
                    sha1 = compute_sha1(rompath)
                except Exception:
                    continue
                base = Path(fname).stem
                # prefer sha1-based image
                found = False
                for ext in ('.jpg', '.png', '.jpeg'):
                    p = covers_dir / (sha1 + ext)
                    if p.exists():
                        index[sha1] = {"image": str(p.resolve()), "title": base}
                        found = True
                        break
                if found:
                    continue

                for ext in ('.jpg', '.png', '.jpeg'):
                    p = covers_dir / (base + ext)
                    if p.exists():
                        index[sha1] = {"image": str(p.resolve()), "title": base}
                        found = True
                        break

        return index


    if __name__ == '__main__':
        parser = argparse.ArgumentParser()
        parser.add_argument('--roms', required=True, help='Path to ROMs folder to scan')
        parser.add_argument('--out', required=True, help='Covers directory where images are stored')
        args = parser.parse_args()

        roms = Path(args.roms)
        out = Path(args.out)
        out.mkdir(parents=True, exist_ok=True)
        idx = build_index(roms, out)
        outFile = out / 'covers.json'
        with outFile.open('w', encoding='utf-8') as f:
            json.dump(idx, f, indent=2, ensure_ascii=False)
        print(f"Wrote {len(idx)} entries to {outFile}")
