elf2nso: elf2nso.c sha256.c
	gcc -o $@ $^ -llz4

install: elf2nso
	cp elf2nso /usr/local/bin/

clean:
	rm -f elf2nso
