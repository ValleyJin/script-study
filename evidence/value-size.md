# `Value` 구조체가 64비트에서 몇 바이트인가

2026-10-01, darwin arm64, Apple clang version 17.0.0 (clang-1700.6.3.2).
`docs/02-만들-언어의-범위.md`의 값 표현 절이 이 결과를 근거로 삼는다.

소스: `evidence/value-size.c`

명령: `cc -O2 -o /tmp/sz evidence/value-size.c && /tmp/sz`

```
sizeof(ValueType) = 4
sizeof(Value)     = 16
_Alignof(Value)   = 8
offsetof(type)    = 0
offsetof(as)      = 8
빈틈              = 4바이트
```

태그가 8바이트를 먹는 것이 아니다. `ValueType`은 4바이트이고, union이 8바이트
정렬을 요구해서 뒤에 4바이트 빈틈이 생긴다. 1회차 초안이 "태그가 8바이트를 먹는다"라고
적은 것은 틀렸다.
