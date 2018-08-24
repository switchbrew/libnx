/**
 * @file list.h
 * @brief Singly Linked List data structure.
 * @author Yordrar
 * @copyright libnx Authors
 */
#pragma once
#include "../../types.h"
#include "../../kernel/condvar.h"
#include "../../kernel/rwlock.h"
#include "../../kernel/thread.h"

typedef struct node {
    void* item;
    struct node* next;
} Node;

typedef struct list {
    Node* header;
    Node* last;
    u32 num_nodes;
    RwLock mutex;
} List;

/**
 * @brief Allocates memory for a list
 * @param l List object
 */
void listInit(List* l);

/**
 * @brief Frees memory allocated for a list
 * @param l List object
 */
void listFree(List* l);

/**
 * @brief Inserts something in a position
 * @param l List object
 * @param item A pointer to the thing you want to insert
 * @param pos The position to insert (0 is the first position)
 */
void listInsert(List* l, void* item, u32 pos);

/**
 * @brief Inserts something in the first position
 * @param l List object
 * @param item A pointer to the thing you want to insert
 */
static inline void listInsertFirst(List* l, void* item) {
    listInsert(l, item, 0);
}

/**
 * @brief Inserts something at the end of the list
 * @param l List object
 * @param item A pointer to the thing you want to insert
 */
void listInsertLast(List* l, void* item);

/**
 * @brief Deletes the node of the list which has the item specified (makes a pointer comparison to ckeck that)
 * @param l List object
 * @param item A pointer to the thing you want to delete
 */
void listDelete(List* l, void* item);

/**
 * @brief Checks if the item is inserted in the list (makes a pointer comparison to ckeck that)
 * @param l List object
 * @param item A pointer to the thing you want to check
 * @return true if the item is in the list, false otherwise
 */
bool listIsInserted(List* l, void* item);

/**
 * @brief Returns the number of items inserted in the list
 * @param l List object
 * @return The number of nodes (the number of inserted things) in the list
 */
u32 listGetNumNodes(List* l);

/**
 * @brief Returns the item inserted in an specified position
 * @param l List object
 * @param pos The position of the item
 * @return A pointer to that item, NULL if it isn't found
 */
void* listGetItem(List* l, u32 pos);

/**
 * @brief Inserts the item in the first position
 * @param l List object
 * @param item A pointer to something to store
 */
static inline void listPushFront(List* l, void* item) {
    listInsert(l, item, 0);
}

/**
 * @brief Returns the item inserted in the first position and deletes it
 * @param l List object
 * @return A pointer to that item, NULL if the list is empty
 */
void* listPopFront(List* l);

/**
 * @brief Returns the item inserted in the first position
 * @param l List object.
 * @return A pointer to that item, NULL if the list is empty
 */
void* listPeekFront(List* l);

/**
 * @brief Inserts the item in the last position
 * @param l List object.
 * @param item A pointer to something you want to insert
 */
static inline void listPushBack(List* l, void* item) {
    listInsertLast(List* l, void* item);
}

/**
 * @brief Returns the item inserted in the last position and deletes it
 * @param l List object.
 * @return A pointer to that item, NULL if the list is empty
 */
void* listPopBack(List* l);

/**
 * @brief Returns the item inserted in the last position
 * @param l List object.
 * @return A pointer to that item, NULL if the list is empty
 */
void* listPeekBack(List* l);