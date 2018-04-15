enum {
    NvCmdCommon_BindObject = 0
};

typedef struct {
    NvGpu*    parent;
    NvCmdList cmd_list;

    NvBuffer  vertex_runout;
    NvBuffer  const_buffer0;
} Vn;

#define vnAddCmd(vn, ...) \
    nvCmdListAddCmd(&(vn)->cmd_list, __VA_ARGS__)

static inline Result vnSubmit(Vn* v) {
    NvFence f;
    return nvGpfifoSubmit(&v->parent->gpfifo, &v->cmd_list, &f);
}

Result vnInit(Vn* vn, NvGpu* parent);
