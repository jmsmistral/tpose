#gcc -o tpose util.c btree.c tpose_io.c tpose.c 

prog = tpose
src = $(wildcard src/*.c)

ifeq ($(shell uname -s), Darwin)
	compiler = gcc-5
else
	compiler = gcc
endif

gcc = $(compiler)

$(prog): $(obj) 
	$(gcc) -o $(prog) $(src) 

.PHONY: clean
clean:
	rm -f $(obj) $(prog)

