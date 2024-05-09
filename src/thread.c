#include "c_types.h"
#include "windows_t.h"
#include "hash_api.h"
#include "list_md.h"
#include "context.h"
#include "random.h"
#include "crypto.h"
#include "thread.h"

// hard encoded address in methods for replace
#ifdef _WIN64
    #define METHOD_ADDR_CREATE_THREAD    0x7FFFFFFFFFFFFF10
    #define METHOD_ADDR_EXIT_THREAD      0x7FFFFFFFFFFFFF11
    #define METHOD_ADDR_SUSPEND_THREAD   0x7FFFFFFFFFFFFF12
    #define METHOD_ADDR_RESUME_THREAD    0x7FFFFFFFFFFFFF13
    #define METHOD_ADDR_TERMINATE_THREAD 0x7FFFFFFFFFFFFF14
    #define METHOD_ADDR_SUSPEND_ALL      0x7FFFFFFFFFFFFF15
    #define METHOD_ADDR_RESUME_ALL       0x7FFFFFFFFFFFFF16
    #define METHOD_ADDR_CLEAN            0x7FFFFFFFFFFFFF17
#elif _WIN32
    #define METHOD_ADDR_CREATE_THREAD    0x7FFFFF10
    #define METHOD_ADDR_EXIT_THREAD      0x7FFFFF11
    #define METHOD_ADDR_SUSPEND_THREAD   0x7FFFFF12
    #define METHOD_ADDR_RESUME_THREAD    0x7FFFFF13
    #define METHOD_ADDR_TERMINATE_THREAD 0x7FFFFF14
    #define METHOD_ADDR_SUSPEND_ALL      0x7FFFFF15
    #define METHOD_ADDR_RESUME_ALL       0x7FFFFF16
    #define METHOD_ADDR_CLEAN            0x7FFFFF17
#endif

typedef struct {
    uint32 threadID;
    HANDLE hThread;
} thread;

typedef struct {
    // API addresses
    CreateThread_t        CreateThread;
    ExitThread_t          ExitThread;
    SuspendThread_t       SuspendThread;
    ResumeThread_t        ResumeThread;
    GetThreadID_t         GetThreadID;
    GetCurrentThreadID_t  GetCurrentThreadID;
    TerminateThread_t     TerminateThread;
    ReleaseMutex_t        ReleaseMutex;
    WaitForSingleObject_t WaitForSingleObject;
    DuplicateHandle_t     DuplicateHandle;
    CloseHandle_t         CloseHandle;

    // store all threads info
    List Threads;
    byte ThreadsKey[CRYPTO_KEY_SIZE];
    byte ThreadsIV [CRYPTO_IV_SIZE];

    // global mutex
    HANDLE Mutex;
} ThreadTracker;

// methods about thread tracker
HANDLE TT_CreateThread(
    uintptr lpThreadAttributes, uint dwStackSize, uintptr lpStartAddress,
    uintptr lpParameter, uint32 dwCreationFlags, uint32* lpThreadId
);
void   TT_ExitThread(uint32 dwExitCode);
uint32 TT_SuspendThread(HANDLE hThread);
uint32 TT_ResumeThread(HANDLE hThread);
bool   TT_TerminateThread(HANDLE hThread, uint32 dwExitCode);
bool   TT_SuspendAll();
bool   TT_ResumeAll();
bool   TT_Clean();

static bool initTrackerAPI(ThreadTracker* tracker, Context* context);
static bool updateTrackerPointers(ThreadTracker* tracker);
static bool updateTrackerPointer(ThreadTracker* tracker, void* method, uintptr address);
static bool initTrackerEnvironment(ThreadTracker* tracker, Context* context);
static bool addThread(ThreadTracker* tracker, uint32 threadID, HANDLE hThread);
static void delThread(ThreadTracker* tracker, uint32 threadID);

