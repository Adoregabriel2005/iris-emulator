#!/usr/bin/env python3
"""
fetch_boxarts.py

Download box art images for Atari 2600 ROMs in a folder.
Saves images into output directory as <basename>.jpg and <sha1>.jpg.

Usage:
  python tools/fetch_boxarts.py --folder /path/to/roms --out ~/.ataricovers

Dependencies: requests, beautifulsoup4, pillow
"""

import argparse
import os
import re
import requests
import hashlib
import time
import json
from bs4 import BeautifulSoup
from PIL import Image
from io import BytesIO

HEADERS = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0 Safari/537.36'}


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
    name = re.sub(r'\s+', ' ', name)
    name = re.sub(r'[\[\]\(\)\{\}]', '', name)
    name = name.strip()
    return name


def ddg_search_images(query, max_results=6):
    s = requests.Session()
    try:
        r = s.get('https://duckduckgo.com/', params={'q': query}, headers=HEADERS, timeout=10)
    except Exception:
        return []

    m = re.search(r"vqd='(?P<vqd>[^']+)'", r.text)
    if not m:
        return []
    vqd = m.group('vqd')

    api = 'https://duckduckgo.com/i.js'
    params = {'q': query, 'vqd': vqd, 'o': 'json', 'p': '1', 's': '0'}
    imgs = []
    while True:
        try:
            res = s.get(api, params=params, headers={**HEADERS, 'Referer': 'https://duckduckgo.com/'}, timeout=10)
            data = res.json()
            results = data.get('results', [])
            for r in results:
                url = r.get('image') or r.get('thumbnail') or r.get('url')
                if url:
                    imgs.append(url)
                    if len(imgs) >= max_results:
                        return imgs
            if 'next' in data:
                api = 'https://duckduckgo.com' + data['next']
                time.sleep(0.2)
            else:
                break
        except Exception:
            break
    return imgs


def bing_search_images(query, max_results=8):
    s = requests.Session()
    url = 'https://www.bing.com/images/search'
    try:
        r = s.get(url, params={'q': query}, headers=HEADERS, timeout=10)
    except Exception:
        return []
    soup = BeautifulSoup(r.text, 'html.parser')
    imgs = []
    for a in soup.find_all('a', {'class': 'iusc'}):
        m = a.get('m')
        if not m:
            continue
        try:
            obj = json.loads(m)
            murl = obj.get('murl')
            if murl:
                imgs.append(murl)
                if len(imgs) >= max_results:
                    break
        except Exception:
            continue
    return imgs


def download_image(url):
    try:
        r = requests.get(url, headers=HEADERS, timeout=12)
        if r.status_code != 200:
            return None
        img = Image.open(BytesIO(r.content))
        img = img.convert('RGB')
        return img
    except Exception:
        return None


def save_image(img, outpath, max_size=800):
    w, h = img.size
    if max(w, h) > max_size:
        img.thumbnail((max_size, max_size), Image.LANCZOS)
    img.save(outpath, format='JPEG', quality=90)


def find_and_save_cover(rompath, outdir):
    base = os.path.splitext(os.path.basename(rompath))[0]
    sha1 = sha1_of_file(rompath)
    name = normalize_name(base)
    outbase1 = os.path.join(outdir, base + '.jpg')
    outsha = os.path.join(outdir, sha1 + '.jpg')
    if os.path.exists(outbase1) or os.path.exists(outsha):
        print(f"Cover already exists for {base}")
        return True

    queries = [
        f"Atari 2600 {name} box art",
        f"{name} Atari 2600 cover",
        f"{name} box art",
        f"{name} cover art",
    ]

    for q in queries:
        print(f"Searching '{q}' via DuckDuckGo...")
        urls = ddg_search_images(q, max_results=6)
        for u in urls:
            img = download_image(u)
            if img and min(img.size) >= 100:
                try:
                    os.makedirs(outdir, exist_ok=True)
                    save_image(img, outbase1)
                    save_image(img, outsha)
                    print(f"Saved cover for {base}")
                    return True
                except Exception as e:
                    print("Failed to save:", e)
        print("DDG failed or no good results, trying Bing...")
        urls = bing_search_images(q, max_results=8)
        for u in urls:
            img = download_image(u)
            if img and min(img.size) >= 100:
                try:
                    os.makedirs(outdir, exist_ok=True)
                    save_image(img, outbase1)
                    save_image(img, outsha)
                    print(f"Saved cover for {base}")
                    return True
                except Exception as e:
                    print("Failed to save:", e)

    print(f"No cover found for {base}")
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--folder', '-f', required=True, help='Path to ROM folder')
    parser.add_argument('--out', '-o', default=os.path.expanduser('~/.ataricovers'), help='Output directory for covers')
    args = parser.parse_args()
    folder = args.folder
    outdir = args.out
    exts = ('.bin', '.rom', '.a26', '.zip', '.7z')
    # search recursively for ROM files
    roms = []
    if not os.path.isdir(folder):
        print('Folder not found or unreadable:', folder)
        return
    for root, dirs, files in os.walk(folder):
        for f in files:
            if f.lower().endswith(exts):
                roms.append(os.path.join(root, f))
    if not roms:
        print('No ROM files found in folder', folder)
        return
    print(f'Found {len(roms)} ROM(s) under {folder}')
    for idx, rom in enumerate(roms, 1):
        try:
            print(f'[{idx}/{len(roms)}] Processing: {os.path.basename(rom)}')
            find_and_save_cover(rom, outdir)
            time.sleep(0.2)
        except Exception as e:
            print('Error handling', rom, e)
    print('Done.')


if __name__ == '__main__':
    main()
