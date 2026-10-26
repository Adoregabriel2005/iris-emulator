"""Run every ROM through the actual Qt frontend, in isolated child processes."""
import argparse
import concurrent.futures
import csv
import hashlib
import html
import json
import os
from pathlib import Path
import re
import subprocess
import time

from PIL import Image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, default=Path('build_interface/irisemulator.exe'))
    parser.add_argument('--roms', type=Path, default=Path('Atari Jaguar Rom Collection/ROMS'))
    parser.add_argument('--output', type=Path, default=Path('reports/jaguar-romset-2026-10-25'))
    parser.add_argument('--frames', type=int, default=600)
    parser.add_argument('--timeout', type=int, default=90)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--match', default='')
    parser.add_argument('--report-only', action='store_true')
    args = parser.parse_args()
    exe, output = args.exe.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    roms = sorted(p.resolve() for p in args.roms.glob('*') if p.suffix.lower() in ('.jag', '.j64', '.rom'))
    build_hash = hashlib.sha256(exe.read_bytes()).hexdigest()
    if os.name == 'nt':
        import ctypes
        ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)

    def run(item):
        index, rom = item
        slug = re.sub(r'[^a-zA-Z0-9_-]+', '-', rom.stem).strip('-')
        directory = output / f'{index:03d}-{slug}'
        directory.mkdir(exist_ok=True)
        started = time.monotonic()
        env = os.environ.copy()
        env['SDL_AUDIODRIVER'] = 'dummy'
        env['QT_LOGGING_RULES'] = '*.debug=false'
        command = [str(exe), '--smoke-test', str(rom), '--frames', str(args.frames), '--output', str(directory)]
        timed_out = False
        with (directory / 'frontend.log').open('wb') as log:
            child = subprocess.Popen(command, cwd=directory, env=env, stdout=log, stderr=log,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
            try:
                code = child.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                child.kill()
                code = child.wait()
        result_path = directory / 'result.json'
        result = json.loads(result_path.read_text(encoding='utf-8')) if result_path.exists() else {}
        status = result.get('status', 'no_result')
        if timed_out:
            status = 'timeout'
        elif code != 0 and status == 'completed':
            status = 'crash_on_shutdown'
        elif code != 0 and status != 'core_stopped':
            status = 'crash'
        elif code == 0 and status != 'completed':
            status = 'early_exit'
        if status == 'core_stopped' and result.get('reason', '').startswith('Failed to load ROM:'):
            status = 'load_rejected'
        images = sorted(directory.glob('frame-*.png'), key=lambda p: int(p.stem.split('-')[1]))
        colours, nonblack, width, height = 0, 0.0, 0, 0
        if images:
            with Image.open(images[-1]) as image:
                rgb = image.convert('RGB')
                width, height = rgb.size
                colours = len(rgb.getcolors(width * height) or [])
                nonblack = 100 * sum(max(p) > 8 for p in rgb.getdata()) / (width * height)
        snapshot = str(images[-1].relative_to(output)).replace('\\', '/') if images else ''
        row = {'rom': rom.name, 'status': status, 'frames': result.get('frames', 0),
            'exit_code': f'0x{code & 0xFFFFFFFF:08X}', 'seconds': round(time.monotonic() - started, 2),
            'colours': colours, 'nonblack_percent': round(nonblack, 2),
            'size': f'{width}x{height}', 'snapshot': snapshot,
            'reason': result.get('reason', ''), 'build_sha256': build_hash}
        (directory / 'audit.json').write_text(json.dumps(row, ensure_ascii=False, indent=2), encoding='utf-8')
        # Logs are retained per ROM, never mixed across concurrent processes.
        return row

    items = [(i, rom) for i, rom in enumerate(roms, 1) if args.match.lower() in rom.name.lower()]
    results = []
    if args.report_only:
        for audit in output.glob('*/audit.json'):
            row = json.loads(audit.read_text(encoding='utf-8'))
            if row['status'] == 'core_stopped' and row['reason'].startswith('Failed to load ROM:'):
                row['status'] = 'load_rejected'
                audit.write_text(json.dumps(row, ensure_ascii=False, indent=2), encoding='utf-8')
            results.append(row)
    else:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = [pool.submit(run, item) for item in items]
            for future in concurrent.futures.as_completed(futures):
                row = future.result()
                results.append(row)
                print(f'[{len(results)}/{len(items)}] {row["status"]}: {row["rom"]} ({row["frames"]} frames)', flush=True)
                (output / 'progress.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    results.sort(key=lambda row: row['rom'].lower())
    if not results:
        raise SystemExit('No ROMs matched')
    notes_path = output / 'review_notes.json'
    notes = json.loads(notes_path.read_text(encoding='utf-8')) if notes_path.exists() else {}
    for row in results:
        if not row['snapshot']:
            visual = 'Sem captura: não chegou ao primeiro quadro'
        elif row['nonblack_percent'] == 0:
            visual = 'Última captura preta; gameplay não validado'
        elif row['colours'] <= 2:
            visual = 'Última captura com cor sólida; gameplay não validado'
        else:
            visual = 'Imagem renderizada; gameplay não validado'
        row['visual'] = notes.get(row['rom'], visual)
        rom_path = args.roms / row['rom']
        row['rom_bytes'] = rom_path.stat().st_size
        with rom_path.open('rb') as rom_file:
            row['rom_header_hex'] = rom_file.read(16).hex()
    with (output / 'results.csv').open('w', encoding='utf-8-sig', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=list(results[0]))
        writer.writeheader()
        writer.writerows(results)
    counts = {}
    for row in results:
        counts[row['status']] = counts.get(row['status'], 0) + 1
    summary = {'counts': counts, 'tested': len(results), 'frames_per_rom': args.frames,
        'audio_tested': False, 'input': 'no controller input', 'bios': False, 'blitter': 'default',
        'build_sha256': build_hash, 'exe': str(exe), 'results': results}
    (output / 'results.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['# Teste do romset Jaguar — interface Qt', '',
        f'{len(results)} ROMs; {args.frames} quadros por ROM; som desligado; configurações isoladas; sem entrada de controle.', '',
        '**Completed significa somente que a interface completou o teste sem fechar. Não comprova gameplay, imagem correta ou compatibilidade plena.**', '',
        f'Resultados: {counts}', '',
        'Perfis: BIOS desligada, blitter padrão, GPU e DSP ligados, sem entrada de controle. Cada ROM roda em processo e diretório de configuração próprios.', '',
        '`load_rejected`: carregador recusou o arquivo antes da execução. `timeout`: processo excedeu o limite; não significa crash. `crash`: saída inesperada do processo. `core_stopped`: núcleo parou durante a execução.', '',
        '| ROM | Resultado | Quadros | Saída | Observação visual | Imagem |', '|---|---|---:|---|---|---|']
    cards = []
    for row in results:
        screenshot = row['snapshot']
        image_link = f'[captura]({screenshot})' if screenshot else 'sem captura'
        lines.append(f'| {row["rom"]} | {row["status"]} | {row["frames"]} | {row["exit_code"]} | {row["visual"]} | {image_link} |')
        image_tag = f'<img loading="lazy" src="{html.escape(screenshot)}">' if screenshot else '<p>Sem captura</p>'
        cards.append(f'<article><h3>{html.escape(row["rom"])}</h3><p>{row["status"]} · {row["frames"]} quadros · {row["exit_code"]}</p>{image_tag}<p>{html.escape(row["visual"])}</p><p>{html.escape(row["reason"])}</p></article>')
    (output / 'README.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    (output / 'gallery.html').write_text('<!doctype html><meta charset="utf-8"><title>Teste Jaguar</title><style>body{background:#10151b;color:#eee;font:16px system-ui;padding:24px}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(330px,1fr));gap:16px}article{background:#1d2732;padding:16px;border-radius:10px}img{width:100%;image-rendering:pixelated}h3{font-size:16px}</style><h1>Teste do romset Jaguar</h1><p>' + str(args.frames) + ' quadros de inicialização por ROM, sem áudio e sem controle. Completed não significa jogo plenamente funcional.</p><main>' + ''.join(cards) + '</main>', encoding='utf-8')
    print(json.dumps(counts), flush=True)


if __name__ == '__main__':
    main()
