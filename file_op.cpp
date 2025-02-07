#include "file_op.h"
#include "common.h" // 没必要, 头文件就包含了.

// using namespace qiniu::largefile;

namespace qiniu
{
    namespace largefile
    {
        FileOperation::FileOperation(const std::string& file_name, const int open_flags) :fd_(-1), open_flags_(open_flags)
        {
            file_name_ = strdup(file_name.c_str()); // strdup含义: 自动分配内存并复制字符串
        }

        FileOperation::~FileOperation()
        {
            if (fd_ > 0)
            {
                ::close(fd_); // :: 表示调用全局命名空间中的 close 函数，而不是类或命名空间中可能存在的同名函数。
            }

            if (NULL != file_name_)
            {
                free(file_name_);
                file_name_ = NULL;
            }
        }

        int FileOperation::open_file()
        {
            if (fd_ > 0) // 这是什么逻辑? 重复打开?
            {
                ::close(fd_);
                fd_ = -1;
            }

            fd_ = ::open(file_name_, open_flags_, OPEN_MODE);
            if (fd_ < 0)
            {
                return -errno;
            }
            return fd_;

        }

        void FileOperation::close_file()
        {
            if (fd_ < 0)
            {
                return;
            }
            ::close(fd_);
            fd_ = -1;
        }

        int64_t FileOperation::get_file_size()
        {
            int fd = check_file(); // 受保护
            if (fd < 0)
            {
                return -1;
            }
            struct stat statbuf;
            if (fstat(fd, &statbuf) != 0)
            {
                return -1;
            }
            return statbuf.st_size;
        }

        int FileOperation::check_file()
        {
            if (fd_ < 0)
            {
                fd_ = open_file();
            }
            return fd_;
        }

        int FileOperation::ftruncate_file(const int64_t length)
        {
            int fd = check_file();

            if (fd < 0)
            {
                return fd;
            }
            return ftruncate(fd, length); // 在<unistd.h>中 On success, zero is returned.  On error, -1 is returned
            // 这儿也应该进行错误检测的.
        }

        int FileOperation::seek_file(const int64_t offset)
        {
            int fd = check_file();

            if (fd < 0)
            {
                return fd;
            }
            return lseek(fd, offset, SEEK_SET);
            // 也没进行错误检测, 不知道是bug还是一切尽在掌握在.
        }

        int FileOperation::flush_file()
        {
            if (open_flags_ & O_SYNC) // 以同步的方式打开, 必须设置这个flags才能用这个函数. 
            {
                return 0;
            }

            int fd = check_file();
            if (fd < 0)
            {
                return fd;
            }

            return fsync(fd);
        }

        int FileOperation::unlink_file()
        {
            close_file();
            return ::unlink(file_name_);
        }

        int FileOperation::pread_file(char* buf, const int32_t nbytes, const int64_t offset)
        {
            // 系统调用的pread约等于lssek+read, 但前者不会移动指针.
            // return pread64(fd_, buf, nbytes, offset); // 不能这样写.要考虑一些特殊情况.
            int32_t left = nbytes;
            int64_t read_offset = offset;
            int32_t read_len = 0;
            char* p_tmp = buf;

            int i = 0;
            while (left > 0) // 这就是Martin老师实战积累的经验~~~
            {
                i++;
                if (i >= MAX_DISK_TIMES)
                {
                    break;
                }

                if (check_file() < 0)
                {
                    return -errno;
                }

                read_len = ::pread64(fd_, p_tmp, left, read_offset);

                if (read_len < 0)
                {
                    read_len = -errno; // 出现问题, 马上保存更保险, 怕多线程等异常情况覆盖errno.

                    if (EINTR == -read_len || EAGAIN == -read_len) // 小问题,继续
                    {
                        continue;
                    }
                    else if (EBADF == -read_len) // 大问题, bad fd?
                    {
                        fd_ = -1;
                        return read_len;
                    }
                    return read_len;
                }
                else if (0 == read_len)
                {
                    break;
                }

                left -= read_len; // read_len为正数
                p_tmp += read_len;
                read_offset += read_len;

            }
            if (0 != left)
            {
                return EXIT_DISK_OPER_INCOMPLETE;
            }

            return TFS_SUCCESS;
        }


        int FileOperation::pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset)
        {
            int32_t left = nbytes;
            int64_t write_offset = offset;
            int32_t written_len = 0;
            const char* p_tmp = buf; // 经典问题, const修饰char, 这个指针指向的值不能修改, 但指针本身可以修改. 所以后面p_tmp += written_len;是合理的.

            int i = 0;
            while (left > 0) // 这就是Martin老师实战积累的经验~~~
            {
                i++;
                if (i >= MAX_DISK_TIMES)
                {
                    break;
                }

                if (check_file() < 0)
                {
                    return -errno;
                }

                written_len = ::pwrite64(fd_, p_tmp, left, write_offset);

                if (written_len < 0)
                {
                    written_len = -errno; // 出现问题, 马上保存更保险, 怕多线程等异常情况覆盖errno.

                    if (EINTR == -written_len || EAGAIN == -written_len) // 小问题,继续
                    {
                        continue;
                    }
                    else if (EBADF == -written_len)
                    {
                        fd_ = -1;
                        continue;
                    }
                    return written_len;
                }
                else if (0 == written_len)
                {
                    break;
                }

                left -= written_len; // read_len为正数
                p_tmp += written_len;
                write_offset += written_len;

            }
            if (0 != left)
            {
                return EXIT_DISK_OPER_INCOMPLETE;
            }

            return TFS_SUCCESS;
        }

        int FileOperation::write_file(const char* buf, const int32_t nbytes)
        {
            int32_t left = nbytes;
            int32_t written_len = 0;
            const char* p_tmp = buf;

            int i = 0;
            while (left > 0)
            {
                i++;
                if (i >= MAX_DISK_TIMES)
                {
                    break;
                }

                if (check_file() < 0)
                {
                    return -errno;
                }

                written_len = ::write(fd_, p_tmp, left);

                if (written_len < 0)
                {
                    written_len = -errno; // 出现问题, 马上保存更保险, 怕多线程等异常情况覆盖errno.

                    if (EINTR == -written_len || EAGAIN == -written_len) // 小问题,继续
                    {
                        continue;
                    }
                    else if (EBADF == -written_len) // 这个也可以用continue; return written_len也行
                    {
                        fd_ = -1;
                        continue;
                    }
                    return written_len;
                }
                else if (0 == written_len) // 几乎不可能为0, 可以删掉
                {
                    break;
                }

                left -= written_len; // read_len为正数
                p_tmp += written_len;

            }
            if (0 != left)
            {
                return EXIT_DISK_OPER_INCOMPLETE;
            }

            return TFS_SUCCESS;
        }

    }


}

