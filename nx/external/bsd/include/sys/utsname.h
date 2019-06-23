/*
 * Technically compliant implementation based on POSIX specification
 */
#ifndef _SYS_UTSNAME_H_
#define _SYS_UTSNAME_H_

/*
 * "The character arrays are of unspecified size" assume 63 + null byte
 *
 * Members are stored in the order specified in the POSIX description
 */
struct utsname {
  char sysname[64];
  char nodename[64];
  char release[64];
  char version[64];
  char machine[64];
};

/*
 * `uname` can be defined as a macro or a function, so inline is fair game.
 * 
 * -1 shall be returned on error.
 * 
 * Note: errno should be set, but the spec doesn't give a list of valid codes.
 */
static inline int uname(struct utsname *name) {
  return -1;
}

#endif
