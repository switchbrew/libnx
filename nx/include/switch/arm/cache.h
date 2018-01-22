#pragma once

void armDCacheFlush(void* addr, size_t size);
void armDCacheClean(void* addr, size_t size);
void armICacheInvalidate(void* addr, size_t size);

void armDCacheZero(void* addr, size_t size);
