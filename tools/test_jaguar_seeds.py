"""Repeat a Qt ROM boot with explicit initial RAM seeds."""
import argparse
import concurrent.futures
import json
import hashlib
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('rom', type=Path)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--seeds', type=int, default=16)
parser.add_argument('--seed-base', type=int, default=0)
parser.add_argument('--seed-values', help='Comma-separated explicit seeds')
parser.add_argument('--frames', type=int, default=600)
parser.add_argument('--timeout', type=int, default=25)
parser.add_argument('--input-script', type=Path, help='Frame ranges and Jaguar buttons for the Qt smoke test')
args = parser.parse_args()
exe = Path('build_interface/irisemulator.exe').resolve()
build_hash = hashlib.sha256(exe.read_bytes()).hexdigest()
rom = args.rom.resolve()
root = args.output.resolve()
root.mkdir(parents=True, exist_ok=True)

def run(seed):
    directory = root / str(seed)
    directory.mkdir(exist_ok=True)
    env = dict(os.environ, IRIS_JAGUAR_RAM_SEED=str(seed), SDL_AUDIODRIVER='dummy', QT_LOGGING_RULES='*.debug=false')
    with (directory / 'frontend.log').open('wb') as log:
        command = [str(exe), '--smoke-test', str(rom), '--frames', str(args.frames), '--output', str(directory)]
        if args.input_script:
            command += ['--input-script', str(args.input_script.resolve())]
        child = subprocess.Popen(command,
            cwd=directory, env=env, stdout=log, stderr=log, creationflags=0x08000000)
        try:
            code = child.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
            code = 'timeout'
    result_path = directory / 'result.json'
    result = json.loads(result_path.read_text(encoding='utf-8')) if result_path.exists() else {}
    row = {'seed': seed, 'exit': code, **result}
    row['buildSha256'] = build_hash
    if code == 'timeout': row['status'] = 'timeout'
    print(json.dumps({'seed':seed, 'exit':code, 'frames':result.get('frames'), 'status':result.get('status')}), flush=True)
    return row

seed_values = [int(value, 0) for value in args.seed_values.split(',')] if args.seed_values else range(args.seed_base, args.seed_base + args.seeds)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    results = list(pool.map(run, seed_values))
(root / 'matrix.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
