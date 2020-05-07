/**
 * @file diag.h
 * @brief Diagnostics utils (logging implementation, wrapping lm service)
 * @author XorTroll
 * @copyright libnx Authors
 */

#pragma once
#include "../services/lm.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

/// DiagLogSeverity
typedef enum {
    DiagLogSeverity_Trace = 0,
    DiagLogSeverity_Info = 1,
    DiagLogSeverity_Warn = 2,
    DiagLogSeverity_Error = 3,
    DiagLogSeverity_Fatal = 4
} DiagLogSeverity;

/// DiagLogPacketFlags
typedef enum {
    DiagLogPacketFlags_Head = BIT(0),           ///< First packet.
    DiagLogPacketFlags_Tail = BIT(1)            ///< Last packet.
} DiagLogPacketFlags;

/// DiagLogPacketHeader
typedef struct {
    u64 process_id;                             ///< Process ID.
    u64 thread_id;                              ///< Thread ID.
    u8 flags;                                   ///< \ref DiagLogPacketFlags
    u8 pad;                                     ///< Padding
    u8 severity;                                ///< \ref DiagLogSeverity
    u8 verbosity;                               ///< Verbosity.
    u32 payload_size;                           ///< Total packet size after this header.
} DiagLogPacketHeader;

/// DiagLogDataChunkKey
typedef enum {
    DiagLogDataChunkKey_LogSessionBegin = 0,    ///< Log session begin (unknown)
    DiagLogDataChunkKey_LogSessionEnd = 1,      ///< Log session end (unknown)
    DiagLogDataChunkKey_TextLog = 2,            ///< Text to be logged.
    DiagLogDataChunkKey_LineNumber = 3,         ///< Source line number.
    DiagLogDataChunkKey_FileName = 4,           ///< Source file name.
    DiagLogDataChunkKey_FunctionName = 5,       ///< Source function name.
    DiagLogDataChunkKey_ModuleName = 6,         ///< Process module name.
    DiagLogDataChunkKey_ThreadName = 7,         ///< Process thread name.
    DiagLogDataChunkKey_LogPacketDropCount = 8, ///< Log packet drop count (unknown)
    DiagLogDataChunkKey_UserSystemClock = 9,    ///< User system clock (unknown)
    DiagLogDataChunkKey_ProcessName = 10        ///< Process name.
} DiagLogDataChunkKey;

/// DiagLogDataChunkTypeHeader (see specific types below)
typedef struct {
    u8 chunk_key;                               ///< \ref DiagLogDataChunkKey
    u8 chunk_len;                               ///< Value length.
} DiagLogDataChunkTypeHeader;

// Specific chunk types.

NX_CONSTEXPR void diagChunkTypeCreate(void *chunk_type, size_t chunk_type_size, DiagLogDataChunkTypeHeader *header, DiagLogDataChunkKey key) {
    __builtin_memset(chunk_type, 0, chunk_type_size);
    header->chunk_key = key;
    header->chunk_len = 0;
}

typedef struct {
    DiagLogDataChunkTypeHeader header;
    u8 value;
} DiagU8ChunkType;

NX_CONSTEXPR void diagU8ChunkTypeCreate(DiagU8ChunkType *chunk_type, DiagLogDataChunkKey key) {
    diagChunkTypeCreate(chunk_type, sizeof(DiagU8ChunkType), &chunk_type->header, key);
}

NX_CONSTEXPR void diagU8ChunkTypeSetValue(DiagU8ChunkType *chunk_type, DiagLogDataChunkKey key, u8 value) {
    diagU8ChunkTypeCreate(chunk_type, key);
    chunk_type->header.chunk_len = sizeof(u8);
    chunk_type->value = value;
}

typedef struct {
    DiagLogDataChunkTypeHeader header;
    u32 value;
} DiagU32ChunkType;

NX_CONSTEXPR void diagU32ChunkTypeCreate(DiagU32ChunkType *chunk_type, DiagLogDataChunkKey key) {
    diagChunkTypeCreate(chunk_type, sizeof(DiagU32ChunkType), &chunk_type->header, key);
}

NX_CONSTEXPR void diagU32ChunkTypeSetValue(DiagU32ChunkType *chunk_type, DiagLogDataChunkKey key, u32 value) {
    diagU32ChunkTypeCreate(chunk_type, key);
    chunk_type->header.chunk_len = sizeof(u32);
    chunk_type->value = value;
}

typedef struct {
    DiagLogDataChunkTypeHeader header;
    u64 value;
} DiagU64ChunkType;

