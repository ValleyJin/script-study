# 이 기계에서 메모리 오류를 어떻게 검증하는가

2026-10-02, macOS 26.7, Apple clang version 17.0.0 (clang-1700.6.3.2).

## AddressSanitizer가 이 기계에서 돌지 않는다

`hello world`조차 끝나지 않는다. 10초 동안 100% CPU를 쓰고 멈추지 않는다.
코드와 무관한 도구 문제다.

```
$ cc -O1 -g -fsanitize=address -o hello-a hello.c
$ time timeout 10 ./hello-a
timeout 10 ./hello-a  5.58s user 3.84s system 94% cpu 10.007 total
종료 코드 124        # 124는 timeout이 끊었다는 뜻이다
```

UndefinedBehaviorSanitizer는 정상으로 돈다. 그래서 UBSan은 쓰고, 주소 검사는
Guard Malloc으로 갈음한다.

## Guard Malloc이 실제로 끼어드는지 먼저 확인한다

도구가 일을 하는지 확인하지 않고 "보고 없음"을 결론으로 쓰면 안 된다.
일부러 한 바이트 넘기는 프로그램(`evidence/oob.c`)으로 가른다.

```
$ cc -O0 -g -o oob evidence/oob.c
$ ./oob
안 잡혔다: x
종료 코드 0                      # 보통 빌드는 말없이 넘어간다

$ DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib ./oob
GuardMalloc[oob]: Allocations will be placed on 16 byte boundaries.
종료 코드 139                    # SIGSEGV. 그 자리에서 잡는다
```

## 1단계 코드를 검증한 결과

| 검증 | 결과 |
|---|---|
| UBSan으로 모든 테스트 | 보고 없음 |
| Guard Malloc으로 모든 테스트 | 깨끗함 |
| UBSan으로 깨진 입력 480개 | 비정상 종료 0건, 보고 0건 |
| Guard Malloc으로 깨진 입력 480개 | 비정상 종료 0건 |

깨진 입력은 `tests/*.sl`을 잘라내고, 바이트를 바꾸고, 바이트를 끼워 만들었다.
새니타이저는 실제로 지나간 길만 보므로 입력을 늘려야 쓸모가 있다.

## 검사 스크립트의 함정

처음에 Guard Malloc 검사가 여덟 건 실패했다고 나왔는데 셸 배관 버그였다.
zsh의 MULTIOS가 파이프라인에서 `2>&1 >/dev/null`을 덮어쓰는 대신 덧붙여서,
프로그램의 표준 출력이 검사 대상에 섞여 들었다. `{ cmd >/dev/null; } 2>&1`로 고쳤다.
