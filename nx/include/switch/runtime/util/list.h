/**
 * @file list.h
 * @brief Singly Linked List data structure.
 * @author Yordrar
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/condvar.h"
#include "../kernel/mutex.h"

typedef struct node {
    void* item;
    Node* next;
} Node;

typedef struct list {
    Node* header;
    Node* last;
    u32 num_nodes;
    RwLock mutex;
    bool isInited;
} List;

void listInit(List* l);
void listFree(List* l);
void listInsert(List* l, void* item, u32 pos);
static inline void listInsertFirst(List* l, void* item) {
    listInsert(l, item, 0);
}
void listInsertLast(List* l, void* item);
void listDelete(List* l, void* item);
bool listIsInserted(List* l, void* item);
u32 listGetNumNodes(List* l);
void* listGetItem(List* l, u32 pos);