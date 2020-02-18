CC = gcc
INCLUDES = -Isrc -Ideps/isotp-c/deps/bitfield-c/src -Ideps/isotp-c/src
CFLAGS = $(INCLUDES) -c -Wall -Werror -g -ggdb -std=gnu99 -coverage
LDFLAGS = -coverage -lm
LDLIBS = -lcheck -lm -lrt -lpthread -lsubunit

TEST_DIR = tests
TEST_OBJDIR = build

# Guard against \r\n line endings only in Cygwin
OSTYPE := $(shell uname)
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		TEST_SET_OPTS = igncr
	endif
endif

SRC = $(wildcard src/**/*.c)
SRC += $(wildcard deps/isotp-c/deps/bitfield-c/src/**/*.c)
SRC += $(wildcard deps/isotp-c/src/**/*.c)
OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(SRC:.c=.o))
TEST_SRC = $(wildcard $(TEST_DIR)/test_*.c)
TESTS=$(patsubst %.c,$(TEST_OBJDIR)/%.bin,$(TEST_SRC))
TEST_SUPPORT_SRC = $(TEST_DIR)/common.c
TEST_SUPPORT_OBJS = $(patsubst %,$(TEST_OBJDIR)/%,$(TEST_SUPPORT_SRC:.c=.o))

all: $(OBJS)

test: $(TESTS)
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh runtests.sh $(TEST_OBJDIR)/$(TEST_DIR)

COVERAGE_INFO_FILENAME = coverage.info
COVERAGE_INFO_PATH = $(TEST_OBJDIR)/$(COVERAGE_INFO_FILENAME)
coverage:
	@lcov --base-directory . --directory src --zerocounters -q
	@make clean
	@make test
	@lcov --base-directory . --directory $(TEST_OBJDIR) -c -o $(TEST_OBJDIR)/coverage.info
	@lcov --remove $(COVERAGE_INFO_PATH) "deps/*" -o $(COVERAGE_INFO_PATH)
	@genhtml -o $(TEST_OBJDIR)/coverage -t "isotp-c test coverage" --num-spaces 4 $(COVERAGE_INFO_PATH)
	@$(BROWSER) $(TEST_OBJDIR)/coverage/index.html
	@echo "$(GREEN)Coverage information generated in $(TEST_OBJDIR)/coverage/index.html.$(COLOR_RESET)"

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CC_SYMBOLS) $(INCLUDES) -o $@ $<

$(TEST_OBJDIR)/%.bin: $(TEST_OBJDIR)/%.o $(OBJS) $(TEST_SUPPORT_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(CC_SYMBOLS) $(INCLUDES) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(TEST_OBJDIR)
