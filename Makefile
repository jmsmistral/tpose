# GNU Make and GNU GCC; override with e.g. make CC=gcc-16.
ifeq ($(origin CC),default)
CC = gcc
endif
CC ?= gcc
CFLAGS ?= -O2 -g -Wall -Wextra
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

PREFIX ?= /usr/local
DESTDIR ?=

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
test_src = tests/group_keys.c src/btree.c src/tpose_io.c
headers = $(wildcard src/*.h)

.PHONY: all test test-asan test-ubsan clean install uninstall check-compiler
all: tpose

# Apple supplies Clang under the name gcc. Fail with a useful explanation.
check-compiler:
	@macros="$$($(CC) -dM -E -x c /dev/null)" || exit 1; \
	case "$$macros" in \
	  *__clang__*) echo 'GNU GCC is required. On macOS, select Homebrew GCC with make CC=gcc-<version>.' >&2; exit 1 ;; \
	  *__GNUC__*) ;; \
	  *) echo 'GNU GCC is required. Set CC to your GCC executable.' >&2; exit 1 ;; \
	esac

tpose: $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(obj) -pthread $(LDLIBS)

src/%.o: src/%.c | check-compiler
	$(CC) $(CPPFLAGS) -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) -pthread -MMD -MP -c $< -o $@

tests/group-keys-test: $(test_src) $(headers) Makefile | check-compiler
	$(CC) $(CPPFLAGS) -Isrc -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) $(LDFLAGS) -o $@ $(test_src) -pthread $(LDLIBS)

test: tpose tests/group-keys-test
	sh tests/run.sh ./tpose ./tests/group-keys-test

# Keep the diagnostic binary separate from the normal build and its objects.
tpose-asan: $(src) $(headers) Makefile | check-compiler
	$(CC) $(CPPFLAGS) -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) -O1 -g -fsanitize=address -fno-omit-frame-pointer $(LDFLAGS) -o $@ $(src) -pthread $(LDLIBS)

# Leak cleanup is a separate review item; check invalid memory access here.
tests/group-keys-test-asan: $(test_src) $(headers) Makefile | check-compiler
	$(CC) $(CPPFLAGS) -Isrc -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) -O1 -g -fsanitize=address -fno-omit-frame-pointer $(LDFLAGS) -o $@ $(test_src) -pthread $(LDLIBS)

test-asan: tpose-asan tests/group-keys-test-asan
	ASAN_OPTIONS=detect_leaks=0 sh tests/run.sh ./tpose-asan ./tests/group-keys-test-asan

tpose-ubsan: $(src) $(headers) Makefile | check-compiler
	$(CC) $(CPPFLAGS) -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined $(LDFLAGS) -o $@ $(src) -pthread $(LDLIBS)

tests/group-keys-test-ubsan: $(test_src) $(headers) Makefile | check-compiler
	$(CC) $(CPPFLAGS) -Isrc -D_FILE_OFFSET_BITS=64 -std=gnu11 $(CFLAGS) -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined $(LDFLAGS) -o $@ $(test_src) -pthread $(LDLIBS)

test-ubsan: tpose-ubsan tests/group-keys-test-ubsan
	sh tests/run.sh ./tpose-ubsan ./tests/group-keys-test-ubsan

install: tpose
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 tpose "$(DESTDIR)$(PREFIX)/bin/tpose"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/tpose"

clean:
	rm -f $(obj) $(dep) tpose tpose-asan tpose-ubsan tests/group-keys-test tests/group-keys-test-asan tests/group-keys-test-ubsan
	rm -rf tpose-asan.dSYM tpose-ubsan.dSYM tests/group-keys-test.dSYM tests/group-keys-test-asan.dSYM tests/group-keys-test-ubsan.dSYM

-include $(dep)
