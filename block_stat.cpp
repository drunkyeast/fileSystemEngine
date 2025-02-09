#include "index_handle.h"
#include "common.h"
#include "file_op.h" // 不需要啊, index_handle.h中就有了啊.
#include <sstream> // 我放到了common.h中

using namespace qiniu;

const static largefile::MMapOption mmap_option = { 1024000, 4096, 4096 }; // 内存映射
const static uint32_t main_blocksize = 1024 * 1024 * 64; // 主块文件的大小.
const static uint32_t bucket_size = 1000; // hash bucket
static int32_t block_id = 1;

static int debug = 1;

int main(int argc, char* argv[])
{
    std::string mainblock_path;
    std::string index_path;
    int32_t ret = largefile::TFS_SUCCESS;

    std::cout << "type you block id: " << '\n';
    std::cin >> block_id;
    if (block_id < 1)
    {
        std::cerr << "invalid blockid, exit" << '\n';
        exit(1);
    }



    // 1.加载索引文件
    largefile::IndexHandle* index_handle = new largefile::IndexHandle(".", block_id); // 索引文件句柄

    if (debug)
    {
        printf("Init index ... \n");
    }

    ret = index_handle->load(block_id, bucket_size, mmap_option);

    if (ret != largefile::TFS_SUCCESS)
    {
        fprintf(stderr, "load index %d failed", block_id);
        delete index_handle;
        exit(1);
    }

    delete index_handle;

    return 0;
}

