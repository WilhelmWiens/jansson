/*
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "jansson_private.h"

#include <string.h>

enum walk_frame_flags {
    WALK_FRAME_NEXT_CHILD_FIRST = 1 << 5,
    WALK_FRAME_HAS_PATH = 1 << 6
};

enum walk_frame_state {
    WALK_FRAME_ENTER,
    WALK_FRAME_CHILDREN,
    WALK_FRAME_POST,
    WALK_FRAME_DONE
};

static json_walk_result_t json_walk_call(struct walk_context *ctx, json_t *json) {
    json_walk_result_t result = ctx->callback(json, ctx);

    if (result == JSON_WALK_CONTINUE || result == JSON_WALK_BREAK ||
        result == JSON_WALK_STOP || result == JSON_WALK_ERROR)
        return result;
    return JSON_WALK_ERROR;
}

struct walk_frame {
    json_t *json;
    char loop_key[LOOP_KEY_LEN];
    size_t loop_key_len;
    union {
        void *object_iter;
        size_t array_index;
    } cursor;
    enum walk_frame_state state;
    unsigned char flags;
};

struct walk_stack {
    struct walk_frame *frames;
    size_t size;
    size_t capacity;
};

static int json_walk_stack_push(struct walk_stack *stack, struct walk_frame frame) {
    if (stack->size == stack->capacity) {
        size_t new_capacity;
        struct walk_frame *new_stack;

        if (stack->capacity >
            ((size_t)-1) / 2 / sizeof(*stack->frames)) /* LCOV_EXCL_BR_LINE */
            return 0;                                  /* LCOV_EXCL_LINE */
        new_capacity = stack->capacity * 2;
        new_stack = jsonp_realloc(stack->frames, stack->capacity * sizeof(*stack->frames),
                                  new_capacity * sizeof(*stack->frames));
        if (!new_stack)
            return 0;
        stack->frames = new_stack;
        stack->capacity = new_capacity;
    }

    stack->frames[stack->size++] = frame;
    return 1;
}

static void json_walk_child_done(struct walk_frame *parent, json_t *path_array) {
    if (json_typeof(parent->json) == JSON_OBJECT)
        parent->cursor.object_iter =
            json_object_iter_next(parent->json, parent->cursor.object_iter);
    else
        parent->cursor.array_index++;
    parent->flags &= (unsigned char)~WALK_FRAME_NEXT_CHILD_FIRST;
    json_array_remove(path_array, json_array_size(path_array) - 1);
}

static void json_walk_stack_pop(struct walk_stack *stack, json_t *path_array) {
    struct walk_frame *frame = &stack->frames[stack->size - 1];

    if (frame->flags & WALK_FRAME_HAS_PATH) {
        json_walk_child_done(&stack->frames[stack->size - 2], path_array);
    }
    stack->size--;
}

