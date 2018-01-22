#include "runtime/util/utf.h"

ssize_t
utf16_to_utf8(uint8_t        *out,
              const uint16_t *in,
              size_t         len)
{
  ssize_t  rc = 0;
  ssize_t  units;
  uint32_t code;
  uint8_t  encoded[4];

  do
  {
    units = decode_utf16(&code, in);
    if(units == -1)
      return -1;

    if(code > 0)
    {
      in += units;

      units = encode_utf8(encoded, code);
      if(units == -1)
        return -1;

      if(out != NULL)
      {
        if(rc + units <= len)
        {
          *out++ = encoded[0];
          if(units > 1)
            *out++ = encoded[1];
          if(units > 2)
            *out++ = encoded[2];
          if(units > 3)
            *out++ = encoded[3];
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
