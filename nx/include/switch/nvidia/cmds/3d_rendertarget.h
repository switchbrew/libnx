typedef struct {
    NvBuffer* color_buffer;
    size_t width;
    size_t height;
    NvBufferKind format;
} VnRenderTargetConfig;

void vnRenderTargetSetColorBuffer(VnRenderTargetConfig* c, NvBuffer* buffer) {
    c->color_buffer = buffer;
}

void vnRenderTargetSetDimensions(VnRenderTargetConfig* c, size_t width, size_t height) {
    c->width = width;
    c->height = height;
}

void vnRenderTargetSetFormat(VnRenderTargetConfig* c, NvBufferKind format) {
    c->format = format;
}

void vnSetRenderTargets(Vn* vn, VnRenderTargetConfig* targets, size_t num_targets);
