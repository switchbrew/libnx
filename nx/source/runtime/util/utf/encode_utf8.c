#include "runtime/util/utf.h"

ssize_t
encode_utf8(uint8_t  *out,
            uint32_t in)
{
  if(in < 0x80)
  {
    if(out != NULL)
      *out++ = in;
    return 1;
  }
  else if(in < 0x800)
  {
    if(out != NULL)
    {
      *out++ = (in >> 6) + 0xC0;
      *out++ = (in & 0x3F) + 0x80;
    }
    return 2;
  }
  else if(in < 0x10000)
  {
    if(out != NULL)
    {
      *out++ = (in >> 12) + 0xE0;
      *out++ = ((in >> 6) & 0x3F) + 0x80;
      *out++ = (in & 0x3F) + 0x80;
    }
    return 3;
  }
  else if(in < 0x110000)
  {
    if(out != NULL)
    {
      *out++ = (in >> 18) + 0xF0;
      *out++ = ((in >> 12) & 0x3F) + 0x80;
      *out++ = ((in >> 6) & 0x3F) + 0x80;
      *out++ = (in & 0x3F) + 0x80;
    }
    return 4;
  }

  return -1;
}
