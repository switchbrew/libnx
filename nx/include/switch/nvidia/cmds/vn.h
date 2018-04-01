typedef struct {
    NvGpu*    parent;
    NvCmdList cmd_list;

    NvBuffer  vertex_runout;
    NvBuffer  const_buffer0;
    NvBuffer  const_buffer1;
} Vn;

#define VnCmd(vn, ...) \
    NvCmd(&(vn)->cmd_list, __VA_ARGS__)
