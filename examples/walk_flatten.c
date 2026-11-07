/*
 * Example using walk to flatten
 *
 * SYNOPSIS:
 * $ examples/walk_flatten
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
#include <unistd.h>

#define FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_NONE  0x00
#define FLATTENING_FLAGS_ARRAY_SQUARBACKET_ALL         0x01
#define FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_LEVEL 0x02

#define FLATTENING_FLAGS_DELIMITER_DOT               "."
#define FLATTENING_FLAGS_DELIMITER_SLASH             "/"
#define FLATTENING_FLAGS_DELIMITER_UNDERSCORE        "_"
#define FLATTENING_FLAGS_DELIMITER_DOUBLE_UNDERSCORE "__"

struct flatten_config {
    json_t *root;
    size_t flags;
    const char *delimiter;
};

static const char *flatten_get_delimiter(const struct flatten_config *config) {
    return config->delimiter ? config->delimiter : FLATTENING_FLAGS_DELIMITER_DOT;
}

int array_to_path(const json_t *pathArray, char *buffer, size_t size, size_t flags,
                  const char *delimiter) {
    size_t pos = 0;
    size_t i;
    json_t *obj;
    int written;

    if (size == 0)
        return -1;
    buffer[0] = '\0';

    json_array_foreach(pathArray, i, obj) {
        if (json_is_integer(obj)) {
            const int bracket_index =
                (i > 0) || (flags & FLATTENING_FLAGS_ARRAY_SQUARBACKET_ALL) ||
                (flags & FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_LEVEL);

            if (bracket_index)
                written = snprintf(buffer + pos, size - pos, "[%" JSON_INTEGER_FORMAT "]",
                                   json_integer_value(obj));
            else
                written = snprintf(buffer + pos, size - pos, "%" JSON_INTEGER_FORMAT,
                                   json_integer_value(obj));
        } else if (json_is_string(obj)) {
            if (i)
                written = snprintf(buffer + pos, size - pos, "%s%s", delimiter,
                                   json_string_value(obj));
            else
                written =
                    snprintf(buffer + pos, size - pos, "%s", json_string_value(obj));
        } else {
            abort();
        }

        if (written < 0 || (size_t)written >= size - pos)
            return -1;
        pos += (size_t)written;
    }

    return (int)pos;
}

json_walk_result_t flatten_callback(json_t *obj, struct walk_context *ctx) {
    char buffer[1024];
    struct flatten_config *cfg = ctx->data;
    int length = array_to_path(ctx->path_array, buffer, sizeof(buffer), cfg->flags,
                               flatten_get_delimiter(cfg));

    if (length < 0)
        return JSON_WALK_ERROR;

    if (json_object_setn_nocheck(cfg->root, buffer, (size_t)length, obj) != 0)
        return JSON_WALK_ERROR;

    return JSON_WALK_CONTINUE;
}

json_t *flatten_json(json_t *root, struct flatten_config *config) {
    json_t *new_root = json_object();
    struct walk_context ctx;

    config->root = new_root;
    ctx.flags = JSON_WALK_LEAFS;
    ctx.callback = flatten_callback;
    ctx.data = config;

    if (json_walk_ctx(&ctx, root)) {
        json_decref(new_root);
        return NULL;
    }

    return new_root;
}

static void print_usage(const char *progname) {
    fprintf(stderr,
            "Usage: %s [--array-none|--array-all|--array-first-level] "
            "[--delimiter .|/|_|__|custom]\n"
            "       %s --help\n",
            progname, progname);
}

static int parse_flags(int argc, char **argv, size_t *flags_out,
                       const char **delimiter_out) {
    size_t flags = FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_NONE;
    const char *delimiter = FLATTENING_FLAGS_DELIMITER_DOT;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        }

        if (strcmp(argv[i], "--array-none") == 0 || strcmp(argv[i], "-n") == 0) {
            flags &= ~FLATTENING_FLAGS_ARRAY_SQUARBACKET_ALL;
            flags &= ~FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_LEVEL;
            continue;
        }

        if (strcmp(argv[i], "--array-all") == 0 || strcmp(argv[i], "-a") == 0) {
            flags |= FLATTENING_FLAGS_ARRAY_SQUARBACKET_ALL;
            flags &= ~FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_LEVEL;
            continue;
        }

        if (strcmp(argv[i], "--array-first-level") == 0 || strcmp(argv[i], "-f") == 0) {
            flags |= FLATTENING_FLAGS_ARRAY_SQUARBACKET_FIRST_LEVEL;
            flags &= ~FLATTENING_FLAGS_ARRAY_SQUARBACKET_ALL;
            continue;
        }

        if (strcmp(argv[i], "--delimiter") == 0 || strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: missing delimiter value after %s\n", argv[i]);
                print_usage(argv[0]);
                return -1;
            }

            delimiter = argv[++i];
            if (strcmp(delimiter, ".") == 0) {
                delimiter = FLATTENING_FLAGS_DELIMITER_DOT;
            } else if (strcmp(delimiter, "/") == 0) {
                delimiter = FLATTENING_FLAGS_DELIMITER_SLASH;
            } else if (strcmp(delimiter, "_") == 0) {
                delimiter = FLATTENING_FLAGS_DELIMITER_UNDERSCORE;
            } else if (strcmp(delimiter, "__") == 0) {
                delimiter = FLATTENING_FLAGS_DELIMITER_DOUBLE_UNDERSCORE;
            }
            continue;
        }

        fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
        print_usage(argv[0]);
        return -1;
    }

    *flags_out = flags;
    *delimiter_out = delimiter;
    return 0;
}

int main(int argc, char *argv[]) {
    json_t *root;
    json_error_t error;
    size_t flags;
    const char *delimiter;
    int parse_result = parse_flags(argc, argv, &flags, &delimiter);

    if (parse_result != 0)
        return parse_result == 1 ? 0 : 1;

    if (isatty(STDIN_FILENO)) {
        fprintf(stderr,
                "error: no JSON input provided via stdin. Pipe input to this command.\n");
        print_usage(argv[0]);
        return 1;
    }

    root = json_loadf(stdin, JSON_DECODE_ANY, &error);
    if (root) {
        struct flatten_config config;
        json_t *flatten;

        config.root = NULL;
        config.flags = flags;
        config.delimiter = delimiter;
        flatten = flatten_json(root, &config);
        if (!flatten) {
            fprintf(stderr, "error: failed to flatten JSON input\n");
            json_decref(root);
            return 1;
        }

        json_dumpf(flatten, stdout, JSON_INDENT(2));
        printf("\n");
        json_decref(root);
        json_decref(flatten);
    } else {
        fprintf(stderr, "json error on line %d column %d pos: %d: %s\n", error.line,
                error.column, error.position, error.text);
        return 1;
    }

    return 0;
}