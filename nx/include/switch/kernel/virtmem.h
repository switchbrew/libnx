#pragma once
#include <switch/types.h>

void* virtmemReserve(size_t size);
void  virtmemFree(void* addr, size_t size);

void* virtmemReserveMap(size_t size);
void  virtmemFreeMap(void* addr, size_t size);
