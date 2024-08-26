#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define DEFAULT_VEC_CAP 10

#define VEC_DEC(t)                                                             \
    typedef struct {                                                           \
        size_t len;                                                            \
        size_t cap;                                                            \
        t *data;                                                               \
    } vec_##t;                                                                 \
                                                                               \
    void vec_init_##t(vec_##t *vec);                                           \
    void vec_push_##t(vec_##t *vec, t x);                                      \
    bool vec_pop_##t(vec_##t *vec, t *x);

#define VEC_IMPL(t)                                                            \
    void vec_init_##t(vec_##t *vec) {                                          \
        vec->cap = DEFAULT_VEC_CAP;                                            \
        vec->len = 0;                                                          \
        vec->data = (t *)malloc(vec->cap * sizeof(t));                         \
    }                                                                          \
                                                                               \
    void vec_push_##t(vec_##t *vec, t x) {                                     \
        if (vec->len + 1 >= vec->cap) {                                        \
            vec->cap *= 2;                                                     \
            vec->data = (t *)realloc(vec->data, vec->cap * sizeof(t));         \
        }                                                                      \
                                                                               \
        vec->data[vec->len++] = x;                                             \
    }                                                                          \
                                                                               \
    bool vec_pop_##t(vec_##t *vec, t *x) {                                     \
        if (vec->len == 0) {                                                   \
            return false;                                                      \
        }                                                                      \
                                                                               \
        *x = vec->data[--vec->len];                                            \
        return true;                                                           \
    }

VEC_DEC(int)
