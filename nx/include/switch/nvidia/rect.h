#define MAKE_RECT(type) \
    typedef struct { \
        type x; \
        type y; \
        type w; \
        type h; \
    } type##_rect;

MAKE_RECT(u16);
MAKE_RECT(u32);
MAKE_RECT(float);
MAKE_RECT(double);
