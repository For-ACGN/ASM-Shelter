#ifndef LIST_MD_H
#define LIST_MD_H

#include "go_types.h"

typedef struct {
    void* (*malloc) (uint size);
    void* (*realloc)(void* address, uint size);
    bool  (*free)   (void* address);
} List_Ctx;

typedef struct {
    List_Ctx ctx;

    void* Data;
    uint  Len;
    uint  Cap;
    uint  Unit;
} List;

// List_Init is used to initialize a mesh dynamic list.
void List_Init(List* list, List_Ctx* ctx, uint unit);

// List_Insert is used to insert a element to list.
bool List_Insert(List* list, void* data);

// List_Delete is used to delete element by index.
bool List_Delete(List* list, uint index);

// List_Resize is used to resize list buffer size.
// It will change capacity, it can be smaller than old.
bool List_Resize(List* list, uint cap);

// List_Free is used to free list buffer.
bool List_Free(List* list);

#endif // LIST_MD_H
