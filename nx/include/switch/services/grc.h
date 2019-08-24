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

/**
 * @brief Creates a \ref GrcGameMovieTrimmer using \ref appletCreateGameMovieTrimmer, uses the cmds from it to trim the specified video, then closes it.
 * @note See \ref appletCreateGameMovieTrimmer for the requirements for using this.
 * @note This will block until video trimming finishes.
 * @param[out] dst_movieid \ref GrcGameMovieTrimmer for the output video.
 * @param[in] src_movieid \ref GrcGameMovieTrimmer for the input video.
 * @param[in] tmem_size TransferMemory size. Official sw uses size 0x2000000.
 * @param[in] thumbnail Optional, can be NULL. RGBA8 1280x720 thumbnail image data.
 * @param[in] start Start timestamp in 0.5s units.
 * @param[in] end End timestamp in 0.5s units.
 */
Result grcTrimGameMovie(GrcGameMovieId *dst_movieid, const GrcGameMovieId *src_movieid, size_t tmem_size, const void* thumbnail, s32 start, s32 end);

