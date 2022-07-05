#ifndef __CHECK_H__DEFINED__
#define __CHECK_H__DEFINED__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHK(expr, is_error)                                                                        \
    do {                                                                                           \
        if ((expr)is_error) {                                                                      \
            perror(#expr);                                                                         \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)
#define CHK_NEG(expr) CHK(expr, < 0)
#define CHK_FALSE(expr) CHK(expr, == 0)
#define CHK_NULL(expr) CHK(expr, == NULL)

#endif // __CHECK_H__DEFINED__
