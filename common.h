#ifndef _COMMON_H_INCLUDE_
#define _COMMON_H_INCLUDE_


#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sstream>

namespace qiniu
{
    namespace largefile
    {
        const int32_t TFS_SUCCESS = 0;
        const int32_t TFS_ERROR = -1;
        const int32_t EXIT_DISK_OPER_INCOMPLETE = -8012; // read or write length is less than required.
        const int32_t EXIT_INDEX_ALREADY_LOADED_ERROR = -8013; // index is loaded when create or load.
        const int32_t EXIT_META_UNEXPECTE_FOUND_ERROR = -8014; // meta found in index when insert.
        const int32_t EXIT_INDEX_CORRUPT_ERROR = -8015; // index is corrupt.
        const int32_t EXIT_BLOCKID_CONFLICT_ERROR = -8016; // 
        const int32_t EXIT_BUCKET_CONFIGURE_ERROR = -8017;
        const int32_t EXIT_META_NOT_FOUND_ERROR = -8018;
        const int32_t EXIT_BLOCKID_ZERO_ERROR = -8019;

        static const std::string MAINBLOCK_DIR_PREFIX = "/mainblock/";
        static const std::string INDEX_DIR_PREFIX = "/index/";
        static const mode_t DIR_MODE = 0755; // 000-111-101-101 rwx-r-x-r-x??

        enum OperType
        {
            C_OPER_INSERT = 1,
            C_OPER_DELETE
        };

        struct MMapOption
        {
            int32_t max_mmap_size; // 3M
            int32_t first_mmap_size; // 4k
            int32_t per_mmap_size; // 4k
        };

        struct BlockInfo // 这代码风格很差劲啊. 与我后面学的mordern概念不符合啊.
        {
            uint32_t block_id_;
            int32_t version_;
            int32_t file_count_;
            int32_t size_t_;
            int32_t del_file_count_;
            int32_t del_size_;
            uint32_t seq_no_;

            BlockInfo()
            {
                memset(this, 0, sizeof(BlockInfo)); // C语言中必须带struct, C++中不用.
            }

            inline bool operator==(const BlockInfo& rhs) const
            {
                // 可否用类似内存的操作, 比较每一个byte?
                return block_id_ == rhs.block_id_ && version_ == rhs.version_ && file_count_ == rhs.file_count_
                    && size_t_ == rhs.size_t_ && del_file_count_ == rhs.del_file_count_ && del_size_ == rhs.del_size_
                    && seq_no_ == rhs.seq_no_;
            }
        };

        struct MetaInfo
        {
        public:
            MetaInfo()
            {
                init();
            }

            MetaInfo(const uint64_t file_id, const int32_t in_offset, const int32_t file_size, const int32_t next_meta_offset)
            {
                fileid_ = file_id;
                location_.inner_offset_ = in_offset;
                location_.size_ = file_size;
                next_meta_offset_ = next_meta_offset;
            }

            MetaInfo(const MetaInfo& meta_info)
            {
                // 一个一个赋值... 或用下面的骚操作.
                memcpy(this, &meta_info, sizeof(meta_info));
            }

            MetaInfo& operator=(const MetaInfo& meta_info)
            {
                if (this == &meta_info)
                {
                    return *this;
                }
                memcpy(this, &meta_info, sizeof(meta_info)); // 没必要一个一个手动操作啊.
                return *this;
            }

            MetaInfo& clone(const MetaInfo& meta_info) // 很多程序员喜欢这样的接口来复制.
            {
                assert(this != &meta_info); // <assert.h>
                memcpy(this, &meta_info, sizeof(meta_info));
                return *this;
            }

            bool  operator == (const MetaInfo& rhs) const // const可以防止你把==写成=
            {
                return fileid_ == rhs.fileid_ && location_.inner_offset_ == rhs.location_.inner_offset_
                    && location_.size_ == rhs.location_.size_ && next_meta_offset_ == rhs.next_meta_offset_;
            }


            uint64_t get_key()
            {
                return fileid_;
            }

            void set_key(const uint64_t key)
            {
                fileid_ = key;
            }

            uint64_t get_file_id() const
            {
                return fileid_;
            }

            void set_file_id(const uint64_t file_id)
            {
                fileid_ = file_id;
            }

            int32_t get_offset() const
            {
                return location_.inner_offset_;
            }

            void set_offset(const int32_t offset)
            {
                location_.inner_offset_ = offset;
            }

            int32_t get_size() const
            {
                return location_.size_;
            }

            void set_size(const int32_t file_size)
            {
                location_.size_ = file_size;
            }

            int32_t get_next_meta_offset() const
            {
                return next_meta_offset_;
            }

            void set_next_meta_offset(const int32_t offset)
            {
                next_meta_offset_ = offset;
            }



        private:

            uint64_t fileid_;

            struct { // 匿名结构体，并声明一个名为 location_ 的变量
                int32_t inner_offset_;
                int32_t size_;
            }location_;

            int32_t next_meta_offset_;

        private:
            void init()
            {
                fileid_ = 0;
                location_.inner_offset_ = 0;
                location_.size_ = 0;
                next_meta_offset_ = 0;
            }
        };

    }
}




#endif /* _COMMON_H_INCLUDE_ */