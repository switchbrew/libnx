all:
	$(MAKE) -C nx/

install:
	$(MAKE) -C buildscripts/ install
	$(MAKE) -C nx/ install

clean:
	$(MAKE) -C nx/ clean