ThreadTracker_M* InitThreadTracker(Context* context)
{
    // set structure address
    uintptr address = context->MainMemPage;
    uintptr trackerAddr = address + 2100 + RandUint(address) % 256;
    uintptr moduleAddr  = address + 2700 + RandUint(address) % 256;
    // initialize tracker
    ThreadTracker* tracker = (ThreadTracker*)trackerAddr;
    uint errCode = 0;
    for (;;)
    {
        if (!initTrackerAPI(tracker, context))
        {
            errCode = 0x11;
            break;
        }
        if (!updateTrackerPointers(tracker))
        {
            errCode = 0x12;
            break;
        }
        if (!initTrackerEnvironment(tracker, context))
        {
            errCode = 0x13;
            break;
        }
        break;
    }
    if (errCode != 0x00)
    {
        return (ThreadTracker_M*)errCode;
    }
    // create methods for tracker
    ThreadTracker_M* module = (ThreadTracker_M*)moduleAddr;
    // Windows API hooks
    module->CreateThread    = (CreateThread_t   )(&TT_CreateThread);
    module->ExitThread      = (ExitThread_t     )(&TT_ExitThread);
    module->SuspendThread   = (SuspendThread_t  )(&TT_SuspendThread);
    module->ResumeThread    = (ResumeThread_t   )(&TT_ResumeThread);
    module->TerminateThread = (TerminateThread_t)(&TT_TerminateThread);
    // methods for runtime
    module->ThdSuspendAll = &TT_SuspendAll;
    module->ThdResumeAll  = &TT_ResumeAll;
    module->ThdClean      = &TT_Clean;
    return module;
}

static bool initTrackerAPI(ThreadTracker* tracker, Context* context)
{
    typedef struct { 
        uint hash; uint key; uintptr address;
    } winapi;
    winapi list[] =
#ifdef _WIN64
    {
        { 0x430932D6A2AC04EA, 0x9AF52A6480DA3C93 }, // CreateThread
        { 0x91238A1B4E365AB0, 0x6C621931AE641330 }, // ExitThread
        { 0x3A4D5132CF0D20D8, 0x89E05A81B86A26AE }, // SuspendThread
        { 0xB1917786CE5B5A94, 0x6BC3328C112C6DDA }, // ResumeThread
        { 0x5133BE509803E44E, 0x20498B6AFFAED91B }, // GetThreadId
        { 0x9AF119F551D952CF, 0x5A1B9D61A26B22D7 }, // GetCurrentThreadId
        { 0xFB891A810F1ABF9A, 0x253BBD721EBD81F0 }, // TerminateThread
    };
#elif _WIN32
    {
        { 0xB9D69C9D, 0xCAB90EB6 }, // CreateThread
        { 0x1D1F85DD, 0x41A9BD17 }, // ExitThread
        { 0x26C71141, 0xF3C390BD }, // SuspendThread
        { 0x20FFDC31, 0x1D4EA347 }, // ResumeThread
        { 0xFE77EB3E, 0x81CB68B1 }, // GetThreadId
        { 0x2884E5D9, 0xA933632C }, // GetCurrentThreadId
        { 0xBA134972, 0x295F9DD2 }, // TerminateThread
    };
#endif
    uintptr address;
    for (int i = 0; i < arrlen(list); i++)
    {
        address = FindAPI(list[i].hash, list[i].key);
        if (address == NULL)
        {
            return false;
        }
        list[i].address = address;
    }

    tracker->CreateThread       = (CreateThread_t      )(list[0].address);
    tracker->ExitThread         = (ExitThread_t        )(list[1].address);
    tracker->SuspendThread      = (SuspendThread_t     )(list[2].address);
    tracker->ResumeThread       = (ResumeThread_t      )(list[3].address);
    tracker->GetThreadID        = (GetThreadID_t       )(list[4].address);
    tracker->GetCurrentThreadID = (GetCurrentThreadID_t)(list[5].address);
    tracker->TerminateThread    = (TerminateThread_t   )(list[6].address);

    tracker->ReleaseMutex        = context->ReleaseMutex;
    tracker->WaitForSingleObject = context->WaitForSingleObject;
    tracker->DuplicateHandle     = context->DuplicateHandle;
    tracker->CloseHandle         = context->CloseHandle;
    return true;
}

static bool updateTrackerPointers(ThreadTracker* tracker)
{
    typedef struct {
        void* address; uintptr pointer;
    } method;
    method methods[] = 
    {
        { &TT_CreateThread,    METHOD_ADDR_CREATE_THREAD },
        { &TT_ExitThread,      METHOD_ADDR_EXIT_THREAD },
        { &TT_SuspendThread,   METHOD_ADDR_SUSPEND_THREAD },
        { &TT_ResumeThread,    METHOD_ADDR_RESUME_THREAD },
        { &TT_TerminateThread, METHOD_ADDR_TERMINATE_THREAD },
        { &TT_SuspendAll,      METHOD_ADDR_SUSPEND_ALL },
        { &TT_ResumeAll,       METHOD_ADDR_RESUME_ALL },
        { &TT_Clean,           METHOD_ADDR_CLEAN},
    };
    bool success = true;
    for (int i = 0; i < arrlen(methods); i++)
    {
        if (!updateTrackerPointer(tracker, methods[i].address, methods[i].pointer))
        {
            success = false;
            break;
        }
    }
    return success;
}

