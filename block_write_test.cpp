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

    // 2.写入文件到主块文件中
    std::stringstream tmp_stream;
    tmp_stream << "." << largefile::MAINBLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;

    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_RDWR | O_LARGEFILE | O_CREAT);

    char buffer[4096];
    memset(buffer, '6', 4096);

    int32_t data_offset = index_handle->get_block_data_offset();
    uint32_t file_no = index_handle->block_info()->seq_no_;

    ret = mainblock->pwrite_file(buffer, 4096, data_offset);
    if (ret != largefile::TFS_SUCCESS)
    {
        perror("pwrite func error");
        mainblock->close_file();
        delete mainblock;
        delete index_handle;
        exit(1);
    }

    // 3.索引文件中写入MetaInfo
    largefile::MetaInfo meta;
    meta.set_file_id(file_no);
    meta.set_offset(data_offset);
    meta.set_size(sizeof(buffer));

    ret = index_handle->write_segment_meta(meta.get_key(), meta);
    if (ret == largefile::TFS_SUCCESS)
    {
        // 1. 更新块信息
        index_handle->commit_block_data_offset(sizeof(buffer));
        // 2. 更新块信息
        index_handle->update_block_info(largefile::C_OPER_INSERT, sizeof(buffer));

        ret = index_handle->flush();
        if (ret != largefile::TFS_SUCCESS)
        {
            perror("flush func error");
        }
    }
    else
    {
        perror("write_segment_meta func error\n");
    }

    mainblock->close_file();

    delete mainblock;
    delete index_handle;

    return 0;
}

