bool smHasInitialized();
Result smInitialize();
void smExit(void);
Result smGetService(Handle* handle_out, const char* name);
Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions);
Result smUnregisterService(const char* name);
