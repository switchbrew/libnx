#include "runtime/util/utf.h"


ssize_t
utf32_to_utf16(uint16_t       *out,
               const uint32_t *in,
               size_t         len)
{
  ssize_t  rc = 0;
  ssize_t  units;
  uint16_t encoded[2];

  while(*in > 0)
  {
    units = encode_utf16(encoded, *in++);
    if(units == -1)
      return -1;

    if(out != NULL)
    {
      if(rc + units <= len)
      {
        *out++ = encoded[0];
        if(units > 1)
          *out++ = encoded[1];
      }
    }

    if(SSIZE_MAX - units >= rc)
      rc += units;
    else
      return -1;
  }

  return rc;
}
