/**
 * @file audrec.h
 * @brief Audio Recorder IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../audio/audio.h"
#include "../sf/service.h"
#include "../kernel/event.h"

typedef struct {
    u64 released_ns;
    u64 next_buffer_ptr;
    u64 sample_buffer_ptr;
    u64 sample_buffer_capacity;
    u64 data_size;
    u64 data_offset;
} FinalOutputRecorderBuffer;

typedef struct {
    u32 sample_rate;
    u32 channel_count;
} FinalOutputRecorderParameter;

typedef struct {
    u32 sample_rate;
    u32 channel_count;
    u32 sample_format;
    u32 state;
} FinalOutputRecorderParameterInternal;

typedef struct {
    Service s;
} AudrecRecorder;

Result audrecInitialize(void);
void audrecExit(void);
Service* audrecGetServiceSession(void);

Result audrecOpenFinalOutputRecorder(AudrecRecorder* recorder_out, FinalOutputRecorderParameter* param_in, u64 aruid, FinalOutputRecorderParameterInternal* param_out);

Result audrecRecorderStart(AudrecRecorder* recorder);
Result audrecRecorderStop(AudrecRecorder* recorder);
Result audrecRecorderRegisterBufferEvent(AudrecRecorder* recorder, Event* out_event);
Result audrecRecorderAppendFinalOutputRecorderBuffer(AudrecRecorder* recorder, u64 buffer_client_ptr, FinalOutputRecorderBuffer* param);
Result audrecRecorderGetReleasedFinalOutputRecorderBuffers(AudrecRecorder* recorder, u64* out_buffers, u64* inout_count, u64* out_released);
void audrecRecorderClose(AudrecRecorder* recorder);