json_walk_result_t json_walk_ctx(struct walk_context *ctx, json_t *json) {
    struct walk_stack stack = {NULL, 0, 16};
    struct walk_frame root_frame;
    hashtable_t parents;
    int result;

    if (!json || !ctx || !ctx->callback) {
        return -1;
    }

    if (hashtable_init(&parents) != 0)
        return -1;

    ctx->path_array = json_array();
    if (!ctx->path_array) {
        hashtable_close(&parents);
        return -1;
    }

    ctx->status_flag = 0;

    stack.frames = jsonp_malloc(stack.capacity * sizeof(*stack.frames));
    if (!stack.frames) {
        json_decref(ctx->path_array);
        hashtable_close(&parents);
        return -1;
    }

    memset(&root_frame, 0, sizeof(root_frame));
    root_frame.json = json;
    root_frame.state = WALK_FRAME_ENTER;
    root_frame.flags = WALK_FRAME_NEXT_CHILD_FIRST;
    stack.frames[stack.size++] = root_frame;

    result = JSON_WALK_CONTINUE;
    while (stack.size > 0) {
        struct walk_frame *frame = &stack.frames[stack.size - 1];
        json_type type = json_typeof(frame->json);

        switch (frame->state) { /* LCOV_EXCL_BR_LINE */
            case WALK_FRAME_ENTER:
                ctx->status_flag = frame->flags & (JSON_WALK_STATUS_FIRST_CHILD |
                                                   JSON_WALK_STATUS_LAST_CHILD);
                if (type == JSON_OBJECT || type == JSON_ARRAY) {
                    int loop_status = jsonp_loop_check(&parents, frame->json, frame->loop_key, LOOP_KEY_LEN, &frame->loop_key_len);
                    if (loop_status < 0) {
                        result = JSON_WALK_ERROR;
                        goto done;
                    }
                    if (loop_status > 0) {
                        frame->loop_key_len = 0;
                        if (!(ctx->flags & JSON_WALK_CIRCULAR_REFERENCE)) {
                            result = JSON_WALK_ERROR;
                            goto done;
                        }
                        ctx->status_flag |= JSON_WALK_STATUS_CIRCULAR_REFERENCE;
                        result = json_walk_call(ctx, frame->json);
                        if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                            goto done;
                        if (result == JSON_WALK_BREAK)
                            result = JSON_WALK_CONTINUE;
                        frame->state = WALK_FRAME_DONE;
                        break;
                    }

                    if (ctx->flags & JSON_WALK_CONTAINER_PRE) {
                        ctx->status_flag |= JSON_WALK_STATUS_CONTAINER_PRE;
                        result = json_walk_call(ctx, frame->json);
                        if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                            goto done;
                        if (result == JSON_WALK_BREAK) {
                            frame->state = (ctx->flags & JSON_WALK_CONTAINER_POST)
                                               ? WALK_FRAME_POST
                                               : WALK_FRAME_DONE;
                            break;
                        }
                    }
                    if (type == JSON_OBJECT)
                        frame->cursor.object_iter = json_object_iter(frame->json);
                    else
                        frame->cursor.array_index = 0;
                    frame->state = WALK_FRAME_CHILDREN;
                } else {
                    if (ctx->flags & JSON_WALK_LEAFS) {
                        ctx->status_flag |= JSON_WALK_STATUS_LEAF;
                        result = json_walk_call(ctx, frame->json);
                        if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                            goto done;
                    }
                    frame->state = WALK_FRAME_DONE;
                }
                break;

            case WALK_FRAME_CHILDREN: {
                json_t *child;
                json_t *path_component;
                unsigned char child_is_first;
                unsigned char child_is_last;
                struct walk_frame child_frame;

                if (type == JSON_OBJECT) {
                    const char *key;
                    if (!frame->cursor.object_iter) {
                        frame->state = (ctx->flags & JSON_WALK_CONTAINER_POST)
                                           ? WALK_FRAME_POST
                                           : WALK_FRAME_DONE;
                        break;
                    }
                    key = json_object_iter_key(frame->cursor.object_iter);
                    child = json_object_iter_value(frame->cursor.object_iter);
                    child_is_last = json_object_iter_next(
                                        frame->json, frame->cursor.object_iter) == NULL;
                    path_component = json_stringn_nocheck(
                        key, json_object_iter_key_len(frame->cursor.object_iter));
                } else {
                    if (frame->cursor.array_index >= json_array_size(frame->json)) {
                        frame->state = (ctx->flags & JSON_WALK_CONTAINER_POST)
                                           ? WALK_FRAME_POST
                                           : WALK_FRAME_DONE;
                        break;
                    }
                    child = json_array_get(frame->json, frame->cursor.array_index);
                    child_is_last =
                        frame->cursor.array_index + 1 == json_array_size(frame->json);
                    path_component = json_integer(frame->cursor.array_index);
                }

                if (!path_component) {
                    result = JSON_WALK_ERROR;
                    goto done;
                }
                if (json_array_append_new(ctx->path_array, path_component) != 0) {
                    result = JSON_WALK_ERROR;
                    goto done;
                }

                child_is_first = (frame->flags & WALK_FRAME_NEXT_CHILD_FIRST) != 0;
                memset(&child_frame, 0, sizeof(child_frame));
                child_frame.json = child;
                child_frame.state = WALK_FRAME_ENTER;
                child_frame.flags = (child_is_first ? JSON_WALK_STATUS_FIRST_CHILD : 0) |
                                    (child_is_last ? JSON_WALK_STATUS_LAST_CHILD : 0) |
                                    WALK_FRAME_NEXT_CHILD_FIRST | WALK_FRAME_HAS_PATH;
                if (!json_walk_stack_push(&stack, child_frame)) {
                    result = JSON_WALK_ERROR;
                    goto done;
                }
                break;
            }

            case WALK_FRAME_POST:
                ctx->status_flag = frame->flags & (JSON_WALK_STATUS_FIRST_CHILD |
                                                   JSON_WALK_STATUS_LAST_CHILD);
                ctx->status_flag |= JSON_WALK_STATUS_CONTAINER_POST;
                result = json_walk_call(ctx, frame->json);
                if (result == JSON_WALK_ERROR || result == JSON_WALK_STOP)
                    goto done;
                frame->state = WALK_FRAME_DONE;
                break;

            case WALK_FRAME_DONE:
                if (frame->loop_key_len > 0)
                    hashtable_del(&parents, frame->loop_key, frame->loop_key_len);
                json_walk_stack_pop(&stack, ctx->path_array);
                break;
        }
    }

done:
    ctx->status_flag = 0;
    jsonp_free(stack.frames);
    json_decref(ctx->path_array);
    hashtable_close(&parents);

    return result == JSON_WALK_BREAK ? JSON_WALK_CONTINUE : result;
}

json_walk_result_t json_walk(json_t *json, json_walk_callback_t callback, void *data) {
    struct walk_context ctx;
    ctx.callback = callback;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_LEAFS;
    ctx.data = data;

    return json_walk_ctx(&ctx, json);
}