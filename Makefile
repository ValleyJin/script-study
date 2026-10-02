# docs/02 의 "어떻게 재는가" 절대로 -O2 로 잰다.
# 두 갈래를 한 실행 파일에 넣으므로 빌드 옵션은 플래그가 같다.
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
LDFLAGS ?= -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := sl

.PHONY: all clean test

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(BIN)
	./run-tests.sh

clean:
	rm -f $(OBJ) $(BIN)
