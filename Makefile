#gcc -o tpose util.c btree.c tpose_io.c tpose.c 

prog = tpose
src = $(wildcard src/*.c)

ifeq ($(shell uname -s), Darwin)
	compiler = gcc-5
else
	compiler = gcc
endif

gcc = $(compiler)

PREFIX = /usr/local

$(prog): $(src) 
	$(gcc) -o $(prog) $(src) 

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

