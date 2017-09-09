all:
	make -C buildscripts/ install
	make -C crt0/ install
	make -C nx/

clean:
	make -C crt0/ clean
	make -C nx/ clean
