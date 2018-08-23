#include "runtime/util/list.h"
#include <stdlib.h>

void listInit(List* l) {
    rwlockWriteLock(l->mutex);
    if(l->isInited) {
        return;
    }
    Node* header = (Node*)malloc(sizeof(Node));
    header->item = NULL;
    header->next = NULL;

    l->header = header;
    l->last = header;
    l->num_nodes = 0;
    l->isInited = true;
    rwlockWriteUnlock(l->mutex);
}

void listFree(List* l) {
    rwlockWriteLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    Node* aux = l->header;
    while(aux != NULL) {
        free(aux);
        aux = aux->next;
    }
    l->header = NULL;
    l->last = NULL;
    l->num_nodes = 0;
    l->isInited = false;
    rwlockWriteUnlock(l->mutex);
}

void listInsert(List* l, void* item, u32 pos) {
    rwlockReadLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    if(pos > l->num_nodes || pos < 0) {
        return;
    }
    rwlockReadUnlock(l->mutex);

    rwlockWriteLock(l->mutex);
    Node* aux = l->header;
    for(u32 i = pos; i > 0; i--) {
        aux = aux->next;
    }

    Node* new = (Node*)malloc(sizeof(Node));
    new->item = item;
    new->next = aux->next;
    aux->next = new;

    l->num_nodes++;
    rwlockWriteUnlock(l->mutex);
}

void listInsertLast(List* l, void* item) {
    rwlockWriteLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    Node* new = (Node*)malloc(sizeof(Node));
    new->item = item;
    new->next = NULL;
    
    l->last->next = new;
    l->last = new;
    rwlockWriteUnlock(l->mutex);
}

void listDelete(List* l, void* item) {
    rwlockWriteLock(l->mutex);
    if(!l->isInited) {
        return;
    }

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

    rwlockWriteUnlock(l->mutex);
}

bool listIsInserted(List* l, void* item) {
    rwlockReadLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    Node* aux = l->header;
    while(aux != NULL && aux->item != item) {
        aux = aux->next;
    }
    bool result = aux == NULL ? false : true;
    rwlockReadUnlock(l->mutex);
    return result;
}

u32 listGetNumNodes(List* l) {
    rwlockReadLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    u32 result = l->num_nodes;
    rwlockReadUnlock(l->mutex);
    return result;
}

void* listGetItem(List* l, u32 pos) {
    rwlockReadLock(l->mutex);
    if(!l->isInited) {
        return;
    }
    if(pos >= l->num_nodes || pos < 0) {
        return;
    }
    Node* aux = l->header->next;
    for(u32 i = pos; i > 0; i--) {
        aux = aux->next;
    }
    void* result = aux->item;
    rwlockReadUnlock(l->mutex);
    return result;
}