Nintendo Switch AArch64-only userland library. Based on libctru.

Install:

* The \*rules files under buildscripts/ should be copied to "devkitA64/".
* The content of buildscripts/lib/ should be copied to "devkitA64/aarch64-none-elf/lib/".
* Build and install switch_crt0 with:
  make -C crt0 install