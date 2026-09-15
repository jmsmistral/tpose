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

.PHONY: all test clean install uninstall check-compiler
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

test: tpose
	sh tests/run.sh ./tpose

install: tpose
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 tpose "$(DESTDIR)$(PREFIX)/bin/tpose"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/tpose"

clean:
	rm -f $(obj) $(dep) tpose

-include $(dep)
