#include <string.h>
#include "libapplet_internal.h"
#include "applets/album_la.h"

static Result _albumLaShow(u8 arg, bool playStartupSound) {
    Result rc=0;
    LibAppletArgs commonargs;

    libappletArgsCreate(&commonargs, 0x10000);
    libappletArgsSetPlayStartupSound(&commonargs, playStartupSound);

    rc = libappletLaunch(AppletId_photoViewer, &commonargs, &arg, sizeof(arg), NULL, 0, NULL);

    return rc;
}

Result albumLaShowAlbumFiles(void) {
    return _albumLaShow(AlbumLaArg_ShowAlbumFiles, false);
}

Result albumLaShowAllAlbumFiles(void) {
    return _albumLaShow(AlbumLaArg_ShowAllAlbumFiles, false);
}

Result albumLaShowAllAlbumFilesForHomeMenu(void) {
    return _albumLaShow(AlbumLaArg_ShowAllAlbumFilesForHomeMenu, true);
}

