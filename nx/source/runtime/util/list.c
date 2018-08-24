#include "runtime/util/list.h"
#include <stdlib.h>

void listInit(List* l) {
    rwlockInit(&l->mutex);
    rwlockWriteLock(&l->mutex);
    Node* header = (Node*)malloc(sizeof(Node));
    header->item = NULL;
    header->next = NULL;

    l->header = header;
    l->last = header;
    l->num_nodes = 0;
    rwlockWriteUnlock(&l->mutex);
}

void listFree(List* l) {
    rwlockWriteLock(&l->mutex);
    Node* aux = l->header;
    while(aux != NULL) {
        Node* erase = aux;
        aux = aux->next;
        free(erase);
    }
    l->header = NULL;
    l->last = NULL;
    l->num_nodes = 0;
    rwlockWriteUnlock(&l->mutex);
}

void listInsert(List* l, void* item, u32 pos) {
    rwlockReadLock(&l->mutex);
    if(pos > l->num_nodes || pos < 0) {
        return;
    }
    rwlockReadUnlock(&l->mutex);

    rwlockWriteLock(&l->mutex);
    Node* aux = l->header;
    for(u32 i = pos; i > 0; i--) {
        aux = aux->next;
    }
    Node* new = (Node*)malloc(sizeof(Node));
    new->item = item;
    new->next = aux->next;
    aux->next = new;

    if(pos == l->num_nodes) {
        l->last = new;
    }

    l->num_nodes++;
    rwlockWriteUnlock(&l->mutex);
}

void listInsertLast(List* l, void* item) {
    rwlockWriteLock(&l->mutex);
    Node* new = (Node*)malloc(sizeof(Node));
    new->item = item;
    new->next = NULL;
    l->last->next = new;
    l->last = new;
    rwlockWriteUnlock(&l->mutex);
}

void listDelete(List* l, void* item) {
    rwlockWriteLock(&l->mutex);

    Node* aux = l->header;
    while(aux->next != NULL && aux->next->item != item) {
        aux = aux->next;
    }

    if(aux->next != NULL && aux->next->item == item) {
        Node* delete = aux->next;
        aux->next = delete->next;
        free(delete);
        l->num_nodes--;
    } 

    rwlockWriteUnlock(&l->mutex);
}

void listDeleteAtPos(List* l, u32 pos) {
    rwlockReadLock(&l->mutex);
    if(pos >= l->num_nodes || pos < 0) {
        return;
    }
    Node* aux = l->header;
    for(int i = pos; i > 0; i++) {
        aux = aux->next;
    }
    Node* delete = aux->next;
    aux->next = delete->next;
    free(delete);
    l->num_nodes--;
    rwlockReadUnlock(&l->mutex);
}

bool listIsInserted(List* l, void* item) {
    rwlockReadLock(&l->mutex);
    Node* aux = l->header;
    while(aux != NULL && aux->item != item) {
        aux = aux->next;
    }
    bool result = aux == NULL ? false : true;
    rwlockReadUnlock(&l->mutex);
    return result;
}

u32 listGetNumNodes(List* l) {
    rwlockReadLock(&l->mutex);
    u32 result = l->num_nodes;
    rwlockReadUnlock(&l->mutex);
    return result;
}

void* listGetItem(List* l, u32 pos) {
    rwlockReadLock(&l->mutex);
    if(pos >= l->num_nodes || pos < 0) {
        return NULL;
    }
    Node* aux = l->header->next;
    for(u32 i = pos; i > 0; i--) {
        aux = aux->next;
    }
    void* result = aux->item;
    rwlockReadUnlock(&l->mutex);
    return result;
}

void* listPopFront(List* l) {
    rwlockReadLock(&l->mutex);
    if(l->num_nodes == 0) {
        return NULL;
    }
    void* result = listGetItem(l, 0);
    listDeleteAtPos(l, 0);
    rwlockReadUnlock(&l->mutex);
    return result;
}

void* listPeekFront(List* l) {
    rwlockReadLock(&l->mutex);
    if(l->num_nodes == 0) {
        return NULL;
    }
    void* result = listGetItem(l, 0);
    rwlockReadUnlock(&l->mutex);
    return result;
}

void* listPopBack(List* l) {
    rwlockReadLock(&l->mutex);
    if(l->num_nodes == 0) {
        return NULL;
    }
    void* result = listGetItem(l, l->num_nodes-1);
    listDeleteAtPos(l, l->num_nodes-1);
    rwlockReadUnlock(&l->mutex);
    return result;
}

void* listPeekBack(List* l) {
    rwlockReadLock(&l->mutex);
    if(l->num_nodes == 0) {
        return NULL;
    }
    void* result = listGetItem(l, l->num_nodes-1);
    rwlockReadUnlock(&l->mutex);
    return result;
}