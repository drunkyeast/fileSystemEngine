#include "mmap_file.h"
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
static int debug = 1;
namespace qiniu
{
    namespace largefile
    {
        MMapFile::MMapFile() : size_(0), fd_(-1), data_(NULL)
        {

        }
        MMapFile::MMapFile(const int fd) :size_(0), fd_(fd), data_(NULL)
        {

        }
        MMapFile::MMapFile(const MMapOption& mmap_option, const int fd) :size_(0), fd_(fd), data_(NULL)
        {
            mmap_file_option_.max_mmap_size = mmap_option.max_mmap_size;
            mmap_file_option_.first_mmap_size = mmap_option.first_mmap_size;
            mmap_file_option_.per_mmap_size = mmap_option.per_mmap_size;
        }
        MMapFile::~MMapFile()
        {
            if (data_)
            {
                if (debug) printf("mmapfile destruct, fd:%d, maped size: %d, data:%p\n", fd_, size_, data_);
                msync(data_, size_, MS_SYNC);
                munmap(data_, size_);
                // 多做点, 避免并发时的意外发生
                size_ = 0;
                data_ = NULL;
                fd_ = -1;
                mmap_file_option_.max_mmap_size = 0;
                mmap_file_option_.first_mmap_size = 0;
                mmap_file_option_.per_mmap_size = 0;
            }
        }

        bool MMapFile::sync_file()
        {
            if (NULL != data_ && size_ > 0)
            {
                return msync(data_, size_, MS_ASYNC) == 0;
            }
            return true;
        }

        bool MMapFile::map_file(const bool write)
        {
            int flags = PROT_READ;

            if (write)
            {
                flags |= PROT_WRITE;
            }

            if (fd_ < 0)
            {
                return false;
            }

            if (0 == mmap_file_option_.max_mmap_size)
            {
                return false;
            }

            if (size_ < mmap_file_option_.max_mmap_size)
            {
                size_ = mmap_file_option_.first_mmap_size;
            }
            else
            {
                size_ = mmap_file_option_.max_mmap_size;
            }

            if (ensure_file_size(size_) == false)
            {
                fprintf(stderr, "ensure file size falied in map_file, size: %d\n", size_);
                return false;
            }

            data_ = mmap(0, size_, flags, MAP_SHARED, fd_, 0);

            if (MAP_FAILED == data_)
            {
                fprintf(stderr, "mmap file failed: %s", strerror(errno));// 不如用perror简单.
                size_ = 0;
                fd_ = -1;
                data_ = NULL;
                return false;
            }

            if (debug)
            {
                printf("mmap file successed, fd:%d, maped size: %d, data:%p\n", fd_, size_, data_);
            }
            return true;
        }

        void* MMapFile::get_data() const
        {
            return data_;
        }

        int32_t MMapFile::get_size() const
        {
            return size_;
        }

        bool MMapFile::munmap_file()
        {
            if (munmap(data_, size_) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        bool MMapFile::ensure_file_size(const int32_t size)
        {
            struct stat s;
            if (fstat(fd_, &s) < 0)
            {
                fprintf(stderr, "fstat func error, error desc: %s\n", strerror(errno));
                return false;
            }

            if (s.st_size < size)
            {
                if (ftruncate(fd_, size) < 0)
                {
                    fprintf(stderr, "ftruncate func error, error desc: %s\n", strerror(errno));
                    return false;
                }
            }
            return true;
        }

        bool MMapFile::remap_file()
        {
            // 1. 防御性编程
            if (fd_ < 0 || data_ == NULL)
            {
                fprintf(stderr, "mremap not mapped yet\n");
                return false;
            }

            if (size_ == mmap_file_option_.max_mmap_size)
            {
                fprintf(stderr, "already mapped max size, now size: %d, max size:%d\n", size_, mmap_file_option_.max_mmap_size);
                return false;
            }

            int32_t new_size = size_ + mmap_file_option_.per_mmap_size;
            if (new_size > mmap_file_option_.max_mmap_size)
            {
                new_size = mmap_file_option_.max_mmap_size;
            }

            if (ensure_file_size(new_size) == false)
            {
                fprintf(stderr, "ensure file size failed in map_file, size:%d\n", size_);
                return false;
            }

            if (debug)
            {
                printf("mremap start fd: %d, now size: %d, new size: %d, old data%p\n", fd_, size_, new_size, data_);
            }

            void* new_map_data = mremap(data_, size_, new_size, MREMAP_MAYMOVE);
            if (MAP_FAILED == new_map_data)
            {
                fprintf(stderr, "mremap failed. fd: %d, new size: %d, error desc: %s\n", fd_, new_size, strerror(errno));
                return false;
            }
            else
            {
                if (debug)
                {
                    printf("mremap success. fd: %d, now size: %d, new size: %d, old data: %p, new data: %p\n", fd_, size_, new_size, data_, new_map_data);
                }
            }

            data_ = new_map_data;
            size_ = new_size;
            return true;
        }


    }
}