NX_CONSTEXPR void diagU64ChunkTypeCreate(DiagU64ChunkType *chunk_type, DiagLogDataChunkKey key) {
    diagChunkTypeCreate(chunk_type, sizeof(DiagU64ChunkType), &chunk_type->header, key);
}

NX_CONSTEXPR void diagU64ChunkTypeSetValue(DiagU64ChunkType *chunk_type, DiagLogDataChunkKey key, u64 value) {
    diagU64ChunkTypeCreate(chunk_type, key);
    chunk_type->header.chunk_len = sizeof(u64);
    chunk_type->value = value;
}

#define DIAG_MAX_STRING_LEN 0xFF

typedef struct {
    DiagLogDataChunkTypeHeader header;
    char value[DIAG_MAX_STRING_LEN + 1];
} DiagStringChunkType;

NX_CONSTEXPR void diagStringChunkTypeCreate(DiagStringChunkType *chunk_type, DiagLogDataChunkKey key) {
    diagChunkTypeCreate(chunk_type, sizeof(DiagStringChunkType), &chunk_type->header, key);
}

NX_CONSTEXPR void diagStringChunkTypeSetValue(DiagStringChunkType *chunk_type, DiagLogDataChunkKey key, const char *value, const size_t value_len) {
    diagStringChunkTypeCreate(chunk_type, key);
    const size_t len = (value_len > DIAG_MAX_STRING_LEN) ? DIAG_MAX_STRING_LEN : value_len;
    chunk_type->header.chunk_len = (u8)len;
    __builtin_memcpy(chunk_type->value, value, len);
}

typedef struct {
    DiagU8ChunkType log_session_begin;
    DiagU8ChunkType log_session_end;
    DiagStringChunkType text_log;
    DiagU32ChunkType line_number;
    DiagStringChunkType file_name;
    DiagStringChunkType function_name;
    DiagStringChunkType module_name;
    DiagStringChunkType thread_name;
    DiagU64ChunkType log_packet_drop_count;
    DiagU64ChunkType user_system_clock;
    DiagStringChunkType process_name;
} DiagLogPacketPayload;

// Struct containing everything that will be logged.

typedef struct {
    DiagLogPacketHeader header;
    DiagLogPacketPayload payload;
} DiagLogPacket;

// Helper structs for logging parameters.

typedef struct {
    u32 line_number;
    const char *file_name;
    const char *function_name;
} DiagSourceInfo;

typedef struct {
    DiagSourceInfo source_info;
    DiagLogSeverity severity;
    bool verbosity;
    const char *text_log;
} DiagLogMetadata;

// Log packet allocation/freeing

/// Allocates the amount of log packets needed depending on the message length.
NX_INLINE DiagLogPacket *diagAllocateLogPackets(const size_t msg_len, size_t *out_packet_count) {
    size_t remaining_len = msg_len;
    size_t packet_count = 1;
    while(remaining_len > DIAG_MAX_STRING_LEN) {
        packet_count++;
        remaining_len -= DIAG_MAX_STRING_LEN;
    }

    // Always send at least 2 packets, a head and a tail.
    if(packet_count < 2) {
        packet_count = 2;
    }

    DiagLogPacket *packets = (DiagLogPacket*)calloc(packet_count, sizeof(DiagLogPacket));
    if(packets == NULL) {
        return 0;
    }
    *out_packet_count = packet_count;
    return packets;
}

NX_INLINE void diagFreeLogPackets(DiagLogPacket *packet_buf) {
    free(packet_buf);
}

// Other log packet / chunk type helpers

NX_CONSTEXPR bool diagIsChunkTypeEmpty(const DiagLogDataChunkTypeHeader *chunk_header) {
    return chunk_header->chunk_len == 0;
}

NX_CONSTEXPR size_t diagComputeChunkKeySize(const DiagLogDataChunkTypeHeader *chunk_header) {
    if(diagIsChunkTypeEmpty(chunk_header)) {
        return 0;
    }
    return sizeof(DiagLogDataChunkTypeHeader) + chunk_header->chunk_len;
}

