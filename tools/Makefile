all: elf2nso elf2nro build_pfs0

build_pfs0: build_pfs0.c
	gcc $(CFLAGS) -o $@ $^

elf2nso: elf2nso.c sha256.c
	gcc $(CFLAGS) -o $@ $^ -llz4

elf2nro: elf2nro.c
	gcc $(CFLAGS) -o $@ $^

install: all
	cp build_pfs0 $(DEVKITA64)/bin/
	cp elf2nso $(DEVKITA64)/bin/

clean:
	rm -f elf2nso elf2nro build_pfs0
