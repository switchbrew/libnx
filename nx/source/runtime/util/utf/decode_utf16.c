#include "runtime/util/utf.h"

ssize_t
decode_utf16(uint32_t       *out,
             const uint16_t *in)
{
  uint16_t code1, code2;

  code1 = *in++;
  if(code1 >= 0xD800 && code1 < 0xDC00)
  {
    /* surrogate pair */
    code2 = *in++;
    if(code2 >= 0xDC00 && code2 < 0xE000)
    {
      *out = (code1 << 10) + code2 - 0x35FDC00;
      return 2;
    }

    return -1;
  }

  *out = code1;
  return 1;
}
