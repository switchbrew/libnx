/// Do not use viInitialize/viExit when using these.
void gfxInitDefault(void);
void gfxExit(void);

void gfxWaitForVsync();
void gfxSwapBuffers();
u8* gfxGetFramebuffer(u16* width, u16* height);
void gfxFlushBuffers(void);
