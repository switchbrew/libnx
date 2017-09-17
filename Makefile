all:
	make -C crt0/
	make -C nx/
	make -C tools/

install:
	make -C buildscripts/ install
	make -C crt0/ install
	make -C nx/ install
	make -C tools/ install

clean:
	make -C crt0/ clean
	make -C nx/ clean
