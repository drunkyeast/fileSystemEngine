#include "mmap_file_op.h"
#include "common.h"

static int debug = 1;

namespace qiniu
{
    namespace largefile
    {
        int MMapFileOperation::mmap_file(const MMapOption& mmap_option)
        {
            if (mmap_option.max_mmap_size < mmap_option.first_mmap_size)
            {
                return TFS_ERROR;
            }

            if (mmap_option.max_mmap_size <= 0)
            {
                return TFS_ERROR;
            }

            int fd = check_file();
            if (fd < 0)
            {
                fprintf(stderr, "MMapFileOperation::mmap_file - checkfile failed!");
                return TFS_ERROR;
            }

            if (!is_mapped_)
            {
                if (map_file_)
                {
                    delete(map_file_);
                }
                map_file_ = new MMapFile(mmap_option, fd);
                is_mapped_ = map_file_->map_file(true);
            }

            if (is_mapped_)
            {
                return TFS_SUCCESS;
            }
            else
            {
                return TFS_ERROR;
            }
        }

        int MMapFileOperation::munmap_file()
        {
            if (is_mapped_ && map_file_ != NULL)
            {
                delete(map_file_); // 这与析构没什么区别呀
                map_file_ = NULL; // 这不应该是好习惯吗
                is_mapped_ = false;
            }
            return TFS_SUCCESS;
        }

        void* MMapFileOperation::get_map_data() const
        {
            if (is_mapped_)
            {
                return map_file_->get_data();
            }
            return NULL;
        }

        int MMapFileOperation::pread_file(char* buf, const int32_t size, const int64_t offset)
        {
            // case1: 内存已经映射
            while (is_mapped_ && (offset + size) > map_file_->get_size()) // 内存不够要扩充
            {
                if (debug)
                {
                    fprintf(stdout, "MMapFileOperation::pread_file, size: %d, offset: %" __PRI64_PREFIX "d, map file size:%d. need remap\n",
                        size, offset, map_file_->get_size());
                    // 这个宏等价于 "ll", 注意C/C++语法会自动拼接多个字符串. 64位系统用%ld, 32位系统用%lld.
                }

                if (map_file_->remap_file() == false) // 不知道怎么处理异常情况.
                {
                    fprintf(stderr, "MMapFileOperation::pread_file, remap_file() failed\n");
                    return -1;
                }
            }


            if (is_mapped_ && (offset + size) <= map_file_->get_size())
            {
                memcpy(buf, (char*)map_file_->get_data() + offset, size); // 返回值不用考虑??
                return TFS_SUCCESS;
            }



            // case2: 内存没有映射或是要读取的数据映射不全. 
            // 这种还是要read, 从磁盘读取而不是从内存读取了, 只是更慢了点.
            return FileOperation::pread_file(buf, size, offset);
        }

        int MMapFileOperation::pwrite_file(const char* buf, const int32_t size, const int64_t offset)
        {
            // case1: 内存已经映射
            while (is_mapped_ && (offset + size) > map_file_->get_size())
            {
                if (debug)
                {
                    fprintf(stdout, "MMapFileOperation::pwrite_file, size: %d, offset: %" __PRI64_PREFIX "d, map file size:%d. need remap\n",
                        size, offset, map_file_->get_size());
                }

                if (map_file_->remap_file() == false)
                {
                    fprintf(stderr, "MMapFileOperation::pread_file, remap_file() failed\n");
                    return -1;
                }
            }

            if (is_mapped_ && (offset + size) <= map_file_->get_size())
            {
                memcpy((char*)map_file_->get_data() + offset, buf, size); // 返回值不用考虑?? 几乎不会失败? 概率问题??
                return TFS_SUCCESS;
            }

            // case2: 内存没有映射或者映射不全
            // 后面梳理了一下项目背景和代码, 就理解了. Martin就是废话太多,不讲关键.
            return FileOperation::pwrite_file(buf, size, offset);

        }

        int MMapFileOperation::flush_file()
        {
            if (is_mapped_)
            {
                if (map_file_->sync_file()) // 同步成功
                {
                    return TFS_SUCCESS;
                }
                else
                {
                    return TFS_ERROR;
                }
            }

            return FileOperation::flush_file();
        }
    }
}