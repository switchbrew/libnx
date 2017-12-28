all:
	make -C crt0/
	make -C nx/

install:
	make -C buildscripts/ install
	make -C crt0/ install
	make -C nx/ install

clean:
	make -C crt0/ clean
	make -C nx/ clean
