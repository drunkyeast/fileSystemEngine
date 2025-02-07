#include "common.h"
#include "mmap_file.h"


using namespace qiniu;

static const mode_t OPEN_MODE = 0644; // 八进制 000 110 100 100 对应-rw-r--r--
const static largefile::MMapOption mmap_option = { 10240000, 4096, 4096 };

// 为什么不直接使用open函数呢? 答:怕调用open后再调用read,导致errno被覆盖.
int open_file(std::string file_name, int open_flags)
{
    int fd = open(file_name.c_str(), open_flags, OPEN_MODE); // 用man 2 open 查询
    if (fd < 0)
    {
        return -errno; // errno是正数, strerror(errno); errno为0表示正常.
    }
    return fd;
}
int main()
{
    // 打开/创建一个文件, 获取句柄
    const char* filename = "./mapfile.txt";
    int fd = open_file(filename, O_RDWR | O_CREAT | O_LARGEFILE);
    if (fd < 0)
    {
        fprintf(stderr, "open file failed. filename: %s, error desc: %s\n", filename, strerror(-fd));
        return -1;
    }

    largefile::MMapFile* map_file = new largefile::MMapFile(mmap_option, fd);

    bool is_mapped = map_file->map_file(true); // ture控制是否可写

    if (is_mapped)
    {
        map_file->remap_file(); // 增大
        memset(map_file->get_data(), '9', map_file->get_size());
        map_file->sync_file();
        map_file->munmap_file(); // 同步后解除很合理吧.
    }
    else
    {
        fprintf(stderr, "map file failed\n");
    }

    close(fd);
    return 0;
}