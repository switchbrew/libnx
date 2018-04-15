typedef struct {
    float scale[3];
    float translate[3];
    u16_rect window;
    float near;
    float far;
} VnViewportConfig;

static inline void vnViewportSetScale(VnViewportConfig* c, float x, float y, float z) {
    c->scale[0] = x;
    c->scale[1] = y;
    c->scale[2] = z;
}

static inline void vnViewportSetTranslate(VnViewportConfig* c, float x, float y, float z) {
    c->translate[0] = x;
    c->translate[1] = y;
    c->translate[2] = z;
}

static inline void vnViewportSetWindow(VnViewportConfig* c, u16_rect r) {
    c->window = r;
}

static inline void vnViewportSetDepth(VnViewportConfig* c, float near, float far) {
    c->near = near;
    c->far = far;
}

static inline void vnViewportSetDefaults(VnViewportConfig* c) {
    vnViewportSetScale(c, 0.5, 0.5, 0.5);
    vnViewportSetTranslate(c, 0.5, 0.5, 0.5);
    vnViewportSetWindow(c, (u16_rect) { 0, 0, 0xFFFF, 0xFFFF });
    vnViewportSetDepth(c, 0, 0); //?
}

void vnSetViewport(Vn* vn, size_t index, VnViewportConfig* c);

