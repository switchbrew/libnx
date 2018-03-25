typedef struct NvGpu NvGpu;

typedef struct {
    NvBuffer buffer;
    size_t   num_cmds;
    size_t   max_cmds;
    NvGpu*   parent;
} NvCmdList;

Result nvCmdListCreate(NvCmdList* c, NvGpu* parent, size_t max_cmds);
void   nvCmdListClose(NvCmdList* c);
iova_t nvCmdListGetGpuAddr(NvCmdList* c);
u64    nvCmdListGetListSize(NvCmdList* c);
u32*   nvCmdListInsert(NvCmdList* c, size_t num_cmds);

#define NvCmd(cmd_list, ...) do { \
        u32 _[] = { __VA_ARGS__ }; \
        memcpy(nvCmdListInsert(cmd_list, sizeof(_)/4), _, sizeof(_)); \
    } while (0)

#define NvImm(subc, reg, val) \
    (0x80000000 | (reg) | ((subc) << 13) | ((val) << 16))

#define NvRep(subc, reg, ...) \
    (0x60000000 | ((reg) | ((subc) << 13) | ((sizeof((u32[]) { __VA_ARGS__ })) << 16))), __VA_ARGS__

#define NvIncr(subc, reg, ...) \
    (0x20000000 | ((reg) | ((subc) << 13) | ((sizeof((u32[]) { __VA_ARGS__ })) << 16))), __VA_ARGS__

#define NvIncrOnce(subc, reg, ...) \
    (0xA0000000 | ((reg) | ((subc) << 13) | ((sizeof((u32[]) { __VA_ARGS__ })) << 16))), __VA_ARGS__

static inline u32 f2i(float f) {
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"
    return *(u32*) &f;
}
