/**
 * @file grc.h
 * @brief GRC Game Recording (grc:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/caps.h"
#include "../kernel/event.h"
#include "../kernel/tmem.h"

/// GameMovieTrimmer
typedef struct {
    Service s;              ///< IGameMovieTrimmer
    TransferMemory tmem;    ///< TransferMemory
} GrcGameMovieTrimmer;

/// GameMovieId
typedef struct {
    CapsAlbumEntryId album_id;    ///< \ref CapsAlbumEntryId
    u8 reserved[0x28];            ///< Unused, always zero.
} GrcGameMovieId;

/// Stream type values for \ref grcdRead.
typedef enum {
    GrcStream_Video = 0,          ///< Video stream with H.264 NAL units. Official sw uses buffer size 0x32000.
    GrcStream_Audio = 1,          ///< Audio stream with PcmFormat_Int16, 2 channels, and samplerate = 48000Hz. Official sw uses buffer size 0x1000.
} GrcStream;

// Trimming

/**
 * @brief Creates a \ref GrcGameMovieTrimmer using \ref appletCreateGameMovieTrimmer, uses the cmds from it to trim the specified video, then closes it.
 * @note See \ref appletCreateGameMovieTrimmer for the requirements for using this.
 * @note This will block until video trimming finishes.
 * @param[out] dst_movieid \ref GrcGameMovieId for the output video.
 * @param[in] src_movieid \ref GrcGameMovieId for the input video.
 * @param[in] tmem_size TransferMemory size. Official sw uses size 0x2000000.
 * @param[in] thumbnail Optional, can be NULL. RGBA8 1280x720 thumbnail image data.
 * @param[in] start Start timestamp in 0.5s units.
 * @param[in] end End timestamp in 0.5s units.
 */
Result grcTrimGameMovie(GrcGameMovieId *dst_movieid, const GrcGameMovieId *src_movieid, size_t tmem_size, const void* thumbnail, s32 start, s32 end);

// grc:d

/// Initialize grc:d.
Result grcdInitialize(void);

/// Exit grc:d.
void grcdExit(void);

/// Gets the Service for grc:d.
Service* grcdGetServiceSession(void);

/// Begins streaming. This must not be called more than once, even from a different service session: otherwise the sysmodule will assert.
Result grcdBegin(void);

/**
 * @brief Reads a stream, from the video recording being done of the currently running game title.
 * @note This will block until data is available. This will hang if there is no game title running which has video capture enabled.
 * @param[in] stream \ref GrcStream
 * @param[out] buffer Output buffer.
 * @param[in] size Max size of the output buffer.
 * @param[out] unk Unknown.
 * @param[out] data_size Actual output data size.
 * @param[out] timestamp Timestamp?
 */
Result grcdRead(GrcStream stream, void* buffer, size_t size, u32 *unk, u32 *data_size, u64 *timestamp);

