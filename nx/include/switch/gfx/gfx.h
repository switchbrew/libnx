/// Do not use viInitialize/viExit when using these.
void gfxInitDefault(void);
void gfxExit(void);

void gfxWaitForVsync();
void gfxSwapBuffers();
u8* gfxGetFramebuffer(u32* width, u32* height);
void gfxFlushBuffers(void);

u32 gfxGetFramebufferDisplayOffset(u32 x, u32 y);
