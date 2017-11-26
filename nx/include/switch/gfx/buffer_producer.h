Result bufferProducerInitialize(binderSession *session);
void bufferProducerExit();

Result bufferProducerRequestBuffer(s32 bufferIdx);
Result bufferProducerDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf);
Result bufferProducerDetachBuffer(s32 slot);
Result bufferProducerQueueBuffer(s32 buf, u8 input[0x5c]);
Result bufferProducerQuery(s32 what, s32* value);
Result bufferProducerConnect(s32 api, bool producerControlledByApp);
Result bufferProducerDisconnect(s32 api);
Result bufferProducerTegraBufferInit(s32 buf, u8 input[0x178]);
