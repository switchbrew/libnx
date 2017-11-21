Result gfxproducerInitialize(binderSession *session);
void gfxproducerExit();

Result gfxproducerRequestBuffer(s32 bufferIdx);
Result gfxproducerDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf);
Result gfxproducerDetachBuffer(s32 slot);
Result gfxproducerQueueBuffer(s32 buf, u8 input[0x5c]);
Result gfxproducerQuery(s32 what, s32* value);
Result gfxproducerConnect(s32 api, bool producerControlledByApp);
Result gfxproducerDisconnect(s32 api);
Result gfxproducerTegraBufferInit(s32 buf, u8 input[0x178]);
