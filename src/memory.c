/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2011-2012 Basile Starynkevitch <basile@starynkevitch.net>
 *
 * Jansson is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "jansson.h"
#include "jansson_private.h"

/* C89 allows these to be macros */
#undef malloc
#undef realloc
#undef free

/* current context */
static json_context_t jsonp_current_context;

static void jsonp_overwrite_memset(void *ptr, size_t size) {
    if (size == 0 || ptr == NULL)
        return;
    memset(ptr, 0, size);
}

json_context_t *jsonp_context(void) {
    static unsigned char initialized = 0;
    if (initialized)
        return &jsonp_current_context;
    initialized = 1;

    memset(&jsonp_current_context, 0, sizeof(json_context_t));

    jsonp_current_context.memfuncs.malloc_fn = malloc;
    jsonp_current_context.memfuncs.free_fn = free;
    jsonp_current_context.memfuncs.realloc_fn = realloc;
    jsonp_current_context.memfuncs.overwrite_fn = jsonp_overwrite_memset;

    jsonp_current_context.have_bigint = 0;
    jsonp_current_context.have_bigreal = 0;

    return &jsonp_current_context;
}

/* memory functions */
void jsonp_overwrite(void *ptr, size_t size) {
    jsonp_context()->memfuncs.overwrite_fn(ptr, size);
}

void *jsonp_malloc(size_t size) {
    if (!size)
        return NULL;

    return (jsonp_context()->memfuncs.malloc_fn)(size);
}

void jsonp_free(void *ptr) {
    if (!ptr)
        return;

    (jsonp_context()->memfuncs.free_fn)(ptr);
}

void *jsonp_realloc(void *ptr, size_t originalSize, size_t newSize) {
    void *newMemory;

    if (jsonp_context()->memfuncs.realloc_fn)
        return (jsonp_context()->memfuncs.realloc_fn)(ptr, newSize);

    // realloc emulation using malloc and free
    if (newSize == 0) {
        if (ptr != NULL)
            (jsonp_context()->memfuncs.free_fn)(ptr);

        return NULL;
    } else {
        newMemory = (jsonp_context()->memfuncs.malloc_fn)(newSize);

        if ((newMemory != NULL) && (ptr != NULL)) {
            memcpy(newMemory, ptr, (originalSize < newSize) ? originalSize : newSize);

            (jsonp_context()->memfuncs.free_fn)(ptr);
        }

        return newMemory;
    }
}

char *jsonp_strndup(const char *str, size_t len) {
    char *new_str;

    new_str = jsonp_malloc(len + 1);
    if (!new_str)
        return NULL;

    memcpy(new_str, str, len);
    new_str[len] = '\0';
    return new_str;
}

void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn) {
    json_set_alloc_funcs2(malloc_fn, NULL, free_fn);
}

void json_set_alloc_funcs2(json_malloc_t malloc_fn, json_realloc_t realloc_fn,
                           json_free_t free_fn) {
    if (malloc_fn == NULL)
        malloc_fn = malloc;
    if (free_fn == NULL)
        free_fn = free;

    jsonp_context()->memfuncs.malloc_fn = malloc_fn;
    jsonp_context()->memfuncs.free_fn = free_fn;
    if (malloc_fn == malloc && realloc_fn == NULL)
        jsonp_context()->memfuncs.realloc_fn = realloc;
    else
        jsonp_context()->memfuncs.realloc_fn = realloc_fn;
}

void json_get_alloc_funcs(json_malloc_t *malloc_fn, json_free_t *free_fn) {
    if (malloc_fn)
        *malloc_fn = jsonp_context()->memfuncs.malloc_fn;
    if (free_fn)
        *free_fn = jsonp_context()->memfuncs.free_fn;
}
void json_get_alloc_funcs2(json_malloc_t *malloc_fn, json_realloc_t *realloc_fn,
                           json_free_t *free_fn) {
    if (malloc_fn)
        *malloc_fn = jsonp_context()->memfuncs.malloc_fn;
    if (realloc_fn)
        *realloc_fn = jsonp_context()->memfuncs.realloc_fn;
    if (free_fn)
        *free_fn = jsonp_context()->memfuncs.free_fn;
}

void json_set_realloc_func(json_realloc_t realloc_fn) {
    jsonp_context()->memfuncs.realloc_fn = realloc_fn ? realloc_fn : realloc;
}

void json_set_overwrite_func(json_overwrite_t overwrite_fn) {
    jsonp_context()->memfuncs.overwrite_fn =
        overwrite_fn ? overwrite_fn : jsonp_overwrite_memset;
}

void json_set_biginteger_funcs(const json_bigint_funcs_t *functions) {
    json_context_t *ctx = jsonp_context();
    if (!functions) {
        ctx->have_bigint = 0;
        memset(&ctx->bigint, 0, sizeof(json_bigint_funcs_t));
    } else {
        ctx->have_bigint = 1;
        memcpy(&ctx->bigint, functions, sizeof(json_bigint_funcs_t));
    }
}

void json_set_bigreal_funcs(const json_bigreal_funcs_t *functions) {
    json_context_t *ctx = jsonp_context();
    if (!functions) {
        ctx->have_bigreal = 0;
        memset(&ctx->bigreal, 0, sizeof(json_bigreal_funcs_t));
    } else {
        ctx->have_bigreal = 1;
        memcpy(&ctx->bigreal, functions, sizeof(json_bigreal_funcs_t));
    }
}
