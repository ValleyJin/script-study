#!/usr/bin/env python3
"""깨진 입력을 만들어 sl 에 넣는다. study/03 의 "지나간 길만 본다" 절이 쓰는 도구다.

세 갈래로 만든다.
  1. 흐트러뜨리기 — tests/*.sl 을 잘라내고, 바이트를 바꾸고, 바이트를 끼운다.
     바꿔 넣는 바이트는 0x00 을 포함해 1..255 전부다. 0x00 을 빼면 프로그램이
     말없이 잘리는 길을 영원히 못 밟는다.
  2. 한도 겨누기 — 중첩 깊이, 토큰 개수, 이름 길이, 서로 다른 이름의 개수를
     한도 아래위로 바꿔 가며 짓는다. 289바이트짜리를 흐트러뜨려서는 한도 근처에
     갈 수 없다.
  3. 보통 파일이 아닌 입력 — 이름 있는 파이프와 /dev/stdin 이다.

쓰임:  python3 evidence/fuzz.py <실행파일> [씨앗]
정상 종료로 보는 코드는 0, 65, 70 이다. 그 밖이면 실패로 센다.
"""
import os, random, subprocess, sys, glob, tempfile, shutil

SEED = int(sys.argv[2]) if len(sys.argv) > 2 else 7
SL = sys.argv[1] if len(sys.argv) > 1 else './sl'
OK = {0, 65, 70}

def perturb(sources, rng, n_each=60):
    for base in sources:
        for _ in range(n_each):
            b = bytearray(base)
            m = rng.random()
            if m < 0.4 and len(b) > 1:
                b = b[:rng.randrange(1, len(b))]
            elif m < 0.8 and len(b) > 0:
                b[rng.randrange(len(b))] = rng.randrange(0, 256)
            else:
                i = rng.randrange(len(b) + 1)
                b[i:i] = bytes([rng.randrange(0, 256)])
            yield bytes(b)

def limits():
    """한도 아래위를 겨눈다. study/02 의 상한은 깊이 128, 지역 255, 인자 255다."""
    for n in (1, 127, 128, 129, 300, 5000):
        yield f"print {'('*n}1{')'*n};\n".encode()
        yield f"print {'-'*n}1;\n".encode()
        yield ("print " + " + ".join(["1"]*n) + ";\n").encode()
    for n in (254, 255, 256, 300):
        body = "".join(f"  var v{i} = {i};\n" for i in range(n))
        yield f"fun f() {{\n{body}}}\n".encode()
        yield ("fun g() { return 1; }\ng(" + ", ".join(["1"]*n) + ");\n").encode()
    for n in (1, 100, 10000):
        yield f"var {'a'*n} = 1;\nprint {'a'*n};\n".encode()
    for n in (1, 1000):
        yield "".join(f"var n{i} = {i};\n" for i in range(n)).encode()
    for n in (1, 64, 128):
        yield ("fun f() {\n" * n + "return 1;\n" + "}\n" * n).encode()

def run(path, mode):
    try:
        r = subprocess.run([SL, mode, path], capture_output=True, timeout=10)
        return r.returncode, r.stderr.decode('utf-8', 'replace')
    except subprocess.TimeoutExpired:
        return -1, '시간을 넘겼다'

def main():
    rng = random.Random(SEED)
    sources = [open(p, 'rb').read() for p in sorted(glob.glob('tests/*.sl'))]
    tmp = tempfile.mkdtemp()
    fails = total = 0
    try:
        inputs = list(perturb(sources, rng)) + list(limits())
        for i, data in enumerate(inputs):
            path = os.path.join(tmp, f"{i:05d}.sl")
            open(path, 'wb').write(data)
            for mode in ('--print', '--check', '--walk'):
                total += 1
                code, err = run(path, mode)
                if code not in OK:
                    fails += 1
                    print(f"!! {mode} {path} 종료 코드 {code}")
                    print("   " + data[:80].decode('utf-8', 'replace').replace('\n', '\\n'))
                    if err.strip():
                        print("   " + err.strip().split('\n')[0])
                    if fails >= 5:
                        break
            if fails >= 5:
                break

        # 보통 파일이 아닌 입력
        fifo = os.path.join(tmp, 'fifo')
        os.mkfifo(fifo)
        if os.fork() == 0:
            open(fifo, 'w').write('print 1;\n')
            os._exit(0)
        total += 1
        code, _ = run(fifo, '--walk')
        if code not in OK:
            fails += 1
            print(f"!! 이름 있는 파이프 종료 코드 {code}")
        os.wait()

        total += 1
        r = subprocess.run([SL, '--walk', '/dev/stdin'], input=b'print 1;\n',
                           capture_output=True, timeout=10)
        if r.returncode not in OK:
            fails += 1
            print(f"!! /dev/stdin 종료 코드 {r.returncode}")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print(f"입력 {len(inputs)}개, 실행 {total}번, 실패 {fails}건 (씨앗 {SEED})")
    return 1 if fails else 0

if __name__ == '__main__':
    sys.exit(main())
