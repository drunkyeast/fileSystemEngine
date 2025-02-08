#include "common.h"
#include "index_handle.h"

static int debug = 1;
namespace qiniu
{
    namespace largefile
    {
        IndexHandle::IndexHandle(const std::string& base_path, const uint32_t main_block_id)
        {
            // create file_op_handle object
            std::stringstream tmp_stream;
            tmp_stream << base_path << INDEX_DIR_PREFIX << main_block_id;  // /root/index/1

            std::string index_path;
            tmp_stream >> index_path;

            file_op_ = new MMapFileOperation(index_path, O_CREAT | O_RDWR | O_LARGEFILE);
            is_load_ = false;
        }
        IndexHandle::~IndexHandle()
        {
            if (file_op_)
            {
                delete file_op_;
                file_op_ = NULL;
            }
        }

        int IndexHandle::create(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option)
        {
            if (debug)
            {
                printf("create index, block id: %u, bucket size: %d, max_mmap_size:%d, first mmap size: %d, per mmap size:%d\n",
                    logic_block_id, bucket_size, map_option.max_mmap_size, map_option.first_mmap_size, map_option.per_mmap_size);
            }
            if (is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }

            int64_t file_size = file_op_->get_file_size();
            int ret = 0;
            if (file_size < 0)
            {
                return TFS_ERROR;
            }
            else if (file_size == 0)
            {
                IndexHeader i_header;
                i_header.block_info_.block_id_ = logic_block_id;
                i_header.block_info_.seq_no_ = 1;
                i_header.bucket_size_ = bucket_size;

                i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size * sizeof(int32_t);

                // index header + total buckets
                char* init_data = new char[i_header.index_file_size_];
                memcpy(init_data, &i_header, sizeof(IndexHeader));
                memset(init_data + sizeof(IndexHeader), 0, i_header.index_file_size_ - sizeof(IndexHeader));

                // write index header and buckets into index file
                ret = file_op_->pwrite_file(init_data, i_header.index_file_size_, 0);

                delete[] init_data;
                init_data = NULL;

                if (ret != TFS_SUCCESS)
                {
                    return ret;
                }
                ret = file_op_->flush_file();
                if (ret != TFS_SUCCESS)
                {
                    return ret;
                }
            }
            else if (file_size > 0) // index already exist
            {
                return EXIT_META_UNEXPECTE_FOUND_ERROR;
            }

            ret = file_op_->mmap_file(map_option);
            if (ret != TFS_SUCCESS)
            {
                return ret;
            }

            is_load_ = true;

            if (debug)
            {
                // printf("init blockid: %d index successful. data file size: %d, index file size: %d"
                //     ", bucket_size: %d, free head offset: %d, seqno: %d, size: %d, "
                //     "filecount: %d, del_size: %d, "
                //     "del_file_count: %d, version: %d\n",
                //     logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_,
                //     index_header()->bucket_size_, index_header()->free_head_offset_, ...
                //     );


            }

            return TFS_SUCCESS;

        }



        int IndexHandle::load(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option)
        {
            int ret = TFS_SUCCESS;

            if (is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }

            int64_t file_size = file_op_->get_file_size();
            if (file_size < 0)
            {
                return file_size;
            }
            else if (file_size == 0)
            {
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            MMapOption tmp_map_option = map_option;

            if (file_size > tmp_map_option.first_mmap_size && file_size <= tmp_map_option.max_mmap_size)
            {
                tmp_map_option.first_mmap_size = file_size;
            }

            ret = file_op_->mmap_file(tmp_map_option); // 把文件映射到内存

            if (ret != TFS_SUCCESS)
            {
                return ret;
            }

            if (0 == this->bucket_size() || 0 == block_info()->block_id_)
            {
                fprintf(stderr, "Index corrupt error. blockid: %u, bucket size: %d\n", block_info()->block_id_, this->bucket_size());
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            // check file size
            int32_t index_file_size = sizeof(IndexHandle) + bucket_size * sizeof(int32_t);
            if (file_size < index_file_size)
            {
                fprintf(stderr, "Index corrupt error, blockid: %u, bucket size: %d, file size%d, index file size: %d\n",
                    block_info()->block_id_, this->bucket_size(), file_size, index_file_size);
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            // check block size
            if (logic_block_id != block_info()->block_id_)
            {
                fprintf(stderr, "martin 讲得跟坨屎一样, 讲得真的垃圾.\n");
                return EXIT_BLOCKID_CONFLICT_ERROR;
            }

            // check bucket size
            if (bucket_size != this->bucket_size())
            {
                fprintf(stderr, "Index configure error, old bucket size: %d, new bucket size: %d\n", this->bucket_size(), bucket_size);
                return EXIT_BUCKET_CONFIGURE_ERROR;

            }

            is_load_ = true;

            if (debug)
            {
                printf("load blockid %d index successful. ....", logic_block_id);
            }

            return TFS_SUCCESS;

        }

    }


}