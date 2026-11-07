/*
 * Example using walk to traverse a JSON structure and dump to stdout
 *
 * SYNOPSIS:
 * $ examples/walk_dump_minimal
 *
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dump_config_t {
    int indentation;  /* -1 single tab, 0 compact, >0 n spaces */
    int embed;        /* when set the outermost brackets are omitted */
    int ensure_ascii; /* when set all non ascii characters are escaped */
    int escape_slash; /* escape forward slash with backslash */
};

static void print_indent(const struct dump_config_t *cfg, size_t depth) {
    size_t i;

    if (cfg->indentation == -1) {
        for (i = 0; i < depth; i++)
            printf("\t");
    } else if (cfg->indentation > 0)
        printf("%*s", (int)(depth * cfg->indentation), "");
}

json_walk_result_t dump(json_t *obj, json_t *path_array, void *data, size_t status) {
    struct dump_config_t *cfg = data;

    size_t depth = json_array_size(path_array);
    size_t dump_flags = JSON_COMPACT | JSON_ENCODE_ANY |
                        (cfg->ensure_ascii ? JSON_ENSURE_ASCII : 0) |
                        (cfg->escape_slash ? JSON_ESCAPE_SLASH : 0);
    int first_child = (status & JSON_WALK_STATUS_FIRST_CHILD) != 0;
    int last_child = (status & JSON_WALK_STATUS_LAST_CHILD) != 0;

    if (status & JSON_WALK_STATUS_CONTAINER_POST) {
        if ((!cfg->embed || depth > 0) &&
            (json_object_size(obj) > 0 || json_array_size(obj) > 0)) {
            if (cfg->indentation != 0)
                printf("\n");
            print_indent(cfg, depth);
        }
        if (!cfg->embed || depth > 0)
            printf("%c", json_is_object(obj) ? '}' : ']');
        if (depth > 0 && !last_child)
            printf(",");
        return JSON_WALK_CONTINUE;
    }

    if (depth > 0) {
        if (cfg->indentation != 0) {
            if (!(cfg->embed && depth == 1 && first_child))
                printf("\n");
        }
    }
    print_indent(cfg, depth);

    if (depth > 0) {
        json_t *path_item = json_array_get(path_array, depth - 1);
        if (json_is_string(path_item)) {
            if (json_dumpf(path_item, stdout, dump_flags) < 0)
                return JSON_WALK_ERROR;
            if (cfg->indentation != 0)
                printf(": ");
            else
                printf(":");
        }
    }

    if (status & JSON_WALK_STATUS_CONTAINER_PRE) {
        if (!cfg->embed || depth > 0)
            printf("%c", json_is_object(obj) ? '{' : '[');
    } else if ((status & JSON_WALK_STATUS_LEAF) &&
               json_dumpf(obj, stdout, dump_flags) < 0) {
        return JSON_WALK_ERROR;
    }

    if ((status & JSON_WALK_STATUS_LEAF) && depth > 0 && !last_child)
        printf(",");

    return JSON_WALK_CONTINUE;
}

void print_json(json_t *root) {
    struct dump_config_t cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.indentation = 2;

    json_walk(root, dump, &cfg,
              JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS);

    printf("\n");
}

int main(int argc, char *argv[]) {
    json_t *root;
    json_error_t error;
    (void)argc;
    (void)argv;

    root =
        json_loads("{\"a\":0,\"b\":[{},1,2],\"c\":{\"a\":0, \"ø\":\"æ\",\">/\":\"//\"}}",
                   JSON_DECODE_ANY, &error);

    if (root) {
        print_json(root);
        json_decref(root);
    } else {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
    }

    return 0;
}