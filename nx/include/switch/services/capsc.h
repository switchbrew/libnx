/**
 * @file capsc.h
 * @brief Album Control (caps:c) service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"

/// Initialize caps:c
Result capscInitialize(void);

/// Exit caps:c.
void capscExit(void);

/// Gets the Service for caps:c.
Service* capscGetServiceSession(void);
Result capscNotifyAlbumStorageIsAvailable(CapsAlbumStorage storage);
Result capscNotifyAlbumStorageIsUnAvailable(CapsAlbumStorage storage);
Result capscGetApplicationIdFromAruid(u64 *application_id, u64 aruid);
Result capscCheckApplicationIdRegistered(u64 application_id);
Result capscGenerateCurrentAlbumFileId(u64 application_id, CapsAlbumFileContents contents, CapsAlbumFileId *file_id);
Result capscSaveAlbumScreenShotFile(CapsAlbumFileId *file_id, void* buffer, u64 buffer_size);
Result capscSaveAlbumScreenShotFileEx(CapsAlbumFileId *file_id, u64 unk_0, u64 unk_1, u64 unk_2, void* buffer, u64 buffer_size);

Result capscOpenAlbumMovieReadStream(u64 *stream, const CapsAlbumFileId *file_id);
Result capscCloseAlbumMovieStream(u64 stream);
Result capscGetAlbumMovieStreamSize(u64 stream, u64 *size);
Result capscReadMovieDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size);
Result capscGetAlbumMovieReadStreamBrokenReason(u64 stream);
Result capscGetAlbumMovieReadStreamImageDataSize(u64 stream, u64 *size);
Result capscReadImageDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size);
Result capscReadFileAttributeFromAlbumMovieReadStream(u64 stream, CapsScreenShotAttribute *attribute);
Result capscOpenAlbumMovieWriteStream(u64 *stream, const CapsAlbumFileId *file_id);
Result capscFinishAlbumMovieWriteStream(u64 stream);
Result capscCommitAlbumMovieWriteStream(u64 stream);
Result capscDiscardAlbumMovieWriteStream(u64 stream);
Result capscDiscardAlbumMovieWriteStreamNoDelete(u64 stream);
Result capscCommitAlbumMovieWriteStreamEx(u64 stream, CapsAlbumCommitOutput *out);
Result capscStartAlbumMovieWriteStreamDataSection(u64 stream);
Result capscEndAlbumMovieWriteStreamDataSection(u64 stream);
Result capscStartAlbumMovieWriteStreamMetaSection(u64 stream);
Result capscEndAlbumMovieWriteStreamMetaSection(u64 stream);
Result capscReadDataFromAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size, u64 *actual_size);
Result capscWriteDataToAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size);
Result capscWriteMetaToAlbumMovieWriteStream();
Result capscGetAlbumMovieWriteStreamBrokenReason(u64 stream);
Result capscGetAlbumMovieWriteStreamDataSize(u64 stream, u64 *size);
Result capscSetAlbumMovieWriteStreamDataSize(u64 stream, u64 size);
