#include "file_op.h"
#include "common.h"

using namespace qiniu;
#define MY_BUFSIZ 64
int main(void)
{
    const char* filename = "./file_op.txt";
    largefile::FileOperation* fileOP = new largefile::FileOperation(filename, O_CREAT | O_RDWR | O_LARGEFILE);
    int fd = fileOP->open_file();
    if (fd < 0)
    {
        fprintf(stderr, "open file %s failed. reason:%s\n", filename, strerror(errno));
        exit(1);
    }

    char buffer[MY_BUFSIZ];
    memset(buffer, '6', MY_BUFSIZ);
    int ret = fileOP->pwrite_file(buffer, MY_BUFSIZ, 25);
    if (ret < 0)
    {
        fprintf(stderr, "pwrite file %s failed. reason: %s\n", filename, strerror(errno));

    }
    memset(buffer, 1, MY_BUFSIZ);
    ret = fileOP->write_file(buffer, MY_BUFSIZ);
    if (ret < 0)
    {
        fprintf(stderr, "pwrite file %s failed. reason: %s\n", filename, strerror(errno));

    }
    fileOP->close_file();

    return 0;
}