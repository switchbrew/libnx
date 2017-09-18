switch_crt0.o: *.s
	$(DEVKITA64)/bin/aarch64-none-elf-gcc -march=armv8-a -x assembler-with-cpp -c $^ -o $@

install: switch_crt0.o
	cp $^ $(DEVKITA64)/aarch64-none-elf/lib/$^

clean:
	rm -f switch_crt0.o
