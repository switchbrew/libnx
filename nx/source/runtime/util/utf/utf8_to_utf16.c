#include "runtime/util/utf.h"

ssize_t
utf8_to_utf16(uint16_t      *out,
              const uint8_t *in,
              size_t        len)
{
  ssize_t  rc = 0;
  ssize_t  units;
  uint32_t code;
  uint16_t encoded[2];

  do
  {
    units = decode_utf8(&code, in);
    if(units == -1)
      return -1;

    if(code > 0)
    {
      in += units;

      units = encode_utf16(encoded, code);
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
  } while(code > 0);

  return rc;
}
