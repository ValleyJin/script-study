#!/bin/sh
# 1부의 단계별 "끝났다고 보는 조건"을 기계로 판정한다.
#   1단계 왕복 검사     — 프린터와 파서가 서로 맞는지 본다
#   1단계 기대 트리 검사 — 파서가 문법과 맞는지 본다. 기대 트리는 손으로 적었다
#   1단계 오류 검사     — bad-lex, bad-check 가 정한 줄 번호와 문구와 종료 코드 65 를 낸다
#   2단계 출력 검사     — tests/ 와 bench/ 가 expected/ 와 같은 출력을 낸다
#   2단계 런타임 오류   — bad-run 이 정한 문구와 종료 코드 70 을 낸다
# 3단계부터 --vm 을 같은 조건에 넣고, 4단계에서 두 갈래를 글자까지 견준다.

set -u
SL=./sl
pass=0
fail=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

ok()  { pass=$((pass+1)); }
bad() { fail=$((fail+1)); printf '  실패: %s\n' "$1"; }

# 2단계부터 트리 순회가 붙었다. 3단계에서 vm 을 더한다.
BRANCHES="walk"

printf '1단계. 왕복 검사\n'
for f in tests/*.sl; do
  if ! "$SL" --print "$f" > "$tmp/a" 2> "$tmp/err"; then
    bad "$f 를 찍지 못했다: $(cat "$tmp/err")"; continue
  fi
  if ! "$SL" --print "$tmp/a" > "$tmp/b" 2> "$tmp/err"; then
    bad "$f 의 출력을 다시 파싱하지 못했다: $(cat "$tmp/err")"; continue
  fi
  if cmp -s "$tmp/a" "$tmp/b"; then ok; else
    bad "$f 왕복이 어긋났다"; diff "$tmp/a" "$tmp/b" | head -6 | sed 's/^/    /'
  fi
done

printf '1단계. 기대 트리 검사\n'
for want in tests/expected-ast/*.txt; do
  name=$(basename "$want" .txt)
  src="tests/$name.sl"
  if [ ! -f "$src" ]; then bad "$src 가 없다"; continue; fi
  if ! "$SL" --print "$src" > "$tmp/got" 2> "$tmp/err"; then
    bad "$src 를 찍지 못했다: $(cat "$tmp/err")"; continue
  fi
  if cmp -s "$tmp/got" "$want"; then ok; else
    bad "$name 의 트리가 손으로 적은 것과 다르다"
    diff "$want" "$tmp/got" | head -10 | sed 's/^/    /'
  fi
done

printf '1단계. 어휘·구문·검사 오류\n'
for dir in tests/bad-lex tests/bad-check; do
  for f in "$dir"/*.sl; do
    want="${f%.sl}.expected"
    [ -f "$want" ] || { bad "$want 가 없다"; continue; }
    "$SL" --check "$f" > "$tmp/out" 2> "$tmp/got"
    code=$?
    if [ "$code" -ne 65 ]; then bad "$f 의 종료 코드가 65가 아니라 $code 다"; continue; fi
    if [ -s "$tmp/out" ]; then bad "$f 가 표준 출력에 무언가를 냈다"; continue; fi
    if cmp -s "$tmp/got" "$want"; then ok; else
      bad "$f 의 문구가 다르다"
      printf '    기대: %s\n    받음: %s\n' "$(cat "$want")" "$(cat "$tmp/got")"
    fi
  done
done

for br in $BRANCHES; do
  printf '2단계. 출력 검사 (--%s)\n' "$br"
  for want in tests/expected/*.txt; do
    name=$(basename "$want" .txt)
    src="tests/$name.sl"
    [ -f "$src" ] || { bad "$src 가 없다"; continue; }
    "$SL" "--$br" "$src" > "$tmp/got" 2> "$tmp/err"
    code=$?
    if [ "$code" -ne 0 ]; then
      bad "$src --$br 종료 코드 $code: $(cat "$tmp/err")"; continue
    fi
    if cmp -s "$tmp/got" "$want"; then ok; else
      bad "$name --$br 출력이 expected/ 와 다르다"
      diff "$want" "$tmp/got" | head -8 | sed 's/^/    /'
    fi
  done

  printf '2단계. 벤치마크 출력 (--%s)\n' "$br"
  for want in bench/expected/*.txt; do
    name=$(basename "$want" .txt)
    src="bench/$name.sl"
    [ -f "$src" ] || { bad "$src 가 없다"; continue; }
    "$SL" "--$br" "$src" > "$tmp/got" 2> "$tmp/err"
    if cmp -s "$tmp/got" "$want"; then ok; else
      bad "$name --$br 벤치마크 출력이 다르다"
      diff "$want" "$tmp/got" | head -6 | sed 's/^/    /'
    fi
  done

  printf '2단계. 런타임 오류 (--%s)\n' "$br"
  for f in tests/bad-run/*.sl; do
    want="${f%.sl}.expected"
    [ -f "$want" ] || { bad "$want 가 없다"; continue; }
    "$SL" "--$br" "$f" > "$tmp/out" 2> "$tmp/got"
    code=$?
    if [ "$code" -ne 70 ]; then bad "$f 의 종료 코드가 70이 아니라 $code 다"; continue; fi
    if cmp -s "$tmp/got" "$want"; then ok; else
      bad "$f 의 문구가 다르다"
      printf '    기대: %s\n    받음: %s\n' "$(cat "$want")" "$(cat "$tmp/got")"
    fi
  done
done

printf '\n통과 %d, 실패 %d\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
