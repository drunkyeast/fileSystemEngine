#ifndef QINIU_LARGEFILE_MMAPFILE_H_
#define QINIU_LARGEFILE_MMAPFILE_H_

#include <unistd.h>
#include "common.h"

namespace qiniu
{
    namespace largefile
    {

        struct MMapOption
        {
            int32_t max_mmap_size; // 3M
            int32_t first_mmap_size; // 4k
            int32_t per_mmap_size; // 4k
        };

        // 类的功能: 指定一个文件映射到内存, 同步,权限...

        class MMapFile
        {
        public:
            MMapFile();
            explicit MMapFile(const int fd); // 更清晰. 避免MMapFile a = 10;这种隐式转化.
            MMapFile(const MMapOption& mmap_option, const int fd);
            ~MMapFile();
            bool sync_file(); // 同步文件.
            bool map_file(const bool write = false); // 将文件映射到内存, 同时设置访问权限.
            void* get_data() const; // 获取映射到内存数据的首地址
            int32_t get_size() const; // 获取映射数据的大小

            bool munmap_file(); // 解除映射
            bool remap_file();

        private:
            bool ensure_file_size(const int32_t size);

        private:
            int32_t size_;
            int fd_;
            void* data_;

            struct MMapOption mmap_file_option_;

        };
    }
}



#endif