static bool updateTrackerPointer(ThreadTracker* tracker, void* method, uintptr address)
{
    bool success = false;
    uintptr target = (uintptr)method;
    for (uintptr i = 0; i < 64; i++)
    {
        uintptr* pointer = (uintptr*)(target);
        if (*pointer != address)
        {
            target++;
            continue;
        }
        *pointer = (uintptr)tracker;
        success = true;
        break;
    }
    return success;
}

static bool initTrackerEnvironment(ThreadTracker* tracker, Context* context)
{
    // initialize thread list
    List_Ctx ctx = {
        .malloc  = context->malloc,
        .realloc = context->realloc,
        .free    = context->free,
    };
    List_Init(&tracker->Threads, &ctx, sizeof(thread));
    RandBuf(&tracker->ThreadsKey[0], CRYPTO_KEY_SIZE);
    RandBuf(&tracker->ThreadsIV[0], CRYPTO_IV_SIZE);
    // add current thread for special executable file like Golang
    uint32 threadID = tracker->GetCurrentThreadID();
    if (threadID == 0)
    {
        return false;
    }
    if (!addThread(tracker, threadID, CURRENT_THREAD))
    {
        return false;
    }
    tracker->Mutex = context->Mutex;
    return true;
}

// updateTrackerPointers will replace hard encode address to the actual address.
// Must disable compiler optimize, otherwise updateTrackerPointer will fail.
#pragma optimize("", off)
static ThreadTracker* getTrackerPointer(uintptr pointer)
{
    return (ThreadTracker*)(pointer);
}
#pragma optimize("", on)

__declspec(noinline)
HANDLE TT_CreateThread(
    uintptr lpThreadAttributes, uint dwStackSize, uintptr lpStartAddress,
    uintptr lpParameter, uint32 dwCreationFlags, uint32* lpThreadId
)
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_CREATE_THREAD);

    if (tracker->WaitForSingleObject(tracker->Mutex, INFINITE) != WAIT_OBJECT_0)
    {
        return NULL;
    }

    uint32 threadID;
    HANDLE hThread;

    bool success = true;
    for (;;)
    {
        hThread = tracker->CreateThread(
            lpThreadAttributes, dwStackSize, lpStartAddress,
            lpParameter, dwCreationFlags, &threadID
        );
        if (hThread == NULL)
        {
            success = false;
            break;
        }
        if (!addThread(tracker, threadID, hThread))
        {
            success = false;
            break;
        }
        break;
    }

    tracker->ReleaseMutex(tracker->Mutex);

    if (!success)
    {
        return NULL;
    }
    if (lpThreadId != NULL)
    {
        *lpThreadId = threadID;
    }
    return hThread;
}

static bool addThread(ThreadTracker* tracker, uint32 threadID, HANDLE hThread)
{
    // duplicate thread handle
    HANDLE dupHandle;
    if (!tracker->DuplicateHandle(
        CURRENT_PROCESS, hThread, CURRENT_PROCESS, &dupHandle,
        0, false, DUPLICATE_SAME_ACCESS
    ))
    {
        tracker->CloseHandle(hThread);
        return false;
    }
    thread thread = {
        .threadID = threadID,
        .hThread  = dupHandle,
    };
    if (!List_Insert(&tracker->Threads, &thread))
    {
        tracker->CloseHandle(hThread);
        tracker->CloseHandle(dupHandle);
        return false;
    }
    return true;
}

__declspec(noinline)
void TT_ExitThread(uint32 dwExitCode)
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_EXIT_THREAD);

    if (tracker->WaitForSingleObject(tracker->Mutex, INFINITE) != WAIT_OBJECT_0)
    {
        return;
    }

    uint32 threadID = tracker->GetCurrentThreadID();
    if (threadID != 0)
    {
        delThread(tracker, threadID);
    }

    tracker->ReleaseMutex(tracker->Mutex);

    tracker->ExitThread(dwExitCode);
}

static void delThread(ThreadTracker* tracker, uint32 threadID)
{
    List* threads = &tracker->Threads;
    thread thread = {
        .threadID = threadID,
    };
    uint index;
    if (!List_Find(threads, &thread, sizeof(thread.threadID), &index))
    {
        return;
    }
    if (List_Delete(threads, index))
    {
        return;
    }
    tracker->CloseHandle(thread.hThread);
}

