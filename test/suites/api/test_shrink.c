/*
 * Copyright (c) 2025 Wilhelm Wiens <wilhelmwiens@gmail.com>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "util.h"
#include <jansson.h>
#include "../../../src/jansson_private.h"
#include <string.h>


static void test_shrink_array(void) {
    json_auto_t *json = json_array();
    json_array_t *array_struct = json_to_array(json);

    if (!json)
        fail("failed to allocate array");

    /* Add some elements */
    for (size_t i = 0; i < 10; i++) {
        json_t *value = json_integer(i);
        if (json_array_append_new(json, value))
            fail("json_array_append failed");
    }
    if(json_array_size(json) != 10)
        fail("json_array_size after append is incorrect");
    if (!array_struct)
        fail("json_to_array failed");
    if (array_struct->size != 16)
        fail("json_to_array size is incorrect after append");
    if (array_struct->entries != 10)
        fail("json_to_array entries is incorrect after append");

    /* Shrink the array */
    if (json_array_shrink(json))
        fail("json_array_shrink failed");
    if (json_array_size(json) != 10)
        fail("json_array_size after shrink is incorrect");
    if (!array_struct)
        fail("json_to_array failed after shrink");
    if (array_struct->size != 10)
        fail("json_to_array size is incorrect after shrink");
    if (array_struct->entries != 10)
        fail("json_to_array entries is incorrect after shrink");
    /* Check that the elements are still there */
    for (size_t i = 0; i < 10; i++) {
        const json_t *value = json_array_get(json, i);
        if (!value || !json_is_integer(value) || json_integer_value(value) != (json_int_t)i)
            fail("json_array_get failed or returned incorrect value");
    }
    /* Check that the array can still be appended to */
    if (json_array_append_new(json, json_integer(10)))
        fail("json_array_append after shrink failed");
    if (json_array_size(json) != 11)
        fail("json_array_size after append is incorrect");
    if (!array_struct)
        fail("json_to_array failed after append");
    if (array_struct->size != 20) // We always double the size when growing
        fail("json_to_array size is incorrect after append");
    if (array_struct->entries != 11)
        fail("json_to_array entries is incorrect after append");
    /* Check that the elements are still there */
    for (size_t i = 0; i < 11; i++) {
        const json_t *value = json_array_get(json, i);
        if (!value || !json_is_integer(value) || json_integer_value(value) != (json_int_t)i)
            fail("json_array_get failed or returned incorrect value after append");
    }
    /* Check that the array can still be shrunk */
    if (json_array_shrink(json))
        fail("json_array_shrink after append failed");
    if (json_array_size(json) != 11)
        fail("json_array_size after second shrink is incorrect");
    if (!array_struct)
        fail("json_to_array failed after second shrink");
    if (array_struct->size != 11)
        fail("json_to_array size is incorrect after second shrink");
    if (array_struct->entries != 11)
        fail("json_to_array entries is incorrect after second shrink");
    /* Check that the elements are still there */
    for (size_t i = 0; i < 11; i++) {
        const json_t *value = json_array_get(json, i);
        if (!value || !json_is_integer(value) || json_integer_value(value) != (json_int_t)i)
            fail("json_array_get failed or returned incorrect value after second shrink");
    }
}

static void test_shrink_object(void) {
    json_auto_t *json = json_object();
    json_object_t *object_struct = json_to_object(json);
    if (!json)
        fail("failed to allocate object");
    /* Add some elements */
    for (size_t i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%zu", i);
        if (json_object_set_new(json, key, json_integer(i)))
            fail("json_object_set_new failed");
    }
    if (json_object_size(json) != 10)
        fail("json_object_size after set is incorrect");
    if (!object_struct)
        fail("json_to_object failed");
    if (object_struct->hashtable.size != 10)
        fail("json_to_object size is incorrect after set");
    if (object_struct->hashtable.order != 4)
        fail("json_to_object entries is incorrect after set");
    
    /* Shrink the object */
    for(int i = 9; i >= 0; i--) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        if (json_object_del(json, key))
            fail("json_object_del failed");
        json_shrink(json);
        if( json_object_size(json) != (size_t)i)
            fail("json_object_size after del is incorrect");
        if (!object_struct)
            fail("json_to_object failed after del");
        if (object_struct->hashtable.order != (size_t)((i <= 1) ? 0 : ((i <= 2) ? 1 : ((i <= 4) ? 2 : ((i <= 8) ? 3 : 4)))))
            fail("json_to_object order is incorrect after del");
        for(int j = 0; j < i; j++) {
            char key_value[16];
            const json_t* value;
            snprintf(key_value, sizeof(key_value), "key%d", j);
            value = json_object_get(json, key_value);
            if (!value || !json_is_integer(value) || json_integer_value(value) != (json_int_t)j)
                fail("json_object_get failed or returned incorrect value after del and shrink");
        }
    }
}
static void test_shrink_recursive(void)
{
    json_auto_t *json = json_loads(
        "{\"1\": \
                { \"1\":11, \
                  \"2\": {\"1\": 121, \"2\": 122}, \
                  \"3\" : [ 131 ,132, [1331]] \
                } \
        }",
        JSON_DECODE_ANY, NULL);
    json_object_t *object_struct;
    json_array_t *array_struct;
    json_t *value_1, *value_11, *value_12, *value_13, *value_133;

    if (!json)
        fail("failed to allocate object");

    /* Shrink the object */
    if (json_shrink_recursive(json))
        fail("json_shrink failed");
    
    object_struct = json_to_object(json);
    if (!object_struct)
        fail("json_to_object failed");
    if(object_struct->hashtable.order != 0)
        fail("json_to_object size is incorrect after shrink");
    
    value_1 = json_object_get(json, "1");
    if (!value_1 || !json_is_object(value_1))
        fail("json_object_get failed or returned incorrect value after shrink");
    object_struct = json_to_object(value_1);
    if(object_struct->hashtable.order != 2)
        fail("json_to_object order is incorrect after shrink");
    // 1.1
    value_11 = json_object_get(value_1, "1");
    if (!value_11 || !json_is_integer(value_11) || json_integer_value(value_11) != 11)
        fail("json_object_get for '1' failed or returned incorrect value after shrink");
    // 1.2
    value_12 = json_object_get(value_1, "2");
    if (!value_12 || !json_is_object(value_12))
        fail("json_object_get for '2' failed or returned incorrect value after shrink");
    object_struct = json_to_object(value_12);
    if(object_struct->hashtable.order != 1)
        fail("json_to_object order is incorrect after shrink");
    // 1.3
    value_13 = json_object_get(value_1, "3");
    if (!value_13 || !json_is_array(value_13))
        fail("json_object_get for '3' failed or returned incorrect value after shrink");
    array_struct = json_to_array(value_13);
    if (!array_struct)
        fail("json_to_array failed after shrink");
    if(array_struct->size != 3)
        fail("json_to_array size is incorrect after shrink");
    if(array_struct->entries != 3)
        fail("json_to_array entries is incorrect after shrink");
    // 1.3.3
    value_133 = json_array_get(value_13, 2);
    if (!value_133 || !json_is_array(value_133))
        fail("json_array_get for [2] failed or returned incorrect value after shrink");
    array_struct = json_to_array(value_133);
    if (!array_struct)
        fail("json_to_array failed after shrink");
    if(array_struct->size != 1)
        fail("json_to_array size is incorrect after shrink");
    if(array_struct->entries != 1)
        fail("json_to_array entries is incorrect after shrink");
}

static void run_tests(void) {
    test_shrink_array();
    test_shrink_object();
    test_shrink_recursive();
}
