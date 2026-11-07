/*
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "jansson_private.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "jansson.h"
#include "strbuffer.h"
#include "utf.h"

static json_walk_result_t walk_callback(json_t *json, json_t *path_item,
                                        json_walk_callback_t callback, void *data,
                                        size_t status) {
    json_walk_result_t result = callback(json, path_item, data, status);

    switch (result) {
        case JSON_WALK_CONTINUE:
        case JSON_WALK_BREAK:
        case JSON_WALK_STOP:
        case JSON_WALK_ERROR:
            return result;
        default:
            return JSON_WALK_ERROR;
    }
}

static json_walk_result_t walk_node(json_t *json, json_t *path_item,
                                    json_walk_callback_t callback, void *data,
                                    size_t flags, size_t position_status, size_t depth) {
    json_walk_result_t result;
    int is_container = json_is_object(json) || json_is_array(json);
    int skip_children = 0;

    if (depth >= JSON_PARSER_MAX_DEPTH)
        return JSON_WALK_ERROR;

    if (!is_container) {
        if (flags & JSON_WALK_LEAFS)
            return walk_callback(json, path_item, callback, data,
                                 position_status | JSON_WALK_STATUS_LEAF);
        return JSON_WALK_CONTINUE;
    }

    if (flags & JSON_WALK_CONTAINER_PRE) {
        result = walk_callback(json, path_item, callback, data,
                               position_status | JSON_WALK_STATUS_CONTAINER_PRE);
        if (result == JSON_WALK_STOP || result == JSON_WALK_ERROR)
            return result;
        skip_children = result == JSON_WALK_BREAK;
    }

    if (!skip_children) {
        size_t count =
            json_is_array(json) ? json_array_size(json) : json_object_size(json);
        size_t index = 0;

        if (json_is_array(json)) {
            for (; index < count; index++) {
                json_int_t int_index = (json_int_t)index;
                json_t *item;
                size_t child_status = 0;

                if ((size_t)int_index != index)
                    return JSON_WALK_ERROR;

                item = json_integer(int_index);
                if (!item)
                    return JSON_WALK_ERROR;

                if (index == 0)
                    child_status |= JSON_WALK_STATUS_FIRST_CHILD;
                if (index + 1 == count)
                    child_status |= JSON_WALK_STATUS_LAST_CHILD;

                result = walk_node(json_array_get(json, index), item, callback, data,
                                   flags, child_status, depth + 1);
                json_decref(item);

                if (result == JSON_WALK_STOP || result == JSON_WALK_ERROR)
                    return result;
                if (result == JSON_WALK_BREAK)
                    break;
            }
        } else {
            void *iter = json_object_iter(json);

            while (iter) {
                const char *key = json_object_iter_key(iter);
                size_t key_len = json_object_iter_key_len(iter);
                json_t *item = json_stringn_nocheck(key, key_len);
                void *next = json_object_iter_next(json, iter);
                size_t child_status = 0;

                if (!item)
                    return JSON_WALK_ERROR;

                if (index == 0)
                    child_status |= JSON_WALK_STATUS_FIRST_CHILD;
                if (index + 1 == count)
                    child_status |= JSON_WALK_STATUS_LAST_CHILD;

                result = walk_node(json_object_iter_value(iter), item, callback, data,
                                   flags, child_status, depth + 1);
                json_decref(item);

                if (result == JSON_WALK_STOP || result == JSON_WALK_ERROR)
                    return result;
                if (result == JSON_WALK_BREAK)
                    break;

                iter = next;
                index++;
            }
        }
    }

    if (flags & JSON_WALK_CONTAINER_POST) {
        result = walk_callback(json, path_item, callback, data,
                               position_status | JSON_WALK_STATUS_CONTAINER_POST);
        if (result != JSON_WALK_CONTINUE)
            return result;
    }

    return JSON_WALK_CONTINUE;
}

json_walk_result_t json_walk(json_t *json, json_walk_callback_t callback, void *data,
                             size_t flags) {
    json_walk_result_t result;
    size_t valid_flags =
        JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;

    if (!json || !callback || (flags & ~valid_flags))
        return JSON_WALK_ERROR;

    result = walk_node(json, NULL, callback, data, flags, 0, 0);

    /* BREAK ends the current container; at the root, traversal is complete. */
    return result == JSON_WALK_BREAK ? JSON_WALK_CONTINUE : result;
}