__declspec(noinline)
uint32 TT_SuspendThread(HANDLE hThread)
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_SUSPEND_THREAD);

    if (tracker->WaitForSingleObject(tracker->Mutex, INFINITE) != WAIT_OBJECT_0)
    {
        return -1;
    }
   
    uint32 count;

    uint32 threadID = tracker->GetThreadID(hThread);
    if (threadID == tracker->GetCurrentThreadID() || threadID == 0)
    {
        tracker->ReleaseMutex(tracker->Mutex);
        count = tracker->SuspendThread(hThread);
    } else {
        count = tracker->SuspendThread(hThread);
        tracker->ReleaseMutex(tracker->Mutex);
    }

    return count;
}

__declspec(noinline)
uint32 TT_ResumeThread(HANDLE hThread)
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_RESUME_THREAD);

    if (tracker->WaitForSingleObject(tracker->Mutex, INFINITE) != WAIT_OBJECT_0)
    {
        return -1;
    }

    uint32 count = tracker->ResumeThread(hThread);

    tracker->ReleaseMutex(tracker->Mutex);

    return count;
}

__declspec(noinline)
bool TT_TerminateThread(HANDLE hThread, uint32 dwExitCode)
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_TERMINATE_THREAD);

    if (tracker->WaitForSingleObject(tracker->Mutex, INFINITE) != WAIT_OBJECT_0)
    {
        return false;
    }

    uint32 threadID = tracker->GetThreadID(hThread);
    if (threadID != 0)
    {
        delThread(tracker, threadID);
    }

    tracker->ReleaseMutex(tracker->Mutex);

    return tracker->TerminateThread(hThread, dwExitCode);
}

__declspec(noinline)
bool TT_SuspendAll()
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_SUSPEND_ALL);

    uint32 currentTID = tracker->GetCurrentThreadID();
    if (currentTID == 0)
    {
        return false;
    }

    bool error = false;

    List* threads = &tracker->Threads;
    uint  index   = 0;
    for (uint num = 0; num < threads->Len; index++)
    {
        thread* thread = List_Get(threads, index);
        if (thread->threadID == NULL)
        {
            continue;
        }
        // skip self thread
        if (thread->threadID == currentTID)
        {
            num++;
            continue;
        }
        uint32 count = tracker->SuspendThread(thread->hThread);
        if (count == -1)
        {
            delThread(tracker, thread->threadID);
            error = true;
        }
        num++;
    }

    // TODO encrypt thread list
    return !error;
}

__declspec(noinline)
bool TT_ResumeAll()
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_RESUME_ALL);

    // TODO decrypt thread list

    uint32 currentTID = tracker->GetCurrentThreadID();
    if (currentTID == 0)
    {
        return false;
    }

    bool error = false;

    List* threads = &tracker->Threads;
    uint  index   = 0;
    for (uint num = 0; num < threads->Len; index++)
    {
        thread* thread = List_Get(threads, index);
        if (thread->threadID == NULL)
        {
            continue;
        }
        // skip self thread
        if (thread->threadID == currentTID)
        {
            num++;
            continue;
        }
        uint32 count = tracker->ResumeThread(thread->hThread);
        if (count == -1)
        {
            delThread(tracker, thread->threadID);
            error = true;
        }
        num++;
    }
    return !error;
}

__declspec(noinline)
bool TT_Clean()
{
    ThreadTracker* tracker = getTrackerPointer(METHOD_ADDR_CLEAN);

    uint32 currentTID = tracker->GetCurrentThreadID();
    if (currentTID == 0)
    {
        return false;
    }

    bool error = false;

    List* threads = &tracker->Threads;
    uint  index   = 0;
    for (uint num = 0; num < threads->Len; index++)
    {
        thread* thread = List_Get(threads, index);
        if (thread->threadID == NULL)
        {
            continue;
        }
        // skip self thread
        if (thread->threadID != currentTID)
        {
            uint32 count = tracker->TerminateThread(thread->hThread, 0);
            if (count == -1)
            {
                delThread(tracker, thread->threadID);
                error = true;
            }
        }
        if (!tracker->CloseHandle(thread->hThread))
        {
            error = true;
        }
        num++;
    }

    // clean thread list
    RandBuf(threads->Data, List_Size(threads));
    return !error;
}