NX_CONSTEXPR size_t diagComputeLogPacketPayloadSize(const DiagLogPacket *packet) {
    return diagComputeChunkKeySize(&packet->payload.log_session_begin.header) +
    diagComputeChunkKeySize(&packet->payload.log_session_end.header) +
    diagComputeChunkKeySize(&packet->payload.text_log.header) +
    diagComputeChunkKeySize(&packet->payload.line_number.header) +
    diagComputeChunkKeySize(&packet->payload.file_name.header) +
    diagComputeChunkKeySize(&packet->payload.function_name.header) +
    diagComputeChunkKeySize(&packet->payload.module_name.header) +
    diagComputeChunkKeySize(&packet->payload.thread_name.header) +
    diagComputeChunkKeySize(&packet->payload.log_packet_drop_count.header) +
    diagComputeChunkKeySize(&packet->payload.user_system_clock.header) +
    diagComputeChunkKeySize(&packet->payload.process_name.header);
}

// Log payload encoding helpers

NX_CONSTEXPR u8 *diagLogPayloadEncode(u8 *payload_buf, const void *data, size_t size) {
    if(size > 0) {
        __builtin_memcpy(payload_buf, data, size);
        return payload_buf + size;
    }
    return payload_buf;
}

NX_CONSTEXPR u8 *diagLogPayloadEncodeU8ChunkType(u8 *payload_buf, DiagU8ChunkType *chunk_type) {
    if(diagIsChunkTypeEmpty(&chunk_type->header)) {
        return payload_buf;
    }
    u8 *buf = diagLogPayloadEncode(payload_buf, &chunk_type->header, sizeof(DiagLogDataChunkTypeHeader));
    // Copy value.
    return diagLogPayloadEncode(buf, &chunk_type->value, chunk_type->header.chunk_len);
}

NX_CONSTEXPR u8 *diagLogPayloadEncodeU32ChunkType(u8 *payload_buf, DiagU32ChunkType *chunk_type) {
    if(diagIsChunkTypeEmpty(&chunk_type->header)) {
        return payload_buf;
    }
    u8 *buf = diagLogPayloadEncode(payload_buf, &chunk_type->header, sizeof(DiagLogDataChunkTypeHeader));
    // Copy value.
    return diagLogPayloadEncode(buf, &chunk_type->value, chunk_type->header.chunk_len);
}

NX_CONSTEXPR u8 *diagLogPayloadEncodeU64ChunkType(u8 *payload_buf, DiagU64ChunkType *chunk_type) {
    if(diagIsChunkTypeEmpty(&chunk_type->header)) {
        return payload_buf;
    }
    u8 *buf = diagLogPayloadEncode(payload_buf, &chunk_type->header, sizeof(DiagLogDataChunkTypeHeader));
    // Copy value.
    return diagLogPayloadEncode(buf, &chunk_type->value, chunk_type->header.chunk_len);
}

NX_CONSTEXPR u8 *diagLogPayloadEncodeStringChunkType(u8 *payload_buf, DiagStringChunkType *chunk_type) {
    if(diagIsChunkTypeEmpty(&chunk_type->header)) {
        return payload_buf;
    }
    u8 *buf = diagLogPayloadEncode(payload_buf, &chunk_type->header, sizeof(DiagLogDataChunkTypeHeader));
    // Copy string by its length.
    return diagLogPayloadEncode(buf, chunk_type->value, chunk_type->header.chunk_len);
}

/**
 * @brief Logs via lm.
 * @note Retail lm is stubbed.
 * @param[in] metadata Log parameters.
 */
