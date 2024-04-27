#ifndef WINDOWS_T_H
#define WINDOWS_T_H

#include "go_types.h"

/* 
* Documents:
* https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
* https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
* https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect
* https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
* https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-flushinstructioncache
* https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexa
* https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasemutex
* https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
* https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
*/

#define MEM_COMMIT  0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80

typedef uint  HANDLE;
typedef byte* LPCSTR;

typedef uintptr (*VirtualAlloc)
(
    uintptr lpAddress, uint dwSize, uint32 flAllocationType, uint32 flProtect
);

typedef bool (*VirtualFree)
(
    uintptr lpAddress, uint dwSize, uint32 dwFreeType
);

typedef bool (*VirtualProtect)
(
    uintptr lpAddress, uint dwSize, uint32 flNewProtect, uint32* lpflOldProtect
);

typedef HANDLE (*CreateThread)
(
    uintptr lpThreadAttributes, uint dwStackSize, uintptr lpStartAddress,
    uintptr lpParameter, uint32 dwCreationFlags, uint32* lpThreadId
);

typedef bool (*FlushInstructionCache)
(
    HANDLE hProcess, uintptr lpBaseAddress, uint dwSize
);

typedef HANDLE (*CreateMutexA)
(
    uintptr lpMutexAttributes, bool bInitialOwner, LPCSTR lpName
);

typedef bool (*ReleaseMutex)
(
    HANDLE hMutex
);

typedef uint32 (*WaitForSingleObject)
(
    HANDLE hHandle, uint32 dwMilliseconds
);

typedef bool (*CloseHandle)
(
    HANDLE hObject
);

#endif // WINDOWS_T_H
