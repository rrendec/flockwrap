TARGETS = flockwrap.so

ifeq ($(shell getconf LONG_BIT), 64)
	LIBDIR ?= /usr/local/lib64
else
	LIBDIR ?= /usr/local/lib
endif

.PHONY: all clean install

all: $(TARGETS)

flockwrap.so: flockwrap.c
	$(CC) -o $@ $(CFLAGS) -Wall -O2 -fpic -shared $< -ldl

clean:
	rm -f $(TARGETS)

install: all
	install -D -m 644 flockwrap.so $(LIBDIR)/flockwrap.so
