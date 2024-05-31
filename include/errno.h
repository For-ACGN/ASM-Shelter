#ifndef ERRNO_H
#define ERRNO_H

#include "c_types.h"

// 0x0000，，，， module id
//   ，，，，00，， flags [1 = can skip]
//   ，，，，，，00 error number

typedef uint errno;

#define NO_ERROR 0x00000000

#define ERR_LIBRARY_INIT_API   0x00020001
#define ERR_LIBRARY_UPDATE_PTR 0x00020002
#define ERR_LIBRARY_INIT_ENV   0x00020003
#define ERR_LIBRARY_CLEAN_MOD  0x00020104
#define ERR_LIBRARY_FREE_LIST  0x00020105

#define ERR_MEMORY_INIT_API         0x00030001
#define ERR_MEMORY_UPDATE_PTR       0x00030002
#define ERR_MEMORY_INIT_ENV         0x00030003
#define ERR_MEMORY_ENCRYPT_PAGE     0x00030004
#define ERR_MEMORY_DECRYPT_PAGE     0x00030005
#define ERR_MEMORY_CLEAN_PAGE       0x00030106
#define ERR_MEMORY_CLEAN_REGION     0x00030107
#define ERR_MEMORY_FREE_PAGE_LIST   0x00030108
#define ERR_MEMORY_FREE_REGION_LIST 0x00030109

#define ERR_THREAD_INIT_API        0x00040001
#define ERR_THREAD_UPDATE_PTR      0x00040002
#define ERR_THREAD_INIT_ENV        0x00040003
#define ERR_THREAD_GET_CURRENT_TID 0x00040004
#define ERR_THREAD_SUSPEND         0x00040105
#define ERR_THREAD_RESUME          0x00040106
#define ERR_THREAD_TERMINATE       0x00040107
#define ERR_THREAD_CLOSE_HANDLE    0x00040108
#define ERR_THREAD_FREE_LIST       0x00040109

#endif // ERRNO_H
