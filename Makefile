#gcc -o tpose util.c btree.c tpose_io.c tpose.c 

prog = tpose
src = $(wildcard src/*.c)

# If compiling on OSX, use homebrew to install gcc.
# This is currently gcc-5, but might be different
# on your system. If different, simply change "gcc-5"
# below with the name of the gcc compiler on your
# system - typically found in "/usr/local/bin/gcc-5"
# See tpose home page for install details
ifeq ($(shell uname -s), Darwin)
	compiler = gcc-5
else
	compiler = gcc
endif

gcc = $(compiler)
# Uncomment -D option below to compile tpose in debug mode
# This will print out debug info relevant to devs
flags = -lpthread #-DTPOSE_DEBUG=1

PREFIX = /usr/local

$(prog): $(src) 
	$(gcc) -o $(prog) $(src) $(flags)

.PHONY: install
install:
	mkdir -p	$(DESTDIR)$(PREFIX)/bin
	cp $(prog) $(DESTDIR)$(PREFIX)/bin/$(prog)

.PHONY: uninstall
uninstall:
	rm -f	$(DESTDIR)$(PREFIX)/bin/$(prog)

.PHONY: clean
clean:
	rm -f $(obj) $(prog)