NX_INLINE void diagLogImpl(const DiagLogMetadata *metadata) {
    Result rc = smInitialize();
    if(R_SUCCEEDED(rc)) {
        rc = lmInitialize();
        smExit();
    }
    if(R_SUCCEEDED(rc)) {
        size_t count = 0;
        const size_t text_log_len = __builtin_strlen(metadata->text_log);
        DiagLogPacket *packets = diagAllocateLogPackets(text_log_len, &count);
        if(count > 0 && packets != NULL) {
            DiagLogPacket *head_packet = &packets[0];
            head_packet->header.flags = DiagLogPacketFlags_Head;
            DiagLogPacket *tail_packet = &packets[count - 1];
            tail_packet->header.flags = DiagLogPacketFlags_Tail;

            const size_t file_name_len = __builtin_strlen(metadata->source_info.file_name);
            diagStringChunkTypeSetValue(&head_packet->payload.file_name, DiagLogDataChunkKey_FileName, metadata->source_info.file_name, file_name_len);

            const size_t function_name_len = __builtin_strlen(metadata->source_info.function_name);
            diagStringChunkTypeSetValue(&head_packet->payload.function_name, DiagLogDataChunkKey_FunctionName, metadata->source_info.function_name, function_name_len);

            diagU32ChunkTypeSetValue(&head_packet->payload.line_number, DiagLogDataChunkKey_LineNumber, metadata->source_info.line_number);

            const char *module = "libnx";
            diagStringChunkTypeSetValue(&head_packet->payload.module_name, DiagLogDataChunkKey_ModuleName, module, __builtin_strlen(module));

            size_t remaining_len = text_log_len;
            DiagLogPacket *cur_packet = head_packet;
            const char *text_log_buf = metadata->text_log;
            while(remaining_len > 0) {
                const size_t cur_len = (remaining_len > DIAG_MAX_STRING_LEN) ? DIAG_MAX_STRING_LEN : remaining_len;
                diagStringChunkTypeSetValue(&cur_packet->payload.text_log, DiagLogDataChunkKey_TextLog, text_log_buf, cur_len);
                cur_packet++;
                text_log_buf += cur_len;
                remaining_len -= cur_len;
            }

            size_t i = 0;
            for(; i < count; i++) {
                DiagLogPacket *cur_packet = &packets[i];
                cur_packet->header.severity = (u8)metadata->severity;
                cur_packet->header.verbosity = (u8)metadata->verbosity;
                
                cur_packet->header.payload_size = (u32)diagComputeLogPacketPayloadSize(cur_packet);
                const size_t log_buf_size = cur_packet->header.payload_size + sizeof(DiagLogPacketHeader);

                void *log_buf = calloc(1, log_buf_size);
                if(log_buf != NULL) {
                    // Write the packet's header.
                    u8 *payload_buf = diagLogPayloadEncode((u8*)log_buf, &cur_packet->header, sizeof(DiagLogPacketHeader));
                    
                    // Write non-empty packets to the payload.
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.log_session_begin.header)) {
                        payload_buf = diagLogPayloadEncodeU8ChunkType(payload_buf, &cur_packet->payload.log_session_begin);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.log_session_end.header)) {
                        payload_buf = diagLogPayloadEncodeU8ChunkType(payload_buf, &cur_packet->payload.log_session_end);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.text_log.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.text_log);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.line_number.header)) {
                        payload_buf = diagLogPayloadEncodeU32ChunkType(payload_buf, &cur_packet->payload.line_number);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.file_name.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.file_name);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.function_name.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.function_name);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.module_name.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.module_name);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.thread_name.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.thread_name);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.log_packet_drop_count.header)) {
                        payload_buf = diagLogPayloadEncodeU64ChunkType(payload_buf, &cur_packet->payload.log_packet_drop_count);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.user_system_clock.header)) {
                        payload_buf = diagLogPayloadEncodeU64ChunkType(payload_buf, &cur_packet->payload.user_system_clock);
                    }
                    if(!diagIsChunkTypeEmpty(&cur_packet->payload.process_name.header)) {
                        payload_buf = diagLogPayloadEncodeStringChunkType(payload_buf, &cur_packet->payload.process_name);
                    }

                    rc = lmLog(log_buf, log_buf_size);
                    free(log_buf);
                    if(R_FAILED(rc)) {
                        break;
                    }
                }
            }
            diagFreeLogPackets(packets);
        }
        lmExit();
    }
}

// Helper macros.

#define DIAG_DETAILED_LOG(log_severity, log_verbosity, msg) ({ \
    const DiagLogMetadata __log_metadata = { \
        .source_info = { \
            .line_number = __LINE__, \
            .file_name = __FILE__, \
            .function_name = __func__, \
        }, \
        .severity = log_severity, \
        .verbosity = log_verbosity, \
        .text_log = msg, \
    }; \
    diagLogImpl(&__log_metadata); \
})

#define DIAG_DETAILED_LOGF(log_severity, log_verbosity, fmt, ...) ({ \
    char msg[0x400] = {}; \
    sprintf(msg, fmt, ##__VA_ARGS__); \
    DIAG_DETAILED_LOG(log_severity, log_verbosity, msg); \
})

#define DIAG_DETAILED_VLOG(log_severity, log_verbosity, fmt, args) ({ \
    char msg[0x400] = {}; \
    vsprintf(msg, fmt, args); \
    DIAG_DETAILED_LOG(log_severity, log_verbosity, msg); \
})

#define DIAG_LOG(msg) DIAG_DETAILED_LOG(DiagLogSeverity_Info, false, msg)

#define DIAG_LOGF(fmt, ...) DIAG_DETAILED_LOGF(DiagLogSeverity_Info, false, fmt, ##__VA_ARGS__)

#define DIAG_VLOG(fmt, args) DIAG_DETAILED_VLOG(DiagLogSeverity_Info, false, fmt, args)
