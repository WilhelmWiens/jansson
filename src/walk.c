/*
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "jansson_private.h"

static json_walk_result_t json_walk_call(json_t *json, json_t *path_array,
                                         json_walk_callback_t callback, void *data,
                                         size_t status) {
    json_walk_result_t result = callback(json, path_array, data, status);

    if (result == JSON_WALK_CONTINUE || result == JSON_WALK_BREAK ||
        result == JSON_WALK_STOP || result == JSON_WALK_ERROR)
        return result;
    return JSON_WALK_ERROR;
}

static json_walk_result_t json_walk_intern(json_t *json, json_t *path_array,
                                           json_walk_callback_t callback, void *data,
                                           size_t flags, const size_t status,
                                           hashtable_t *parents);

static json_walk_result_t json_walk_enter(json_t *json, json_t *path_array,
                                          json_walk_callback_t callback, void *data,
                                          size_t flags, const size_t status,
                                          hashtable_t *parents) {
    json_walk_result_t result;
    char loop_key[LOOP_KEY_LEN];
    size_t loop_key_len;
    size_t position_status =
        status & (JSON_WALK_STATUS_FIRST_CHILD | JSON_WALK_STATUS_LAST_CHILD);
    int loop_status;

    if (!json_is_object(json) && !json_is_array(json))
        return json_walk_intern(json, path_array, callback, data, flags, status,
                                parents);

    loop_status = jsonp_loop_check(parents, json, loop_key, LOOP_KEY_LEN, &loop_key_len);
    if (loop_status < 0)
        return JSON_WALK_ERROR;
    if (loop_status > 0) {
        if (!(flags & JSON_WALK_CIRCULAR_REFERENCE))
            return JSON_WALK_ERROR;
        result = json_walk_call(json, path_array, callback, data,
                                position_status | JSON_WALK_STATUS_CIRCULAR_REFERENCE);
        return result == JSON_WALK_BREAK ? JSON_WALK_CONTINUE : result;
    }

    result = json_walk_intern(json, path_array, callback, data, flags, status, parents);
    hashtable_del(parents, loop_key, loop_key_len);
    return result;
}

static json_walk_result_t json_walk_intern(json_t *json, json_t *path_array,
                                           json_walk_callback_t callback, void *data,
                                           size_t flags, const size_t status,
                                           hashtable_t *parents) {
    json_walk_result_t result;

    size_t status_flags =
        status & (JSON_WALK_STATUS_FIRST_CHILD | JSON_WALK_STATUS_LAST_CHILD);

    if (parents->size > JSON_PARSER_MAX_DEPTH)
        return JSON_WALK_ERROR;

    switch (json_typeof(json)) {
        case JSON_OBJECT: {
            const char *key;
            size_t key_len;
            json_t *value;
            size_t child_count = json_object_size(json);
            size_t child_index = 0;
            int skip_children = 0;

            if (flags & JSON_WALK_CONTAINER_PRE) {
                result = json_walk_call(json, path_array, callback, data,
                                        status_flags | JSON_WALK_STATUS_CONTAINER_PRE);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    return result;
                skip_children = result == JSON_WALK_BREAK;
            }

            if (!skip_children) {
                json_object_keylen_foreach(json, key, key_len, value) {
                    json_t *key_json = json_stringn_nocheck(key, key_len);
                    size_t status_flags_child;
                    if (!key_json)
                        return JSON_WALK_ERROR;

                    status_flags_child =
                        ((child_index == 0) ? JSON_WALK_STATUS_FIRST_CHILD : 0) |
                        ((child_index + 1 == child_count) ? JSON_WALK_STATUS_LAST_CHILD
                                                          : 0);
                    child_index++;
                    if (json_array_append_new(path_array, key_json) != 0) {
                        return JSON_WALK_ERROR;
                    }
                    result = json_walk_enter(value, path_array, callback, data, flags,
                                             status_flags_child, parents);
                    json_array_remove(path_array, json_array_size(path_array) - 1);
                    if (result != JSON_WALK_CONTINUE) {
                        return result;
                    }
                }
            }

            if (flags & JSON_WALK_CONTAINER_POST) {
                result = json_walk_call(json, path_array, callback, data,
                                        status_flags | JSON_WALK_STATUS_CONTAINER_POST);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    return result;
            }
            return JSON_WALK_CONTINUE;
        }
        case JSON_ARRAY: {
            json_t *value;
            size_t index;
            size_t child_count = json_array_size(json);
            int skip_children = 0;

            if (flags & JSON_WALK_CONTAINER_PRE) {
                result = json_walk_call(json, path_array, callback, data,
                                        status_flags | JSON_WALK_STATUS_CONTAINER_PRE);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    return result;
                skip_children = result == JSON_WALK_BREAK;
            }

            if (!skip_children) {
                json_array_foreach(json, index, value) {
                    json_t *index_json = json_integer(index);
                    size_t status_flags_child;
                    if (!index_json) {
                        return JSON_WALK_ERROR;
                    }

                    status_flags_child =
                        ((index == 0) ? JSON_WALK_STATUS_FIRST_CHILD : 0) |
                        ((index + 1 == child_count) ? JSON_WALK_STATUS_LAST_CHILD : 0);
                    if (json_array_append_new(path_array, index_json) != 0) {
                        return JSON_WALK_ERROR;
                    }
                    result = json_walk_enter(value, path_array, callback, data, flags,
                                             status_flags_child, parents);
                    json_array_remove(path_array, json_array_size(path_array) - 1);
                    if (result != JSON_WALK_CONTINUE) {
                        return result;
                    }
                }
            }

            if (flags & JSON_WALK_CONTAINER_POST) {
                result = json_walk_call(json, path_array, callback, data,
                                        status_flags | JSON_WALK_STATUS_CONTAINER_POST);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    return result;
            }
            return JSON_WALK_CONTINUE;
        }
        default:
            if (flags & JSON_WALK_LEAFS) {
                result = json_walk_call(json, path_array, callback, data,
                                        status_flags | JSON_WALK_STATUS_LEAF);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    return result;
            }
            return JSON_WALK_CONTINUE;
    }
}

json_walk_result_t json_walk(json_t *json, json_walk_callback_t callback, void *data,
                             size_t flags) {
    json_walk_result_t result;
    json_t *path_array;
    hashtable_t parents;
    size_t valid_flags =
        JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS |
        JSON_WALK_CIRCULAR_REFERENCE;

    if (!json || !callback || (flags & ~valid_flags)) {
        return JSON_WALK_ERROR;
    }

    if (hashtable_init(&parents) != 0)
        return JSON_WALK_ERROR;

    path_array = json_array();
    if (!path_array) {
        hashtable_close(&parents);
        return JSON_WALK_ERROR;
    }

    result = json_walk_enter(json, path_array, callback, data, flags, 0, &parents);

    json_decref(path_array);
    hashtable_close(&parents);

    return result == JSON_WALK_BREAK ? JSON_WALK_CONTINUE : result;
}