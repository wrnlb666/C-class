CC ?= gcc
CFLAG = -Wall -Wextra -std=gnu17 `pkg-config --cflags libffi` -g
LIB = `pkg-config --libs libffi` -fsanitize=leak

.PHONY: ffiutil test

all: ffiutil test

ffiutil: src/ffiutil.c 
	$(CC) $(CFLAG) -fPIC -shared $< -o lib$@.so $(LIB)

test: test.c 
	$(CC) $(CFLAG) $< -o $@ -Wl,-rpath=./ $(LIB) -L. -lffiutil
