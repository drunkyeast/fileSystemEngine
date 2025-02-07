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

namespace qiniu
{
    namespace largefile
    {
        const int32_t TFS_SUCCESS = 0;
        const int32_t TFS_ERROR = -1;
        const int32_t EXIT_DISK_OPER_INCOMPLETE = -8012; // read or write length is less than required.


    }
}




#endif /* _COMMON_H_INCLUDE_ */