#ifndef EFI_ALLOCA_H
#define EFI_ALLOCA_H

#include <stddef.h>

#if defined(__GNUC__)
    #define alloca __builtin_alloca
#elif defined(_MSC_VER)
    #define alloca(size) _alloca(size)
    void *_alloca(size_t);
#else
    #error Not supported
#endif

#endif // EFI_ALLOCA_H
