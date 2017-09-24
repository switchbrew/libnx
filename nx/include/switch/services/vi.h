typedef enum
{
	VISERVTYPE_Default = -1,
	VISERVTYPE_Application = 0,
	VISERVTYPE_System = 1,
	VISERVTYPE_Manager = 2,
} viServiceType;

Result viInitialize(viServiceType servicetype);
void viExit(void);
Handle viGetSessionService(void);
Handle viGetSession_IApplicationDisplayService(void);
Handle viGetSession_IHOSBinderDriverRelay(void);
Handle viGetSession_ISystemDisplayService(void);
Handle viGetSession_IManagerDisplayService(void);
Handle viGetSession_IHOSBinderDriverIndirect(void);
