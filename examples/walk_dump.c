/*
 * Example using walk to traverse a JSON structure and dump to FILE stream using
 * callbackfunction
 *
 * SYNOPSIS:
 * $ echo '{"example": true}' | examples/walk_dump [options]
 * $ examples/walk_dump --help
 *
 * Copyright (c) 2026 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <getopt.h>
#include <jansson.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

enum output_format { OUTPUT_COMPACT, OUTPUT_PRETTY, OUTPUT_SEMI_PRETTY };

#define SEMI_PRETTY_LINE_WIDTH 80

static const struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"compact", no_argument, NULL, 'c'},
    {"pretty", no_argument, NULL, 'p'},
    {"semi-pretty", no_argument, NULL, 'P'},
    {"indent", required_argument, NULL, 'i'},
    {"tab", no_argument, NULL, 't'},
    {"embed", no_argument, NULL, 'e'},
    {"ensure-ascii", no_argument, NULL, 'a'},
    {"escape-slash", no_argument, NULL, 'E'},
    {"sort-keys", no_argument, NULL, 's'},
    {"real-precision", required_argument, NULL, 'r'},
    {"color", no_argument, NULL, 'C'},
    {"no-color", no_argument, NULL, 'M'},
    {NULL, 0, NULL, 0},
};

struct dump_config_t {
    int indentation;
    enum output_format format;
    int embed;
    int ensure_ascii;
    int escape_slash;
    int color;
    int failed;
    int force_inline;
    int real_precision;
    unsigned char *inline_stack;
    size_t inline_stack_capacity;
    json_dump_callback_t output;
    void *output_data;
};

static int emit_data(struct dump_config_t *cfg, const char *data, size_t size) {
    return cfg->output(data, size, cfg->output_data);
}

static int emit_text(struct dump_config_t *cfg, const char *text) {
    return emit_data(cfg, text, strlen(text));
}

static int emit_char(struct dump_config_t *cfg, char character) {
    return emit_data(cfg, &character, 1);
}

static int emit_repeated(struct dump_config_t *cfg, char character, size_t count) {
    char buffer[64];
    memset(buffer, character, sizeof(buffer));

    while (count > 0) {
        size_t chunk = count < sizeof(buffer) ? count : sizeof(buffer);
        if (emit_data(cfg, buffer, chunk) < 0)
            return -1;
        count -= chunk;
    }
    return 0;
}

static int emit_indentation(struct dump_config_t *cfg, size_t depth) {
    if (cfg->indentation == -1)
        return emit_repeated(cfg, '\t', depth);
    if (cfg->indentation > 0)
        return emit_repeated(cfg, ' ', depth * (size_t)cfg->indentation);
    return 0;
}

static json_walk_result_t output_error(struct dump_config_t *cfg) {
    cfg->failed = 1;
    return JSON_WALK_ERROR;
}

static size_t dump_flags(const struct dump_config_t *cfg) {
    size_t flags = JSON_ENCODE_ANY;
    if (cfg->ensure_ascii)
        flags |= JSON_ENSURE_ASCII;
    if (cfg->escape_slash)
        flags |= JSON_ESCAPE_SLASH;
    if (cfg->real_precision >= 0)
        flags |= JSON_REAL_PRECISION(cfg->real_precision);
    return flags;
}

struct width_measure {
    size_t columns;
    size_t limit;
    mbstate_t state;
    int exceeded;
};

static int add_width(struct width_measure *measure, size_t width) {
    if (width > SIZE_MAX - measure->columns ||
        measure->columns + width > measure->limit) {
        measure->exceeded = 1;
        return -1;
    }
    measure->columns += width;
    return 0;
}

static json_walk_result_t measure_output_callback(const char *text, size_t size,
                                                  void *data) {
    struct width_measure *measure = data;
    size_t offset = 0;

    while (offset < size) {
        wchar_t character = L'\0';
        size_t length =
            mbrtowc(&character, text + offset, size - offset, &measure->state);
        size_t width;

        if (length == (size_t)-1) {
            memset(&measure->state, 0, sizeof(measure->state));
            length = 1;
            width = 1;
        } else if (length == (size_t)-2) {
            break;
        } else if (length == 0) {
            length = 1;
            width = 1;
        } else {
            int character_width = wcwidth(character);
            width = character_width < 0 ? 1 : (size_t)character_width;
        }

        if (character == L'\n') {
            offset += length;
            continue;
        }
        if (add_width(measure, width) < 0)
            return JSON_WALK_ERROR;
        offset += length;
    }
    return JSON_WALK_CONTINUE;
}

static int print_json_callback(json_t *root, struct dump_config_t *cfg,
                               json_dump_callback_t callback, void *data);

static size_t indentation_width(const struct dump_config_t *cfg, size_t depth) {
    if (cfg->indentation == -1)
        return depth > SIZE_MAX / 8 ? SIZE_MAX : depth * 8;
    if (cfg->indentation > 0 && depth > SIZE_MAX / (size_t)cfg->indentation)
        return SIZE_MAX;
    return depth * (size_t)cfg->indentation;
}

static int container_fits_inline(struct walk_context *ctx, json_t *value,
                                 struct dump_config_t *cfg, int *fits) {
    size_t depth = json_array_size(ctx->path_array);
    struct width_measure measure = {0};
    struct dump_config_t measure_cfg = *cfg;
    size_t prefix = indentation_width(cfg, depth);
    int omit_outer_brackets = depth == 0 && cfg->embed;
    int result;

    *fits = 0;
    if (prefix == SIZE_MAX || prefix >= SEMI_PRETTY_LINE_WIDTH)
        return 0;
    measure.columns = prefix;
    measure.limit = SEMI_PRETTY_LINE_WIDTH - 1;
    if (depth > 0) {
        json_t *path_item = json_array_get(ctx->path_array, depth - 1);
        if (json_is_string(path_item)) {
            if (json_dump_callback(path_item, measure_output_callback, &measure,
                                   dump_flags(cfg)) != 0) {
                if (measure.exceeded)
                    return 0;
                return -1;
            }
            if (add_width(&measure, 2) < 0)
                return 0;
        }
    }

    measure_cfg.embed = omit_outer_brackets;
    measure_cfg.force_inline = 1;
    measure_cfg.color = 0;
    measure_cfg.inline_stack = NULL;
    measure_cfg.inline_stack_capacity = 0;
    result = print_json_callback(value, &measure_cfg, measure_output_callback, &measure);
    if (result < 0 && !measure.exceeded)
        return -1;
    *fits = !measure.exceeded;
    return 0;
}

static int reserve_layout(struct dump_config_t *cfg, size_t depth) {
    unsigned char *new_stack;
    size_t capacity;

    if (depth < cfg->inline_stack_capacity)
        return 0;
    capacity = cfg->inline_stack_capacity ? cfg->inline_stack_capacity : 16;
    while (capacity <= depth) {
        if (capacity > SIZE_MAX / 2)
            return -1;
        capacity *= 2;
    }
    if (capacity > SIZE_MAX / sizeof(*new_stack))
        return -1;
    new_stack = realloc(cfg->inline_stack, capacity * sizeof(*new_stack));
    if (!new_stack)
        return -1;
    cfg->inline_stack = new_stack;
    cfg->inline_stack_capacity = capacity;
    return 0;
}

static int dump_json_value(struct dump_config_t *cfg, const json_t *value) {
    return json_dump_callback(value, cfg->output, cfg->output_data, dump_flags(cfg));
}

static int emit_colored_value(struct dump_config_t *cfg, const json_t *value,
                              const char *color) {
    int result;

    if (cfg->color && emit_text(cfg, color) < 0)
        return -1;
    result = dump_json_value(cfg, value);
    if (cfg->color && emit_text(cfg, "\033[0m") < 0)
        return -1;
    return result;
}

static int emit_container_open(struct walk_context *ctx, char c) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);

    if (!cfg->embed || depth > 0)
        return emit_char(cfg, c);

    return 0;
}

static int should_inline_container(const struct dump_config_t *cfg, size_t depth) {
    return cfg->force_inline || cfg->format == OUTPUT_COMPACT ||
           (cfg->format == OUTPUT_SEMI_PRETTY && cfg->inline_stack[depth]);
}

static int uses_inline_layout(const struct dump_config_t *cfg, size_t depth) {
    return cfg->force_inline || cfg->format == OUTPUT_COMPACT ||
           (cfg->format == OUTPUT_SEMI_PRETTY && depth > 0 &&
            cfg->inline_stack[depth - 1]);
}

static int emit_container_close(struct walk_context *ctx, json_t *obj) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);
    int inline_container = should_inline_container(cfg, depth);

    if (!cfg->embed || depth > 0) {
        if (!inline_container &&
            (json_object_size(obj) > 0 || json_array_size(obj) > 0)) {
            if (emit_char(cfg, '\n') < 0 || emit_indentation(cfg, depth) < 0)
                return -1;
        }

        if (emit_char(cfg, json_is_object(obj) ? '}' : ']') < 0)
            return -1;
    }
    return 0;
}

static int emit_member_separator(struct walk_context *ctx, const size_t status) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);
    int last_child = (status & JSON_WALK_STATUS_LAST_CHILD) != 0;
    int parent_inline = uses_inline_layout(cfg, depth);

    if (depth > 0 && !last_child) {
        if (emit_char(cfg, ',') < 0)
            return -1;
        if (cfg->format == OUTPUT_SEMI_PRETTY && parent_inline && emit_char(cfg, ' ') < 0)
            return -1;
    }
    return 0;
}

static int emit_member_prefix(struct walk_context *ctx, const size_t status) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);
    int parent_inline = uses_inline_layout(cfg, depth);

    int first_child = (status & JSON_WALK_STATUS_FIRST_CHILD) != 0;

    if (cfg->format == OUTPUT_PRETTY ||
        (cfg->format == OUTPUT_SEMI_PRETTY && !parent_inline)) {
        if (!cfg->embed || depth != 1 || !first_child) {
            if (emit_char(cfg, '\n') < 0)
                return -1;
        }
        if (emit_indentation(cfg, depth) < 0)
            return -1;
    }

    if (depth > 0) {
        json_t *path_item = json_array_get(ctx->path_array, depth - 1);
        if (json_is_string(path_item)) {
            if (emit_colored_value(cfg, path_item, "\033[36m") < 0)
                return -1;
            if (emit_text(cfg, cfg->format == OUTPUT_COMPACT ? ":" : ": ") < 0)
                return -1;
        }
    }

    return 0;
}

static json_walk_result_t dump_value(struct walk_context *ctx, json_t *value,
                                     const size_t status) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);
    const char *color;

    if (depth > 0 && emit_member_prefix(ctx, status) < 0)
        return output_error(cfg);

    if (json_is_string(value)) {
        color = "\033[32m";
    } else if (json_is_integer(value) || json_is_real(value)) {
        color = "\033[33m";
    } else {
        color = "\033[35m";
    }

    if (emit_colored_value(cfg, value, color) < 0 ||
        emit_member_separator(ctx, status) < 0)
        return output_error(cfg);
    return JSON_WALK_CONTINUE;
}

static json_walk_result_t dump_container(struct walk_context *ctx, json_t *value,
                                         const size_t status) {
    struct dump_config_t *cfg = ctx->data;
    size_t depth = json_array_size(ctx->path_array);
    int parent_inline;
    int inline_container;

    if (status & JSON_WALK_STATUS_CONTAINER_POST) {
        if (emit_container_close(ctx, value) < 0 ||
            emit_member_separator(ctx, status) < 0)
            return output_error(cfg);
        return JSON_WALK_CONTINUE;
    }

    parent_inline = uses_inline_layout(cfg, depth);
    inline_container = parent_inline;
    if (!cfg->force_inline && cfg->format == OUTPUT_SEMI_PRETTY) {
        if (reserve_layout(cfg, depth) < 0)
            return output_error(cfg);
        if (!parent_inline &&
            container_fits_inline(ctx, value, cfg, &inline_container) < 0)
            return output_error(cfg);
        cfg->inline_stack[depth] = inline_container;
    }

    if (depth > 0 && emit_member_prefix(ctx, status) < 0)
        return output_error(cfg);

    if (emit_container_open(ctx, json_is_object(value) ? '{' : '[') < 0)
        return output_error(cfg);
    return JSON_WALK_CONTINUE;
}

static json_walk_result_t dump(json_t *obj, struct walk_context *ctx) {
    if (ctx->status & (JSON_WALK_STATUS_CONTAINER_PRE | JSON_WALK_STATUS_CONTAINER_POST))
        return dump_container(ctx, obj, ctx->status) == -1 ? JSON_WALK_ERROR
                                                           : JSON_WALK_CONTINUE;
    if (ctx->status & JSON_WALK_STATUS_LEAF)
        return dump_value(ctx, obj, ctx->status);
    return JSON_WALK_CONTINUE;
}

static int print_json_callback(json_t *root, struct dump_config_t *cfg,
                               json_dump_callback_t callback, void *data) {
    struct walk_context ctx;
    int result;

    ctx.flags = JSON_WALK_CONTAINER_PRE | JSON_WALK_CONTAINER_POST | JSON_WALK_LEAFS;
    ctx.callback = dump;
    ctx.data = cfg;
    cfg->output = callback;
    cfg->output_data = data;
    result = json_walk_ctx(&ctx, root);

    if (result != 0 || cfg->failed || emit_char(cfg, '\n') < 0)
        return -1;
    return 0;
}

static int write_file_callback(const char *data, size_t size, void *context) {
    FILE *file = context;
    return fwrite(data, 1, size, file) == size ? 0 : -1;
}

static int print_json(json_t *root, struct dump_config_t *cfg) {
    if (print_json_callback(root, cfg, write_file_callback, stdout) < 0 ||
        fflush(stdout) == EOF)
        return -1;
    return 0;
}

static void print_usage(const char *progname) {
    fprintf(stderr,
            "Usage: %s [--compact|--pretty|--semi-pretty] [--indent N|--tabs] [--embed] "
            "[--ensure-ascii] [--escape-slash] [--sort-keys] "
            "[--real-precision N] [--color|--no-color]\n"
            "       %s --help\n",
            progname, progname);
}

static int parse_integer(const char *text, int minimum, int maximum, int *value) {
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < minimum || parsed > maximum)
        return -1;
    *value = (int)parsed;
    return 0;
}

static int format_conflicts_with_indentation(int selected_format, int indentation) {
    return (selected_format == OUTPUT_COMPACT) != (indentation == 0);
}

static int parse_options(int argc, char **argv, struct dump_config_t *cfg, int *sort_keys,
                         int *color_override) {
    int selected_format = -1;
    int indentation = cfg->indentation;
    int option;
    int format_set = 0;
    int indentation_set = 0;
    opterr = 0;
    optind = 1;

    while ((option = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (option) {
            case 'h':
                print_usage(argv[0]);
                return 1;
            case 'c':
            case 'p':
            case 'P': {
                int requested = option == 'c'   ? OUTPUT_COMPACT
                                : option == 'P' ? OUTPUT_SEMI_PRETTY
                                                : OUTPUT_PRETTY;
                if ((format_set && selected_format != requested) ||
                    (indentation_set &&
                     format_conflicts_with_indentation(requested, indentation)))
                    goto invalid_option;
                selected_format = requested;
                format_set = 1;
            } break;
            case 'i':
                if (parse_integer(optarg, 0, 31, &indentation) < 0)
                    goto invalid_option;
                if (format_set &&
                    format_conflicts_with_indentation(selected_format, indentation))
                    goto invalid_option;
                indentation_set = 1;
                break;
            case 't':
                if (format_set && selected_format == OUTPUT_COMPACT)
                    goto invalid_option;
                indentation = -1;
                indentation_set = 1;
                break;
            case 'e':
                cfg->embed = 1;
                break;
            case 'a':
                cfg->ensure_ascii = 1;
                break;
            case 'E':
                cfg->escape_slash = 1;
                break;
            case 's':
                *sort_keys = 1;
                break;
            case 'r':
                if (parse_integer(optarg, 0, 31, &cfg->real_precision) < 0)
                    goto invalid_option;
                break;
            case 'C':
                *color_override = 1;
                break;
            case 'M':
                *color_override = 0;
                break;
            default:
                goto invalid_option;
        }
    }

    if (optind != argc)
        goto invalid_option;

    if (format_set) {
        cfg->format = (enum output_format)selected_format;
        if (selected_format == OUTPUT_COMPACT)
            cfg->indentation = 0;
        else if (indentation_set)
            cfg->indentation = indentation;
    } else if (indentation_set) {
        cfg->indentation = indentation;
        cfg->format = indentation == 0 ? OUTPUT_COMPACT : OUTPUT_PRETTY;
    }
    return 0;

invalid_option:
    fprintf(stderr, "error: invalid or conflicting option\n");
    print_usage(argv[0]);
    return -1;
}

struct object_entry {
    const char *key;
    size_t key_length;
    json_t *value;
};

static int compare_object_entries(const void *left, const void *right) {
    const struct object_entry *a = left;
    const struct object_entry *b = right;
    size_t common_length = a->key_length < b->key_length ? a->key_length : b->key_length;
    int result = memcmp(a->key, b->key, common_length);

    if (result != 0)
        return result;
    if (a->key_length < b->key_length)
        return -1;
    if (a->key_length > b->key_length)
        return 1;
    return 0;
}

static json_t *sorted_copy(json_t *value) {
    if (json_is_object(value)) {
        size_t count = json_object_size(value);
        size_t index = 0;
        size_t key_length;
        const char *key;
        json_t *child_value;
        struct object_entry *entries;
        json_t *copy;

        if (count > SIZE_MAX / sizeof(*entries))
            return NULL;
        entries = count ? malloc(count * sizeof(*entries)) : NULL;
        copy = json_object();
        if ((count && !entries) || !copy) {
            free(entries);
            json_decref(copy);
            return NULL;
        }
        json_object_keylen_foreach(value, key, key_length, child_value) {
            entries[index].key = key;
            entries[index].key_length = key_length;
            entries[index].value = child_value;
            index++;
        }
        if (count > 1)
            qsort(entries, count, sizeof(*entries), compare_object_entries);
        for (index = 0; index < count; index++) {
            json_t *child = sorted_copy(entries[index].value);
            if (!child ||
                json_object_setn_nocheck(copy, entries[index].key,
                                         entries[index].key_length, child) != 0) {
                json_decref(child);
                json_decref(copy);
                free(entries);
                return NULL;
            }
            json_decref(child);
        }
        free(entries);
        return copy;
    }

    if (json_is_array(value)) {
        size_t index;
        json_t *item;
        json_t *copy = json_array();
        if (!copy)
            return NULL;
        json_array_foreach(value, index, item) {
            json_t *child = sorted_copy(item);
            if (!child || json_array_append(copy, child) != 0) {
                json_decref(child);
                json_decref(copy);
                return NULL;
            }
            json_decref(child);
        }
        return copy;
    }

    return json_incref(value);
}

int main(int argc, char *argv[]) {
    json_t *root = NULL;
    json_t *sorted_root = NULL;
    json_t *to_dump;
    json_error_t error;
    struct dump_config_t cfg;
    int sort_keys = 0;
    int color_override = -1;
    int parse_result;
    int exit_code = 1;

    memset(&cfg, 0, sizeof(cfg));
    cfg.indentation = 2;
    cfg.format = OUTPUT_PRETTY;
    cfg.real_precision = -1;

    setlocale(LC_CTYPE, "");
    parse_result = parse_options(argc, argv, &cfg, &sort_keys, &color_override);

    if (parse_result != 0)
        return parse_result == 1 ? 0 : 1;

    if (color_override >= 0) {
        cfg.color = color_override != 0;
    } else {
        const char *no_color = getenv("NO_COLOR");
        cfg.color = isatty(STDOUT_FILENO) && (!no_color || *no_color == '\0');
    }

    if (isatty(STDIN_FILENO)) {
        fprintf(stderr,
                "error: no JSON input provided via stdin. Pipe input to this command.\n");
        return 1;
    }

    root = json_loadf(stdin, JSON_DECODE_ANY, &error);
    if (!root) {
        fprintf(stderr, "json error on line %d column %d: %s\n", error.line, error.column,
                error.text);
        goto cleanup;
    }

    to_dump = root;
    if (sort_keys) {
        sorted_root = sorted_copy(root);
        if (!sorted_root) {
            fprintf(stderr, "error: failed to sort JSON input\n");
            goto cleanup;
        }
        to_dump = sorted_root;
    }
    if (print_json(to_dump, &cfg) < 0) {
        fprintf(stderr, "error: failed to write JSON output\n");
        goto cleanup;
    }
    exit_code = 0;

cleanup:
    free(cfg.inline_stack);
    json_decref(sorted_root);
    json_decref(root);
    return exit_code;
}
