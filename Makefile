# study/02 의 "어떻게 재는가" 절대로 -O2 로 잰다.
# 두 갈래를 한 실행 파일에 넣으므로 빌드 옵션은 플래그가 같다.
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
LDFLAGS ?= -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)
BIN := sl

.PHONY: all clean test fuzz ubsan gmalloc

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# -MMD -MP 로 헤더 의존성을 만들어 둔다. 이것이 없으면 헤더를 고쳐도 .c 를 다시
# 컴파일하지 않아 번역 단위마다 구조체 크기가 어긋난다. 그러면 ALLOCATE 가 잡은
# 자리보다 큰 구조체를 쓰게 되어 힙을 넘긴다. 보통 실행은 조용히 넘어가고
# MALLOC_STRICT_SIZE=1 을 켠 Guard Malloc 만 그것을 잡는다.
src/%.o: src/%.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEP)

test: $(BIN)
	./run-tests.sh

# 깨진 입력을 만들어 넣는다. study/03 의 "지나간 길만 본다" 절이 쓰는 도구다.
fuzz: $(BIN)
	python3 evidence/fuzz.py ./$(BIN)

# 미정의 동작을 본다. 기본값은 찍고 계속 가므로 halt_on_error=1 을 준다.
ubsan:
	$(CC) $(CFLAGS) -O1 -g -fsanitize=undefined -o sl-ubsan $(SRC) $(LDFLAGS)
	@for f in tests/*.sl; do \
	  UBSAN_OPTIONS=halt_on_error=1 ./sl-ubsan --walk $$f >/dev/null || exit 1; \
	done; echo "UBSan 보고 없음"

# 할당 범위 밖 접근을 본다. MALLOC_STRICT_SIZE=1 로 15바이트 사각지대까지 본다.
gmalloc: $(BIN)
	@for f in tests/*.sl; do \
	  DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib MALLOC_STRICT_SIZE=1 \
	    ./$(BIN) --walk $$f >/dev/null 2>/dev/null || { echo "!! $$f"; exit 1; }; \
	done; echo "Guard Malloc 보고 없음"

clean:
	rm -f $(OBJ) $(DEP) $(BIN) sl-ubsan
	rm -rf *.dSYM
