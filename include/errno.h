#ifndef ERRNO_H
#define ERRNO_H

#include "c_types.h"

typedef uint32 errno;

void  SetLastErrno(errno errno);
errno GetLastErrno();

#define NO_ERROR 0x00000000

// 00，，，，，， module id
// ，，00，，，， error flags
// ，，，，00，， major error id
// ，，，，，，00 minor error id

#define ERR_FLAG_CAN_IGNORE 0x00010000

#define ERR_RUNTIME_INIT_API            (0x01000001)
#define ERR_RUNTIME_ADJUST_PROTECT      (0x01000002)
#define ERR_RUNTIME_UPDATE_PTR          (0x01000003)
#define ERR_RUNTIME_INIT_IAT_HOOKS      (0x01000004)
#define ERR_RUNTIME_FLUSH_INST          (0x01000005)
#define ERR_RUNTIME_START_TRIGGER       (0x01000006)
#define ERR_RUNTIME_DUP_PROCESS_HANDLE  (0x01000101)
#define ERR_RUNTIME_CREATE_GLOBAL_MUTEX (0x01000102)
#define ERR_RUNTIME_CREATE_EVENT_SLEEP  (0x01000103)
#define ERR_RUNTIME_CREATE_EVENT_DONE   (0x01000104)
#define ERR_RUNTIME_CREATE_EVENT_MUTEX  (0x01000105)
#define ERR_RUNTIME_LOCK                (0x01000201)
#define ERR_RUNTIME_UNLOCK              (0x01000202)
#define ERR_RUNTIME_DEFENSE_RT          (0x01000203)

#define ERR_LIBRARY_INIT_API     (0x02000001)
#define ERR_LIBRARY_UPDATE_PTR   (0x02000002)
#define ERR_LIBRARY_INIT_ENV     (0x02000003)
#define ERR_LIBRARY_CLEAN_MODULE (0x02000004|ERR_FLAG_CAN_IGNORE)
#define ERR_LIBRARY_FREE_LIST    (0x02000005|ERR_FLAG_CAN_IGNORE)

#define ERR_MEMORY_INIT_API         (0x03000001)
#define ERR_MEMORY_UPDATE_PTR       (0x03000002)
#define ERR_MEMORY_INIT_ENV         (0x03000003)
#define ERR_MEMORY_ENCRYPT_PAGE     (0x03000004)
#define ERR_MEMORY_DECRYPT_PAGE     (0x03000005)
#define ERR_MEMORY_CLEAN_PAGE       (0x03000006|ERR_FLAG_CAN_IGNORE)
#define ERR_MEMORY_CLEAN_REGION     (0x03000007|ERR_FLAG_CAN_IGNORE)
#define ERR_MEMORY_FREE_PAGE_LIST   (0x03000008|ERR_FLAG_CAN_IGNORE)
#define ERR_MEMORY_FREE_REGION_LIST (0x03000009|ERR_FLAG_CAN_IGNORE)

#define ERR_THREAD_INIT_API        (0x04000001)
#define ERR_THREAD_UPDATE_PTR      (0x04000002)
#define ERR_THREAD_INIT_ENV        (0x04000003)
#define ERR_THREAD_GET_CURRENT_TID (0x04000004)
#define ERR_THREAD_SUSPEND         (0x04000005|ERR_FLAG_CAN_IGNORE)
#define ERR_THREAD_RESUME          (0x04000006|ERR_FLAG_CAN_IGNORE)
#define ERR_THREAD_TERMINATE       (0x04000007|ERR_FLAG_CAN_IGNORE)
#define ERR_THREAD_CLOSE_HANDLE    (0x04000008|ERR_FLAG_CAN_IGNORE)
#define ERR_THREAD_FREE_LIST       (0x04000009|ERR_FLAG_CAN_IGNORE)

#define ERR_RESOURCE_INIT_API    (0x05000001)
#define ERR_RESOURCE_UPDATE_PTR  (0x05000002)
#define ERR_RESOURCE_INIT_ENV    (0x05000003)
#define ERR_RESOURCE_WSA_CLEANUP (0x05000004)

#endif // ERRNO_H
