#ifndef QINIU_LARGE_FILE_OP_H_
#define QINIU_LARGE_FILE_OP_H_

#include "common.h"

namespace qiniu
{
    namespace largefile // 这样搞更方便管理模块.
    {
        class FileOperation
        {
        public:
            FileOperation(const std::string& file_name, const int open_flags = O_RDWR | O_LARGEFILE);
            ~FileOperation();

            int open_file();
            void close_file();

            int flush_file(); // 把文件立即写入磁盘.

            int unlink_file(); // 删除文件.

            int pread_file(char* buf, const int32_t nbytes, const int64_t offset); // 
            int pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset);

            int write_file(const char* buf, const int32_t nbytes); // seek?

            int64_t get_file_size();

            int ftruncate_file(const int64_t length);
            int seek_file(const int64_t offset);

            int get_fd() const
            {
                return fd_;
            }

        protected:
            int fd_;
            int open_flags_;
            char* file_name_; // 这不符合我的理解: C++尽量用std::string.

        protected:
            int check_file();

        protected:
            static const mode_t OPEN_MODE = 0644;
            static const int MAX_DISK_TIMES = 5; // 有时临时故障, 如系统繁忙...

        };


    }
}




#endif /*QINIU_LARGE_FILE_OP_H_*/