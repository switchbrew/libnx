typedef enum
{
    APPLET_TYPE_None = -2,
    APPLET_TYPE_Default = -1,
    APPLET_TYPE_Application = 0,
    APPLET_TYPE_SystemApplet = 1,
    APPLET_TYPE_LibraryApplet = 2,
    APPLET_TYPE_OverlayApplet = 3,
    APPLET_TYPE_SystemApplication = 4,
} appletType;

Result appletInitialize(void);
void appletExit(void);
Result appletGetAppletResourceUserId(u64 *out);
