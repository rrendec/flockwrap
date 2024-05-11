TARGETS = flockwrap.so

.PHONY: all clean

all: $(TARGETS)

flockwrap.so: flockwrap.c
	$(CC) -o $@ $(CFLAGS) -Wall -O2 -fpic -shared $< -ldl

clean:
	rm -f $(TARGETS)
