#include <string.h>
#include <stdlib.h>

#include "result.h"
#include "runtime/env.h"
#include "kernel/svc.h"
#include "services/set.h"
#include "nacp.h"

static u32 g_nacpLanguageTable[15] = {
    [SetLanguage_JA]    = 2,
    [SetLanguage_ENUS]  = 0,
    [SetLanguage_ENGB]  = 1,
    [SetLanguage_FR]    = 3,
    [SetLanguage_DE]    = 4,
    [SetLanguage_ES419] = 5,
    [SetLanguage_ES]    = 6,
    [SetLanguage_IT]    = 7,
    [SetLanguage_NL]    = 8,
    [SetLanguage_FRCA]  = 9,
    [SetLanguage_PT]    = 10,
    [SetLanguage_RU]    = 11,
    [SetLanguage_KO]    = 12,
    [SetLanguage_ZHTW]  = 13,
    [SetLanguage_ZHCN]  = 14,
};

//Official sw uses nsam for this, but since that's a privileged service, use set-service instead for compatibility with newer system-versions.
Result nacpGetLanguageEntry(NacpStruct* nacp, NacpLanguageEntry** langentry) {
    Result rc=0;
    u64 LanguageCode=0;
    s32 Language=0;
    NacpLanguageEntry *entry = NULL;
    u32 i=0;

    if (nacp==NULL || langentry==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *langentry = NULL;

    rc = setInitialize();
    if (R_FAILED(rc))
        return rc;

    rc = setGetSystemLanguage(&LanguageCode);
    if (R_SUCCEEDED(rc))
        rc = setMakeLanguage(LanguageCode, &Language);

    if (Language < 0)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (R_SUCCEEDED(rc) && Language >= 15)
        Language = SetLanguage_ENUS;//Use ENUS for unsupported system languages.

    setExit();

    if (R_FAILED(rc))
        return rc;

    entry = &nacp->lang[g_nacpLanguageTable[Language]];

    if (entry->name[0]==0 && entry->author[0]==0) {
        for(i=0; i<16; i++) {
            entry = &nacp->lang[i];
            if (entry->name[0] || entry->author[0]) break;
        }
    }

    if (entry->name[0]==0 && entry->author[0]==0)
        return rc;

    *langentry = entry;

    return rc;
}

