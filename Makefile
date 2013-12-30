CC = gcc
INCLUDES = -Isrc -Ideps/bitfield-c/src -Ideps/isotp-c/src
CFLAGS = $(INCLUDES) -c -w -Wall -Werror -g -ggdb -std=c99
LDFLAGS =
LDLIBS = -lcheck

TEST_DIR = tests

# Guard against \r\n line endings only in Cygwin
OSTYPE := $(shell uname)
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		TEST_SET_OPTS = igncr
	endif
endif

SRC = $(wildcard src/**/*.c)
SRC += $(wildcard deps/bitfield-c/src/**/*.c)
SRC += $(wildcard deps/isotp-c/src/**/*.c)
OBJS = $(SRC:.c=.o)
TEST_SRC = $(wildcard $(TEST_DIR)/test_*.c)
TESTS=$(patsubst %.c,%.bin,$(TEST_SRC))
TEST_SUPPORT_SRC = $(TEST_DIR)/common.c
TEST_SUPPORT_OBJS = $(TEST_SUPPORT_SRC:.c=.o)

all: $(OBJS)

test: $(TESTS)
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh runtests.sh $(TEST_DIR)

$(TEST_DIR)/%.bin: $(TEST_DIR)/%.o $(OBJS) $(TEST_SUPPORT_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(CC_SYMBOLS) $(INCLUDES) -o $@ $^ $(LDLIBS)

clean:
	rm -rf **/*.o $(TEST_DIR)/*.bin
