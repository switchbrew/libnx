all:
	$(MAKE) -C crt0/
	$(MAKE) -C nx/

install:
	$(MAKE) -C buildscripts/ install
	$(MAKE) -C crt0/ install
	$(MAKE) -C nx/ install

clean:
	$(MAKE) -C crt0/ clean
	$(MAKE) -C nx/ clean
