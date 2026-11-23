/*
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "util.h"
#include <jansson.h>
#include <stdlib.h>
#include <string.h>

struct walk_event {
    json_t *json;
    size_t path_size;
    size_t status_flag;
};

struct walk_trace {
    struct walk_event events[16];
    size_t count;
    json_t *error_target;
    unsigned char error_post;
    size_t error_count;
    json_t *break_target;
    unsigned char break_post;
    size_t break_count;
    json_walk_result_t break_result;
};

static size_t allocation_count;
static size_t allocation_failure;
static int allocation_failed;
static int allocation_realloc_failed;

static void *walk_malloc(size_t size) {
    if (allocation_count++ == allocation_failure) {
        allocation_failed = 1;
        return NULL;
    }
    return malloc(size);
}

static void *walk_realloc(void *ptr, size_t size) {
    if (allocation_count++ == allocation_failure) {
        allocation_failed = 1;
        allocation_realloc_failed = 1;
        return NULL;
    }
    return realloc(ptr, size);
}

static void walk_free(void *ptr) { free(ptr); }

static json_walk_result_t trace_callback(json_t *json, struct walk_context *ctx) {
    struct walk_trace *trace = (struct walk_trace *)ctx->data;
    struct walk_event *event = &trace->events[trace->count++];
    int container_post = (ctx->status_flag & JSON_WALK_STATUS_CONTAINER_POST) != 0;

    event->json = json;
    event->path_size = json_array_size(ctx->path_array);
    event->status_flag = ctx->status_flag;
    if (json == trace->error_target && container_post == trace->error_post) {
        trace->error_count++;
        return JSON_WALK_ERROR;
    }
    if (json == trace->break_target && container_post == trace->break_post) {
        trace->break_count++;
        return trace->break_result ? trace->break_result : JSON_WALK_BREAK;
    }
    return JSON_WALK_CONTINUE;
}

static json_walk_result_t ignore_callback(json_t *json, struct walk_context *ctx) {
    (void)ctx;
    (void)json;
    return 0;
}

static json_walk_result_t invalid_result_callback(json_t *json,
                                                  struct walk_context *ctx) {
    (void)ctx;
    (void)json;
    return 42;
}

static void test_walk_flags(void) {
    size_t i;
    json_t *root = json_loads(
        "{\"user\": {\"name\": \"alice\"}, \"items\": [10, \"two\"]}", 0, NULL);
    json_t *empty_object = json_object();
    json_t *empty_array = json_array();
    struct walk_trace trace = {0};
    struct walk_context ctx = {0};
    json_t *user;
    json_t *items;
    json_t *name;
    json_t *ten;
    json_t *two;
    json_t *expected_json[9];
    size_t expected_path_sizes[] = {0, 1, 2, 1, 1, 2, 2, 1, 0};
    size_t expected_status_flag[] = {
        JSON_WALK_STATUS_CONTAINER_PRE,
        JSON_WALK_STATUS_FIRST_CHILD | JSON_WALK_STATUS_CONTAINER_PRE,
        JSON_WALK_STATUS_LAST_CHILD | JSON_WALK_STATUS_FIRST_CHILD |
            JSON_WALK_STATUS_LEAF,
        JSON_WALK_STATUS_FIRST_CHILD | JSON_WALK_STATUS_CONTAINER_POST,
        JSON_WALK_STATUS_LAST_CHILD | JSON_WALK_STATUS_CONTAINER_PRE,
        JSON_WALK_STATUS_FIRST_CHILD | JSON_WALK_STATUS_LEAF,
        JSON_WALK_STATUS_LAST_CHILD | JSON_WALK_STATUS_LEAF,
        JSON_WALK_STATUS_LAST_CHILD | JSON_WALK_STATUS_CONTAINER_POST,
        JSON_WALK_STATUS_CONTAINER_POST};

    if (!root)
        fail("json_walk flags: failed to create input");

    user = json_object_get(root, "user");
    items = json_object_get(root, "items");
    name = json_object_get(user, "name");
    ten = json_array_get(items, 0);
    two = json_array_get(items, 1);
    expected_json[0] = root;
    expected_json[1] = user;
    expected_json[2] = name;
    expected_json[3] = user;
    expected_json[4] = items;
    expected_json[5] = ten;
    expected_json[6] = two;
    expected_json[7] = items;
    expected_json[8] = root;

    if (json_walk(root, trace_callback, &trace) != 0 || trace.count != 6)
        fail("json_walk: default traversal mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.callback = trace_callback;
    ctx.data = &trace;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    if (json_walk_ctx(&ctx, root) != 0)
        fail("json_walk flags: traversal failed");

    if (trace.count != sizeof(expected_json) / sizeof(expected_json[0]))
        fail("json_walk flags: expected %zu events, got %zu",
             sizeof(expected_json) / sizeof(expected_json[0]), trace.count);

    for (i = 0; i < trace.count; i++) {
        if (trace.events[i].json != expected_json[i])
            fail("json_walk flags: unexpected json* at index %zu", i);
        if (trace.events[i].path_size != expected_path_sizes[i])
            fail("json_walk flags: unexpected path_sizes at index %zu", i);
        if (trace.events[i].status_flag != expected_status_flag[i])
            fail("json_walk flags: unexpected status_flag at index %zu. exp %zu  got:%zu",
                 i, expected_status_flag[i], trace.events[i].status_flag);
    }

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_POST;
    if (json_walk_ctx(&ctx, root) != 0 || trace.count != 3)
        fail("json_walk flags: postorder-only traversal mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_LEAFS;
    if (json_walk_ctx(&ctx, root) != 0 || trace.count != 3)
        fail("json_walk flags: leaf-only traversal mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = 0;
    if (json_walk_ctx(&ctx, root) != 0 || trace.count != 0)
        fail("json_walk flags: empty traversal mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    if (json_walk_ctx(&ctx, empty_object) != 0 || trace.count != 2 ||
        (trace.events[0].status_flag & JSON_WALK_STATUS_CONTAINER_POST) ||
        !(trace.events[1].status_flag & JSON_WALK_STATUS_CONTAINER_POST))
        fail("json_walk flags: empty object traversal with pre/post mismatch");
    memset(&trace, 0, sizeof(trace));
    if (json_walk_ctx(&ctx, empty_array) != 0 || trace.count != 2 ||
        (trace.events[0].status_flag & JSON_WALK_STATUS_CONTAINER_POST) ||
        !(trace.events[1].status_flag & JSON_WALK_STATUS_CONTAINER_POST))
        fail("json_walk flags: empty array traversal with pre/post mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_POST;
    if (json_walk_ctx(&ctx, empty_object) != 0 || trace.count != 1 ||
        !(trace.events[0].status_flag & JSON_WALK_STATUS_CONTAINER_POST))
        fail("json_walk flags: empty object traversal mismatch");
    memset(&trace, 0, sizeof(trace));
    if (json_walk_ctx(&ctx, empty_array) != 0 || trace.count != 1 ||
        !(trace.events[0].status_flag & JSON_WALK_STATUS_CONTAINER_POST))
        fail("json_walk flags: empty array traversal mismatch");

    if (json_walk_ctx(NULL, root) != -1 || json_walk_ctx(&ctx, NULL) != -1)
        fail("json_walk flags: invalid argument check failed");
    ctx.flags = (size_t)-1;
    if (json_walk_ctx(&ctx, root) != -1)
        fail("json_walk flags: invalid flag check failed");
    ctx.flags = JSON_WALK_CONTAINER_POST;
    ctx.callback = NULL;
    if (json_walk_ctx(&ctx, root) != -1)
        fail("json_walk flags: missing callback check failed");

    json_decref(root);
    json_decref(empty_object);
    json_decref(empty_array);
}

static void test_walk_circular_references(void) {
    json_t *root = json_object();
    json_t *array = json_array();
    json_t *shared = json_object();
    struct walk_context ctx = {0};
    struct walk_trace trace = {0};
    size_t cycle_status = JSON_WALK_STATUS_FIRST_CHILD |
                          JSON_WALK_STATUS_LAST_CHILD |
                          JSON_WALK_STATUS_CIRCULAR_REFERENCE;

    ctx.callback = trace_callback;
    ctx.data = &trace;
    if (!root || !array || json_object_set(root, "loop", array) != 0 ||
        json_array_append(array, root) != 0)
        fail("json_walk circular references: failed to create object cycle");

    ctx.flags = JSON_WALK_CIRCULAR_REFERENCE;
    if (json_walk_ctx(&ctx, root) != JSON_WALK_CONTINUE || trace.count != 1 ||
        trace.events[0].json != root || trace.events[0].path_size != 2 ||
        trace.events[0].status_flag != cycle_status)
        fail("json_walk circular references: object cycle event mismatch");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_PRE;
    if (json_walk_ctx(&ctx, root) != JSON_WALK_ERROR || trace.count != 2 ||
        trace.events[0].json != root || trace.events[1].json != array)
        fail("json_walk circular references: disabled cycle reporting did not error");

    if (json_object_del(root, "loop") != 0 || json_array_remove(array, 0) != 0)
        fail("json_walk circular references: failed to break object cycle");
    json_decref(root);
    json_decref(array);

    if (!shared)
        fail("json_walk circular references: failed to create shared value");
    root = json_object();
    if (!root || json_object_set(root, "a", shared) != 0 ||
        json_object_set(root, "b", shared) != 0)
        fail("json_walk circular references: failed to create shared subtree");
    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CIRCULAR_REFERENCE;
    if (json_walk_ctx(&ctx, root) != JSON_WALK_CONTINUE || trace.count != 3 ||
        (trace.events[0].status_flag & JSON_WALK_STATUS_CIRCULAR_REFERENCE) ||
        (trace.events[1].status_flag & JSON_WALK_STATUS_CIRCULAR_REFERENCE) ||
        (trace.events[2].status_flag & JSON_WALK_STATUS_CIRCULAR_REFERENCE))
        fail("json_walk circular references: shared subtree was reported as a cycle");
    json_decref(root);
    json_decref(shared);
}

static void test_walk_callback_errors(void) {
    size_t case_index;
    json_t *root = json_loads(
        "{\"user\": {\"name\": \"alice\"}, \"items\": [10, \"two\"]}", 0, NULL);
    json_t *targets[5];
    static const unsigned char target_post[] = {0, 0, 1, 1, 0};
    static const size_t event_counts[] = {2, 5, 4, 8, 6};
    struct walk_context ctx = {0};
    struct walk_trace trace = {0};

    if (!root)
        fail("json_walk callback errors: failed to create input");

    targets[0] = targets[2] = json_object_get(root, "user");
    targets[1] = targets[3] = json_object_get(root, "items");
    targets[4] = json_array_get(targets[1], 0);

    ctx.callback = trace_callback;
    ctx.data = &trace;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    for (case_index = 0; case_index < 5; case_index++) {
        memset(&trace, 0, sizeof(trace));
        trace.error_target = targets[case_index];
        trace.error_post = target_post[case_index];

        if (json_walk_ctx(&ctx, root) != JSON_WALK_ERROR)
            fail("json_walk callback errors: error result not propagated for case %zu",
                 case_index);
        if (trace.error_count != 1 || trace.count != event_counts[case_index])
            fail("json_walk callback errors: unexpected callback count for case %zu",
                 case_index);
    }

    json_decref(root);
}

static void test_walk_callback_breaks(void) {
    json_t *root = json_loads("{\"skip\": {\"child\": 1}, \"next\": 2}", 0, NULL);
    json_t *skip;
    json_t *single_child_root;
    json_t *array_root;
    json_t *items;
    struct walk_context ctx = {0};
    struct walk_trace trace = {0};
    int result;

    if (!root)
        fail("json_walk callback breaks: failed to create input");
    skip = json_object_get(root, "skip");
    ctx.callback = trace_callback;
    ctx.data = &trace;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    trace.break_target = skip;
    result = json_walk_ctx(&ctx, root);
    if (result != JSON_WALK_CONTINUE || trace.break_count != 1 || trace.count != 5 ||
        trace.events[1].json != skip ||
        (trace.events[1].status_flag & JSON_WALK_STATUS_CONTAINER_POST) ||
        trace.events[2].json != skip ||
        !(trace.events[2].status_flag & JSON_WALK_STATUS_CONTAINER_POST) ||
        trace.events[3].json != json_object_get(root, "next"))
        fail("json_walk callback breaks: local break did not skip only the current "
             "container");

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_LEAFS;
    trace.break_target = skip;
    result = json_walk_ctx(&ctx, root);
    if (result != JSON_WALK_CONTINUE || trace.break_count != 1 || trace.count != 3 ||
        trace.events[2].json != json_object_get(root, "next"))
        fail("json_walk callback breaks: local break without POST did not skip only "
             "the current container");

    single_child_root = json_loads("{\"skip\": {\"child\": 1}}", 0, NULL);
    if (!single_child_root)
        fail("json_walk callback breaks: failed to create single-child input");
    memset(&trace, 0, sizeof(trace));
    trace.break_target = json_object_get(single_child_root, "skip");
    ctx.flags = JSON_WALK_CONTAINER_PRE;
    result = json_walk_ctx(&ctx, single_child_root);
    if (result != JSON_WALK_CONTINUE || trace.break_count != 1 || trace.count != 2)
        fail("json_walk callback breaks: completed walk returned BREAK");
    json_decref(single_child_root);

    memset(&trace, 0, sizeof(trace));
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    trace.break_target = skip;
    trace.break_post = 1;
    trace.break_result = JSON_WALK_STOP;
    result = json_walk_ctx(&ctx, root);
    if (result != JSON_WALK_STOP || trace.break_count != 1 || trace.count != 4 ||
        trace.events[3].json != skip ||
        !(trace.events[3].status_flag & JSON_WALK_STATUS_CONTAINER_POST))
        fail("json_walk callback breaks: stop from POST did not stop the walk");

    memset(&trace, 0, sizeof(trace));
    trace.break_target = skip;
    trace.break_result = JSON_WALK_STOP;
    result = json_walk_ctx(&ctx, root);
    if (result != JSON_WALK_STOP || trace.break_count != 1 || trace.count != 2)
        fail("json_walk callback breaks: total break did not stop the walk");

    memset(&trace, 0, sizeof(trace));
    trace.break_target = json_object_get(skip, "child");
    trace.break_result = JSON_WALK_STOP;
    result = json_walk_ctx(&ctx, root);
    if (result != JSON_WALK_STOP || trace.break_count != 1 || trace.count != 3)
        fail("json_walk callback breaks: total leaf break did not stop the walk");

    array_root = json_loads("{\"items\": [1, 2], \"next\": 3}", 0, NULL);
    if (!array_root)
        fail("json_walk callback breaks: failed to create array input");
    items = json_object_get(array_root, "items");
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    memset(&trace, 0, sizeof(trace));
    trace.break_target = items;
    result = json_walk_ctx(&ctx, array_root);
    if (result != JSON_WALK_CONTINUE || trace.break_count != 1 || trace.count != 5 ||
        trace.events[2].json != items ||
        !(trace.events[2].status_flag & JSON_WALK_STATUS_CONTAINER_POST) ||
        trace.events[3].json != json_object_get(array_root, "next"))
        fail("json_walk callback breaks: local array break did not skip only its "
             "children");

    memset(&trace, 0, sizeof(trace));
    trace.break_target = items;
    trace.break_result = JSON_WALK_STOP;
    result = json_walk_ctx(&ctx, array_root);
    if (result != JSON_WALK_STOP || trace.break_count != 1 || trace.count != 2)
        fail("json_walk callback breaks: total array break did not stop the walk");

    json_decref(array_root);
    json_decref(root);
}

static void test_walk_invalid_callback_result(void) {
    json_t *root = json_loads("{\"value\": 1, \"nested\": {\"child\": 2}}", 0, NULL);
    struct walk_context ctx = {0};

    if (!root)
        fail("json_walk invalid callback result: failed to create input");

    ctx.callback = invalid_result_callback;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    if (json_walk_ctx(&ctx, root) != JSON_WALK_ERROR)
        fail("json_walk invalid callback result: invalid callback result was not treated "
             "as error");
    if (json_walk(root, invalid_result_callback, NULL) != JSON_WALK_ERROR)
        fail("json_walk invalid callback result: helper wrapper did not propagate "
             "invalid callback errors");

    json_decref(root);
}

static void test_walk_allocation_failures(void) {
    size_t i;
    json_t *root = json_object();
    json_t *cursor = root;
    struct walk_context ctx = {0};
    size_t realloc_failures = 0;

    if (!root)
        fail("json_walk allocation failures: failed to create input");
    for (i = 0; i < 9; i++) {
        json_t *child = json_object();
        if (!child || json_object_set_new(cursor, "nested", child) != 0)
            fail("json_walk allocation failures: failed to create input");
        cursor = child;
    }
    {
        json_t *array = json_array();
        if (!array || json_object_set_new(cursor, "array", array) != 0)
            fail("json_walk allocation failures: failed to create input");
        cursor = array;
    }
    for (i = 0; i < 8; i++) {
        json_t *child = json_array();
        if (!child || json_array_append_new(cursor, child) != 0)
            fail("json_walk allocation failures: failed to create input");
        cursor = child;
    }
    if (json_array_append_new(cursor, json_integer(1)) != 0)
        fail("json_walk allocation failures: failed to create input");

    json_set_alloc_funcs2(walk_malloc, walk_realloc, walk_free);
    ctx.callback = ignore_callback;
    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_LEAFS;

    for (allocation_failure = 0; allocation_failure < 100; allocation_failure++) {
        int result;

        allocation_count = allocation_failed = allocation_realloc_failed = 0;
        result = json_walk_ctx(&ctx, root);
        if (allocation_realloc_failed)
            realloc_failures++;
        if (!allocation_failed) {
            if (result != 0)
                fail("json_walk allocation failures: unexpected walk result");
            break;
        }
        if (result != -1)
            fail("json_walk allocation failures: ignored allocation failure");
    }
    if (allocation_failure == 100)
        fail("json_walk allocation failures: too many allocation sites");

    json_decref(root);

    if (realloc_failures != 3)
        fail("json_walk allocation failures: expected 3 realloc failures, got %zu",
             realloc_failures);
}

static void run_tests() {
    test_walk_flags();
    test_walk_circular_references();
    test_walk_callback_errors();
    test_walk_callback_breaks();
    test_walk_invalid_callback_result();
    test_walk_allocation_failures();
}
