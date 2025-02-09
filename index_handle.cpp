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

            int64_t file_size = file_op_->get_file_size(); // 首先文件并没有创建, 第一次open时才会创建一个空的.
            int ret = 0;
            if (file_size < 0)
            {
                return TFS_ERROR;
            }
            else if (file_size == 0) // 一开始就是0啊
            {
                IndexHeader i_header; // IndexHandle的块头信息, BlockInfo+哈希桶等, 不包括MetaInfo.
                i_header.block_info_.block_id_ = logic_block_id;
                i_header.block_info_.seq_no_ = 1;
                i_header.bucket_size_ = bucket_size;

                i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size * sizeof(int32_t); //32位系统??

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
                fprintf(stderr, "martin 讲得跟坨屎一样, 讲得真的垃圾, 代码也是一坨屎.\n");
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
                printf("load blockid %d index successful. ....\n", logic_block_id);
            }

            return TFS_SUCCESS;

        }

        int IndexHandle::remove(const uint32_t logic_block_id)
        {
            if (is_load_)
            {
                if (logic_block_id != block_info()->block_id_)
                {
                    fprintf(stderr, "remove func error\n");
                    return EXIT_BLOCKID_CONFLICT_ERROR;
                }
            }

            int ret = file_op_->munmap_file();
            if (TFS_SUCCESS != ret)
            {
                return ret;
            }

            ret = file_op_->unlink_file();
            return ret;
        }

        int IndexHandle::flush()
        {
            int ret = file_op_->flush_file();
            if (TFS_SUCCESS != ret)
            {
                fprintf(stderr, "IndexHandle::flush func error\n");
            }
            return ret;
        }

        int32_t IndexHandle::write_segment_meta(const uint64_t key, MetaInfo& meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;

            int ret = hash_find(key, current_offset, previous_offset);
            if (TFS_SUCCESS == ret) // 我们把meta写入到这个新块中, 但这个块居然存在东西(TFS_SUCCESS), 所以错误.
            {
                return EXIT_META_UNEXPECTE_FOUND_ERROR;
            }
            else if (EXIT_META_NOT_FOUND_ERROR != ret)
            {
                return ret;
            }

            // 不存在就写入mete到文件哈希表中 hash_insert(key, previous_offset, meta);
            ret = hash_insert(key, previous_offset, meta);

            return ret;
        }

        int32_t IndexHandle::read_segment_meta(const uint64_t key, MetaInfo& meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;
            // int32_t slot = static_cast<int32_t>(key) % bucket_size();

            int ret = hash_find(key, current_offset, previous_offset);
            if (TFS_SUCCESS == ret)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(meta), current_offset);
                return ret;
            }

            return ret;
        }

        int32_t IndexHandle::delete_segment_meta(const uint64_t key, MetaInfo& meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;
            int ret = hash_find(key, current_offset, previous_offset);
            if (TFS_SUCCESS != ret) // 如果没找到
            {
                printf("Delete Failed. meta not found!!\n");
                return ret;
            }
            // 找到了, 可以删除这个meta
            MetaInfo meta_info;

            ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(meta), current_offset);
            if (TFS_SUCCESS != ret)
            {
                return ret;
            }

            int32_t next_pos = meta_info.get_next_meta_offset();

            if (previous_offset == 0) // 当前meta节点时首结点
            {
                int32_t slot = static_cast<uint32_t>(key) % bucket_size();
                bucket_slot()[slot] = next_pos; // 修改首结点的next_pos

            }
            else
            {
                MetaInfo pre_meta_info;
                ret = file_op_->pread_file(reinterpret_cast<char*>(&pre_meta_info), sizeof(pre_meta_info), previous_offset);
                if (TFS_SUCCESS != ret)
                {
                    return ret;
                }
                pre_meta_info.set_next_meta_offset(next_pos);

                ret = file_op_->pwrite_file(reinterpret_cast<char*>(&pre_meta_info), sizeof(pre_meta_info), previous_offset);
                if (TFS_SUCCESS != ret)
                {
                    return ret;
                }

                update_block_info(C_OPER_DELETE, meta_info.get_size());
                // 除了让上个结点指向下一个节点外, 还要将这个delete的结点放到可重用链表结点.... 
                // 注意这是删除handler部分, 而主块部分的文件在删除文件数量达到20%时, 且在晚上时整理磁盘.

            }

            return TFS_SUCCESS;

        }

        int IndexHandle::update_block_info(const OperType oper_type, const uint32_t modify_size)
        {
            if (block_info()->block_id_ == 0)
            {
                return EXIT_BLOCKID_ZERO_ERROR;
            }

            if (oper_type == C_OPER_INSERT)
            {
                ++block_info()->version_;
                ++block_info()->file_count_;
                ++block_info()->seq_no_;
                block_info()->size_t_ += modify_size;
            }
            else if (oper_type == C_OPER_DELETE)
            {
                ++block_info()->version_; // 它这么定义的
                --block_info()->file_count_;
                block_info()->seq_no_; // 不变
                block_info()->size_t_ -= modify_size;
                ++block_info()->del_file_count_;
                block_info()->del_size_ += modify_size;
            }

            if (debug)
            {
                printf("update_block_info : modify_size: %d......\n", modify_size);
            }

            return TFS_SUCCESS;
        }

        int32_t IndexHandle::hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset)
        {
            int ret = TFS_SUCCESS;
            MetaInfo meta_info;

            current_offset = 0;
            previous_offset = 0;

            // 1.确定key存放再桶(slot)的位置.
            int32_t slot = static_cast<uint32_t>(key) % bucket_size(); // 文件编号 % 桶大小. 逆天强转, TFS源码就有这问题??

            // 2.读取桶首节点存储的第一个节点的偏移量, 如果偏移量为零, 直接返回EXIT_META_NOT_FOUND_ERROR
            // 3.根据偏移量读取存储的metainfo
            // 4.与key进行比较, 相等就设置current_offset和previous_offset并返回TFS_SUCCESS, 否则继续执行.
            // 5.从metainfo中取得下一个节点在文件中的偏移量. 如果偏移量为零直接返回EXIT_META_NOT_FOUND_ERROR(表示链表结尾?)
            int32_t pos = bucket_slot()[slot];

            for (;pos != 0;)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&meta_info), sizeof(MetaInfo), pos);
                if (TFS_SUCCESS != ret)
                {
                    perror("pread_file func error");
                    return ret;
                }

                if (hash_compare(key, meta_info.get_key()))
                {
                    current_offset = pos;
                    return TFS_SUCCESS;
                }

                previous_offset = pos;
                pos = meta_info.get_next_meta_offset();
            }
            return EXIT_META_NOT_FOUND_ERROR;

        }

        int32_t IndexHandle::hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta)
        {
            int ret = TFS_SUCCESS;
            MetaInfo tmp_meta_info;


            // 1. 确定key存放的桶(slot)的位置
            int32_t slot = static_cast<uint32_t>(key) % bucket_size();

            // 2. 确定meta节点存储在文件中的偏移量
            int32_t current_offset = index_header()->index_file_size_;
            index_header()->index_file_size_ += sizeof(MetaInfo);

            // 3. 将meta节点写入索引文件中
            meta.set_next_meta_offset(0);
            ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&meta), sizeof(MetaInfo), current_offset);
            if (TFS_SUCCESS != ret)
            {
                index_header()->index_file_size_ -= sizeof(MetaInfo); // 写失败.
                return ret;
            }

            // 4. 将meta节点插入到哈希链表中
            if (0 != previous_offset)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta_info), sizeof(MetaInfo), previous_offset);
                if (TFS_SUCCESS != ret)
                {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);
                    return ret;
                }
                tmp_meta_info.set_next_meta_offset(current_offset);

                ret = file_op_->pwrite_file(reinterpret_cast<char*>(&tmp_meta_info), sizeof(MetaInfo), previous_offset);
                if (TFS_SUCCESS != ret)
                {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);
                    return ret;
                }
            }
            else // 不存在前一个节点的情况
            {
                bucket_slot()[slot] = current_offset;
            }

            return TFS_SUCCESS;
        }




    }


}