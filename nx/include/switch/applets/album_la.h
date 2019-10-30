/**
 * @file album_la.h
 * @brief Wrapper for using the Album LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Arg type values pushed for the applet input storage, stored as an u8.
typedef enum {
    AlbumLaArg_ShowAlbumFiles                 = 0,  ///< ShowAlbumFiles. Only displays AlbumFiles associated with the application which launched the Album applet, with the filter button disabled.
    AlbumLaArg_ShowAllAlbumFiles              = 1,  ///< ShowAllAlbumFiles. Displays all AlbumFiles, with filtering allowed.
    AlbumLaArg_ShowAllAlbumFilesForHomeMenu   = 2,  ///< ShowAllAlbumFilesForHomeMenu. Similar to ::AlbumLaArg_ShowAllAlbumFiles.
} AlbumLaArg;

/**
 * @brief Launches the applet with ::AlbumLaArg_ShowAlbumFiles and playStartupSound=false.
 */
Result albumLaShowAlbumFiles(void);

/**
 * @brief Launches the applet with ::AlbumLaArg_ShowAllAlbumFiles and playStartupSound=false.
 */
Result albumLaShowAllAlbumFiles(void);

/**
 * @brief Launches the applet with ::AlbumLaArg_ShowAllAlbumFilesForHomeMenu and playStartupSound=true.
 */
Result albumLaShowAllAlbumFilesForHomeMenu(void);

