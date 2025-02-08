#include "mmap_file_op.h"
#include <iostream>

using namespace qiniu;

const static largefile::MMapOption mmap_option = { 10240000, 4096, 4096 };

int main(void)
{
    const char* filename = "mmap_file_op.txt";
    largefile::MMapFileOperation* mmfo = new largefile::MMapFileOperation(filename);

    int fd = mmfo->open_file();
    if (fd < 0)
    {
        fprintf(stderr, "open file %s failed. reason: %s", filename, strerror(errno));
        exit(-1); // -1等价于255
    }

    int ret = mmfo->mmap_file(mmap_option);
    if (ret == largefile::TFS_ERROR)
    {
        fprintf(stderr, "mmap_file failed. reason:%s", strerror(errno));
        mmfo->close_file();
        exit(-2); // 等价于254??
    }

    char buffer[128 + 1];
    memset(buffer, '6', 128);
    buffer[127] = '\n';

    ret = mmfo->pwrite_file(buffer, 128, 4000);
    // 总感觉这接口很混乱.
    if (ret < 0)
    {
        if (ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr, " pwrite_file: read length is less than required!\n");
        }
        else
        {
            fprintf(stderr, "pwrite faile %s failed. reason: %s\n", filename, strerror(-ret));
        }
    }

    memset(buffer, 0, 128);

    ret = mmfo->pread_file(buffer, 128, 4000);
    if (ret < 0)
    {
        if (ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr, " pread_file: read length is less than required!\n");
        }
        else
        {
            fprintf(stderr, "pread faile %s failed. reason: %s\n", filename, strerror(-ret));
        }
    }
    else
    {
        buffer[128] = '\0';
        printf("read:%s\n", buffer);
    }

    ret = mmfo->flush_file();
    if (ret == largefile::TFS_ERROR)
    {
        fprintf(stderr, "flush file failed. reason: %s\n", strerror(errno));
    }
    else
    {
        printf("flush file %s success\n", filename);
    }

    mmfo->munmap_file();
    mmfo->close_file();


    return 0;
}