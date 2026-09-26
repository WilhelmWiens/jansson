.. _apiref:

*************
API Reference
*************

.. highlight:: c

Preliminaries
=============

All declarations are in :file:`jansson.h`, so it's enough to

::

   #include <jansson.h>

in each source file.

All constants are prefixed with ``JSON_`` (except for those describing
the library version, prefixed with ``JANSSON_``). Other identifiers
are prefixed with ``json_``. Type names are suffixed with ``_t`` and
``typedef``\ 'd so that the ``struct`` keyword need not be used.


Library Version
===============

The Jansson version is of the form *A.B.C*, where *A* is the major
version, *B* is the minor version and *C* is the micro version. If the
micro version is zero, it's omitted from the version string, i.e. the
version string is just *A.B*.

When a new release only fixes bugs and doesn't add new features or
functionality, the micro version is incremented. When new features are
added in a backwards compatible way, the minor version is incremented
and the micro version is set to zero. When there are backwards
incompatible changes, the major version is incremented and others are
set to zero.

The following preprocessor constants specify the current version of
the library:

``JANSSON_MAJOR_VERSION``, ``JANSSON_MINOR_VERSION``, ``JANSSON_MICRO_VERSION``
  Integers specifying the major, minor and micro versions,
  respectively.

``JANSSON_VERSION``
  A string representation of the current version, e.g. ``"1.2.1"`` or
  ``"1.3"``.

``JANSSON_VERSION_HEX``
  A 3-byte hexadecimal representation of the version, e.g.
  ``0x010201`` for version 1.2.1 and ``0x010300`` for version 1.3.
  This is useful in numeric comparisons, e.g.::

      #if JANSSON_VERSION_HEX >= 0x010300
      /* Code specific to version 1.3 and above */
      #endif

Additionally, there are functions to determine the version of Jansson at
runtime:

.. function:: const char *jansson_version_str()

   Return the version of the Jansson library, in the same format as
   the ``JANSSON_VERSION`` preprocessor constant.

   .. versionadded:: 2.13

.. function:: int jansson_version_cmp(int major, int minor, int micro)

   Returns an integer less than, equal to, or greater than zero if
   the runtime version of Jansson is found, respectively, to be less
   than, to match, or be greater than the provided *major*, *minor*, and
   *micro*.

   .. versionadded:: 2.13

``JANSSON_THREAD_SAFE_REFCOUNT``
  If this value is defined all read-only operations and reference counting in
  Jansson are thread safe.  This value is not defined for versions older than
  ``2.11`` or when the compiler does not provide built-in atomic functions.


Value Representation
====================

The JSON specification (:rfc:`4627`) defines the following data types:
*object*, *array*, *string*, *number*, *boolean*, and *null*. JSON
types are used dynamically; arrays and objects can hold any other data
type, including themselves. For this reason, Jansson's type system is
also dynamic in nature. There's one C type to represent all JSON
values, and this structure knows the type of the JSON value it holds.

.. type:: json_t

  This data structure is used throughout the library to represent all
  JSON values. It always contains the type of the JSON value it holds
  and the value's reference count. The rest depends on the type of the
  value.

Objects of :type:`json_t` are always used through a pointer. There
are APIs for querying the type, manipulating the reference count, and
for constructing and manipulating values of different types.

Unless noted otherwise, all API functions return an error value if an
error occurs. Depending on the function's signature, the error value
is either *NULL* or -1. Invalid arguments or invalid input are
apparent sources for errors. Memory allocation and I/O operations may
also cause errors.


Type
----

.. c:enum:: json_type

   The type of a JSON value. The following members are defined:

   +---------------------+
   | ``JSON_OBJECT``     |
   +---------------------+
   | ``JSON_ARRAY``      |
   +---------------------+
   | ``JSON_STRING``     |
   +---------------------+
   | ``JSON_INTEGER``    |
   +---------------------+
   | ``JSON_BIGINTEGER`` |
   +---------------------+
   | ``JSON_REAL``       |
   +---------------------+
   | ``JSON_BIGREAL``    |
   +---------------------+
   | ``JSON_TRUE``       |
   +---------------------+
   | ``JSON_FALSE``      |
   +---------------------+
   | ``JSON_NULL``       |
   +---------------------+

   These correspond to JSON object, array, string, number, boolean and
   null. A number is represented by one of the values of type
   ``JSON_INTEGER``, ``JSON_BIGINTEGER``, ``JSON_REAL``, or ``JSON_BIGREAL``.
   A true boolean value is represented by a value of the type ``JSON_TRUE``
   and false by a value of the type ``JSON_FALSE``.

   The two big-number types, ``JSON_BIGINTEGER`` and ``JSON_BIGREAL``,
   are used with add-on extensions to allow arbitrarily large or
   high-precision numbers to be represented. Unless an extension is
   provided and enabled using the big number extension API, these
   types will not be used.

.. function:: int json_typeof(const json_t *json)

   Return the type of the JSON value (a :type:`json_type` cast to
   ``int``). *json* MUST NOT be *NULL*. This function is actually
   implemented as a macro for speed.

.. function:: int json_is_object(const json_t *json)
               int json_is_array(const json_t *json)
               int json_is_string(const json_t *json)
               int json_is_integer(const json_t *json)
               int json_is_biginteger(const json_t *json)
               int json_is_real(const json_t *json)
               int json_is_bigreal(const json_t *json)
               int json_is_true(const json_t *json)
               int json_is_false(const json_t *json)
               int json_is_null(const json_t *json)

   These functions (actually macros) return true (non-zero) for values
   of the given type, and false (zero) for values of other types and
   for *NULL*.

.. function:: int json_is_number(const json_t *json)

   Returns true for values of types ``JSON_INTEGER`` and
   ``JSON_REAL``; and false for other types and for *NULL*.

.. function:: int json_is_bignumber(const json_t *json)

   Returns true for values of types ``JSON_BIGINTEGER`` and
   ``JSON_BIGREAL``, and false for other types and for *NULL*.

.. function:: int json_is_anynumber(const json_t *json)

   Returns true for values of types ``JSON_INTEGER``,
   ``JSON_BIGINTEGER``, ``JSON_REAL`` and ``JSON_BIGREAL``, and false
   for other types and for *NULL*.

.. function:: int json_is_anyinteger(const json_t *json)

   Returns true for values of types ``JSON_INTEGER`` and
   ``JSON_BIGINTEGER``, and false for other types and for *NULL*.

.. function:: int json_is_anyreal(const json_t *json)

   Returns true for values of types ``JSON_REAL`` and
   ``JSON_BIGREAL``, and false for other types and for *NULL*.

.. function:: int json_is_boolean(const json_t *json)

   Returns true for types ``JSON_TRUE`` and ``JSON_FALSE``, and false
   for values of other types and for *NULL*.

.. function:: int json_boolean_value(const json_t *json)

   Alias of :func:`json_is_true()`, i.e. returns 1 for ``JSON_TRUE``
   and 0 otherwise.

   .. versionadded:: 2.7


.. _apiref-reference-count:

Reference Count
---------------

The reference count is used to track whether a value is still in use
or not. When a value is created, it's reference count is set to 1. If
a reference to a value is kept (e.g. a value is stored somewhere for
later use), its reference count is incremented, and when the value is
no longer needed, the reference count is decremented. When the
reference count drops to zero, there are no references left, and the
value can be destroyed.

.. function:: json_t *json_incref(json_t *json)

   Increment the reference count of *json* if it's not *NULL*.
   Returns *json*.

.. function:: void json_decref(json_t *json)

   Decrement the reference count of *json*. As soon as a call to
   :func:`json_decref()` drops the reference count to zero, the value
   is destroyed and it can no longer be used.

Functions creating new JSON values set the reference count to 1. These
functions are said to return a **new reference**. Other functions
returning (existing) JSON values do not normally increase the
reference count. These functions are said to return a **borrowed
reference**. So, if the user will hold a reference to a value returned
as a borrowed reference, he must call :func:`json_incref`. As soon as
the value is no longer needed, :func:`json_decref` should be called
to release the reference.

Normally, all functions accepting a JSON value as an argument will
manage the reference, i.e. increase and decrease the reference count
as needed. However, some functions **steal** the reference, i.e. they
have the same result as if the user called :func:`json_decref()` on
the argument right after calling the function. These functions are
suffixed with ``_new`` or have ``_new_`` somewhere in their name.

For example, the following code creates a new JSON array and appends
an integer to it::

  json_t *array, *integer;

  array = json_array();
  integer = json_integer(42);

  json_array_append(array, integer);
  json_decref(integer);

Note how the caller has to release the reference to the integer value
by calling :func:`json_decref()`. By using a reference stealing
function :func:`json_array_append_new()` instead of
:func:`json_array_append()`, the code becomes much simpler::

  json_t *array = json_array();
  json_array_append_new(array, json_integer(42));

In this case, the user doesn't have to explicitly release the
reference to the integer value, as :func:`json_array_append_new()`
steals the reference when appending the value to the array.

In the following sections it is clearly documented whether a function
will return a new or borrowed reference or steal a reference to its
argument.


Circular References
-------------------

A circular reference is created when an object or an array is,
directly or indirectly, inserted inside itself. The direct case is
simple::

  json_t *obj = json_object();
  json_object_set(obj, "foo", obj);

Jansson will refuse to do this, and :func:`json_object_set()` (and
all the other such functions for objects and arrays) will return with
an error status. The indirect case is the dangerous one::

  json_t *arr1 = json_array(), *arr2 = json_array();
  json_array_append(arr1, arr2);
  json_array_append(arr2, arr1);

In this example, the array ``arr2`` is contained in the array
``arr1``, and vice versa. Jansson cannot check for this kind of
indirect circular references without a performance hit, so it's up to
the user to avoid them.

If a circular reference is created, the memory consumed by the values
cannot be freed by :func:`json_decref()`. The reference counts never
drops to zero because the values are keeping the references to each
other. Moreover, trying to encode the values with any of the encoding
functions will fail. The encoder detects circular references and
returns an error status.

Scope Dereferencing
-------------------

.. versionadded:: 2.9

It is possible to use the ``json_auto_t`` type to automatically
dereference a value at the end of a scope. For example::

  void function(void) {
    json_auto_t *value = NULL;
    value = json_string("foo");
    /* json_decref(value) is automatically called. */
  }

This feature is only available on GCC and Clang. So if your project
has a portability requirement for other compilers, you should avoid
this feature.

Additionally, as always, care should be taken when passing values to
functions that steal references.

True, False and Null
====================

These three values are implemented as singletons, so the returned
pointers won't change between invocations of these functions.

.. function:: json_t *json_true(void)

   .. refcounting:: new

   Returns the JSON true value.

.. function:: json_t *json_false(void)

   .. refcounting:: new

   Returns the JSON false value.

.. function:: json_t *json_boolean(val)

   .. refcounting:: new

   Returns JSON false if ``val`` is zero, and JSON true otherwise.
   This is a macro, and equivalent to ``val ? json_true() :
   json_false()``.

   .. versionadded:: 2.4


.. function:: json_t *json_null(void)

   .. refcounting:: new

   Returns the JSON null value.


String
======

Jansson uses UTF-8 as the character encoding. All JSON strings must be
valid UTF-8 (or ASCII, as it's a subset of UTF-8). All Unicode
codepoints U+0000 through U+10FFFF are allowed, but you must use
length-aware functions if you wish to embed null bytes in strings.

.. function:: json_t *json_string(const char *value)

   .. refcounting:: new

   Returns a new JSON string, or *NULL* on error. *value* must be a
   valid null terminated UTF-8 encoded Unicode string.

.. function:: json_t *json_stringn(const char *value, size_t len)

   .. refcounting:: new

   Like :func:`json_string`, but with explicit length, so *value* may
   contain null characters or not be null terminated.

   .. versionadded:: 2.7

.. function:: json_t *json_string_nocheck(const char *value)

   .. refcounting:: new

   Like :func:`json_string`, but doesn't check that *value* is valid
   UTF-8. Use this function only if you are certain that this really
   is the case (e.g. you have already checked it by other means).

.. function:: json_t *json_stringn_nocheck(const char *value, size_t len)

   .. refcounting:: new

   Like :func:`json_string_nocheck`, but with explicit length, so
   *value* may contain null characters or not be null terminated.

   .. versionadded:: 2.7

.. function:: const char *json_string_value(const json_t *string)

   Returns the associated value of *string* as a null terminated UTF-8
   encoded string, or *NULL* if *string* is not a JSON string.

   The returned value is read-only and must not be modified or freed by
   the user. It is valid as long as *string* exists, i.e. as long as
   its reference count has not dropped to zero.

.. function:: size_t json_string_length(const json_t *string)

   Returns the length of *string* in its UTF-8 presentation, or zero
   if *string* is not a JSON string.

   .. versionadded:: 2.7

.. function:: int json_string_set(json_t *string, const char *value)

   Sets the associated value of *string* to *value*. *value* must be a
   valid UTF-8 encoded Unicode string. Returns 0 on success and -1 on
   error.

.. function:: int json_string_setn(json_t *string, const char *value, size_t len)

   Like :func:`json_string_set`, but with explicit length, so *value*
   may contain null characters or not be null terminated.

   .. versionadded:: 2.7

.. function:: int json_string_set_nocheck(json_t *string, const char *value)

   Like :func:`json_string_set`, but doesn't check that *value* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_string_setn_nocheck(json_t *string, const char *value, size_t len)

   Like :func:`json_string_set_nocheck`, but with explicit length,
   so *value* may contain null characters or not be null terminated.

   .. versionadded:: 2.7

.. function:: json_t *json_sprintf(const char *format, ...)
              json_t *json_vsprintf(const char *format, va_list ap)

   .. refcounting:: new

   Construct a JSON string from a format string and varargs, just like
   :func:`printf()`.

   .. versionadded:: 2.11


Number
======

The JSON specification only contains one numeric type, "number". The C
programming language has distinct types for integer and floating-point
numbers, so for practical reasons Jansson also has distinct types for
the two. They are called "integer" and "real", respectively.

Additionally, Jansson provides an extension API to allow external
packages to be used to represent arbitrarily large integers or
arbitrarily high-precision real numbers.  So a JSON number may be
represented by any of four different C types.

For more information, see :ref:`rfc-conformance`.

.. type:: json_int_t

   This is the C type that is used to store JSON integer values in the
   absence of a big number extension. It represents the widest integer
   type available on your system. In practice it's just a typedef of
   ``long long`` if your compiler supports it, otherwise ``long``.

   Usually, you can safely use plain ``int`` in place of
   ``json_int_t``, and the implicit C integer conversion handles the
   rest. Only when you know that you need the full 64-bit range, you
   should use ``json_int_t`` explicitly.

.. type:: double

   The C type ``double`` is used to store JSON real values in the
   absence of a big number extension.

   Note that the C type ``long double`` is only supported by using the
   big number extension API.

.. type:: json_bigz_t
.. type:: json_bigz_const_t

   This is an unspecified C pointer type used to reference JSON
   integer values that are stored using an external big number
   package.  The ``_const_t`` type is the same only it is used to
   point to a constant value.

.. type:: json_bigr_t
.. type:: json_bigr_const_t

   This is an unspecified C pointer type used to reference JSON real
   values that are stored using an external big number package.  The
   ``_const_t`` type is the same only it is used to point to a
   constant value.

All of the functions and macros for dealing with the big number types
:type:`json_bigz_t` and :type:`json_bigr_t` are documented separately
in the section :ref:`apiref-big-number-extension`.

``JSON_INTEGER_IS_LONG_LONG``
   This is a preprocessor variable that holds the value 1 if
   :type:`json_int_t` is ``long long``, and 0 if it's ``long``. It
   can be used as follows::

       #if JSON_INTEGER_IS_LONG_LONG
       /* Code specific for long long */
       #else
       /* Code specific for long */
       #endif

``JSON_INTEGER_FORMAT``
   This is a macro that expands to a :func:`printf()` conversion
   specifier that corresponds to :type:`json_int_t`, without the
   leading ``%`` sign, i.e. either ``"lld"`` or ``"ld"``. This macro
   is required because the actual type of :type:`json_int_t` can be
   either ``long`` or ``long long``, and :func:`printf()` requires
   different length modifiers for the two.

   Example::

       json_int_t x = 123123123;
       printf("x is %" JSON_INTEGER_FORMAT "\n", x);


.. function:: json_t *json_integer(json_int_t value)

   .. refcounting:: new

   Returns a new JSON integer, or *NULL* on error.

.. function:: json_int_t json_integer_value(const json_t *integer)

   Returns the associated value of *integer*, or 0 if *json* is not a
   JSON integer.

.. function:: int json_integer_set(const json_t *integer, json_int_t value)

   Sets the associated value of *integer* to *value*. Returns 0 on
   success and -1 if *integer* is not a JSON integer.

.. function:: json_t *json_real(double value)

   .. refcounting:: new

   Returns a new JSON real, or *NULL* on error.

.. function:: double json_real_value(const json_t *real)

   Returns the associated value of *real*, or 0.0 if *real* is not a
   JSON real.

.. function:: int json_real_set(const json_t *real, double value)

   Sets the associated value of *real* to *value*. Returns 0 on
   success and -1 if *real* is not a JSON real.

.. function:: double json_number_value(const json_t *json)

   Returns the associated value of the JSON integer or JSON real
   *json*, cast to double regardless of the actual type. If *json* is
   neither JSON real nor JSON integer, 0.0 is returned.


Array
=====

A JSON array is an ordered collection of other JSON values.

.. function:: json_t *json_array(void)

   .. refcounting:: new

   Returns a new JSON array, or *NULL* on error. Initially, the array
   is empty.

.. function:: size_t json_array_size(const json_t *array)

   Returns the number of elements in *array*, or 0 if *array* is NULL
   or not a JSON array.

.. function:: json_t *json_array_get(const json_t *array, size_t index)

   .. refcounting:: borrow

   Returns the element in *array* at position *index*. The valid range
   for *index* is from 0 to the return value of
   :func:`json_array_size()` minus 1. If *array* is not a JSON array,
   if *array* is *NULL*, or if *index* is out of range, *NULL* is
   returned.

.. function:: int json_array_set(json_t *array, size_t index, json_t *value)

   Replaces the element in *array* at position *index* with *value*.
   The valid range for *index* is from 0 to the return value of
   :func:`json_array_size()` minus 1. Returns 0 on success and -1 on
   error.

.. function:: int json_array_set_new(json_t *array, size_t index, json_t *value)

   Like :func:`json_array_set()` but steals the reference to *value*.
   This is useful when *value* is newly created and not used after
   the call.

.. function:: int json_array_append(json_t *array, json_t *value)

   Appends *value* to the end of *array*, growing the size of *array*
   by 1. Returns 0 on success and -1 on error.

.. function:: int json_array_append_new(json_t *array, json_t *value)

   Like :func:`json_array_append()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_array_insert(json_t *array, size_t index, json_t *value)

   Inserts *value* to *array* at position *index*, shifting the
   elements at *index* and after it one position towards the end of
   the array. Returns 0 on success and -1 on error.

.. function:: int json_array_insert_new(json_t *array, size_t index, json_t *value)

   Like :func:`json_array_insert()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_array_remove(json_t *array, size_t index)

   Removes the element in *array* at position *index*, shifting the
   elements after *index* one position towards the start of the array.
   Returns 0 on success and -1 on error. The reference count of the
   removed value is decremented.

.. function:: int json_array_clear(json_t *array)

   Removes all elements from *array*. Returns 0 on success and -1 on
   error. The reference count of all removed values are decremented.

.. function:: int json_array_extend(json_t *array, json_t *other_array)

   Appends all elements in *other_array* to the end of *array*.
   Returns 0 on success and -1 on error.

.. function:: void json_array_foreach(array, index, value)

   Iterate over every element of ``array``, running the block
   of code that follows each time with the proper values set to
   variables ``index`` and ``value``, of types :type:`size_t` and
   :type:`json_t` pointer respectively. Example::

       /* array is a JSON array */
       size_t index;
       json_t *value;

       json_array_foreach(array, index, value) {
           /* block of code that uses index and value */
       }

   The items are returned in increasing index order.

   This macro expands to an ordinary ``for`` statement upon
   preprocessing, so its performance is equivalent to that of
   hand-written code using the array access functions.
   The main advantage of this macro is that it abstracts
   away the complexity, and makes for more concise and readable code.

   .. versionadded:: 2.5


Object
======

A JSON object is a dictionary of key-value pairs, where the key is a
Unicode string and the value is any JSON value.

Even though null bytes are allowed in string values, they are not
allowed in object keys.

.. function:: json_t *json_object(void)

   .. refcounting:: new

   Returns a new JSON object, or *NULL* on error. Initially, the
   object is empty.

.. function:: size_t json_object_size(const json_t *object)

   Returns the number of elements in *object*, or 0 if *object* is not
   a JSON object.

.. function:: json_t *json_object_get(const json_t *object, const char *key)

   .. refcounting:: borrow

   Get a value corresponding to *key* from *object*. Returns *NULL* if
   *key* is not found and on error.

.. function:: json_t *json_object_getn(const json_t *object, const char *key, size_t key_len)

   .. refcounting:: borrow

   Like :func:`json_object_get`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_set(json_t *object, const char *key, json_t *value)

   Set the value of *key* to *value* in *object*. *key* must be a
   valid null terminated UTF-8 encoded Unicode string. If there
   already is a value for *key*, it is replaced by the new value.
   Returns 0 on success and -1 on error.

.. function:: int json_object_setn(json_t *object, const char *key, size_t key_len, json_t *value)

   Like :func:`json_object_set`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_set_nocheck(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set`, but doesn't check that *key* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_object_setn_nocheck(json_t *object, const char *key, size_t key_len, json_t *value)

   Like :func:`json_object_set_nocheck`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_set_new(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set()` but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: int json_object_setn_new(json_t *object, const char *key, size_t key_len, json_t *value)

   Like :func:`json_object_set_new`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_set_new_nocheck(json_t *object, const char *key, json_t *value)

   Like :func:`json_object_set_new`, but doesn't check that *key* is
   valid UTF-8. Use this function only if you are certain that this
   really is the case (e.g. you have already checked it by other
   means).

.. function:: int json_object_setn_new_nocheck(json_t *object, const char *key, size_t key_len, json_t *value)

   Like :func:`json_object_set_new_nocheck`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_del(json_t *object, const char *key)

   Delete *key* from *object* if it exists. Returns 0 on success, or
   -1 if *key* was not found. The reference count of the removed value
   is decremented.

.. function:: int json_object_deln(json_t *object, const char *key, size_t key_len)

   Like :func:`json_object_del`, but give the fixed-length *key* with length *key_len*.
   See :ref:`fixed_length_keys` for details.

   .. versionadded:: 2.14

.. function:: int json_object_clear(json_t *object)

   Remove all elements from *object*. Returns 0 on success and -1 if
   *object* is not a JSON object. The reference count of all removed
   values are decremented.

.. function:: int json_object_update(json_t *object, json_t *other)

   Update *object* with the key-value pairs from *other*, overwriting
   existing keys. Returns 0 on success or -1 on error.

.. function:: int json_object_update_existing(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but only the values of existing
   keys are updated. No new keys are created. Returns 0 on success or
   -1 on error.

   .. versionadded:: 2.3

.. function:: int json_object_update_missing(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but only new keys are created.
   The value of any existing key is not changed. Returns 0 on success
   or -1 on error.

   .. versionadded:: 2.3

.. function:: int json_object_update_new(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but steals the reference to
   *other*. This is useful when *other* is newly created and not used
   after the call.

.. function:: int json_object_update_existing_new(json_t *object, json_t *other)

   Like :func:`json_object_update_new()`, but only the values of existing
   keys are updated. No new keys are created. Returns 0 on success or
   -1 on error.

.. function:: int json_object_update_missing_new(json_t *object, json_t *other)

   Like :func:`json_object_update_new()`, but only new keys are created.
   The value of any existing key is not changed. Returns 0 on success
   or -1 on error.

.. function:: int json_object_update_recursive(json_t *object, json_t *other)

   Like :func:`json_object_update()`, but object values in *other* are
   recursively merged with the corresponding values in *object* if they are also
   objects, instead of overwriting them. Returns 0 on success or -1 on error.

.. function:: void json_object_foreach(object, key, value)

   Iterate over every key-value pair of ``object``, running the block
   of code that follows each time with the proper values set to
   variables ``key`` and ``value``, of types ``const char *`` and
   :type:`json_t` pointer respectively. Example::

       /* obj is a JSON object */
       const char *key;
       json_t *value;

       json_object_foreach(obj, key, value) {
           /* block of code that uses key and value */
       }

   The items are returned in the order they were inserted to the
   object.

   **Note:** It's not safe to call ``json_object_del(object, key)`` or ``json_object_deln(object, key, key_len)``
   during iteration. If you need to, use
   :func:`json_object_foreach_safe` instead.

   This macro expands to an ordinary ``for`` statement upon
   preprocessing, so its performance is equivalent to that of
   hand-written iteration code using the object iteration protocol
   (see below). The main advantage of this macro is that it abstracts
   away the complexity behind iteration, and makes for more concise and
   readable code.

   .. versionadded:: 2.3


.. function:: void json_object_foreach_safe(object, tmp, key, value)

   Like :func:`json_object_foreach()`, but it's safe to call
   ``json_object_del(object, key)`` or ``json_object_deln(object, key, key_len)`` during iteration.
   You need to pass an extra ``void *`` parameter ``tmp`` that is used for temporary storage.

   .. versionadded:: 2.8

.. function:: void json_object_keylen_foreach(object, key, key_len, value)

   Like :c:func:`json_object_foreach`, but in *key_len* stored length of the *key*.
   Example::

       /* obj is a JSON object */
       const char *key;
       json_t *value;
       size_t len;

       json_object_keylen_foreach(obj, key, len, value) {
            printf("got key %s with length %zu\n", key, len);
       }

   **Note:** It's not safe to call ``json_object_deln(object, key, key_len)``
   during iteration. If you need to, use
   :func:`json_object_keylen_foreach_safe` instead.

   .. versionadded:: 2.14


.. function:: void json_object_keylen_foreach_safe(object, tmp, key, key_len, value)

   Like :func:`json_object_keylen_foreach()`, but it's safe to call
   ``json_object_deln(object, key, key_len)`` during iteration.
   You need to pass an extra ``void *`` parameter ``tmp`` that is used for temporary storage.

   .. versionadded:: 2.14

The following functions can be used to iterate through all key-value
pairs in an object. The items are returned in the order they were
inserted to the object.

.. function:: void *json_object_iter(json_t *object)

   Returns an opaque iterator which can be used to iterate over all
   key-value pairs in *object*, or *NULL* if *object* is empty.

.. function:: void *json_object_iter_at(json_t *object, const char *key)

   Like :func:`json_object_iter()`, but returns an iterator to the
   key-value pair in *object* whose key is equal to *key*, or NULL if
   *key* is not found in *object*. Iterating forward to the end of
   *object* only yields all key-value pairs of the object if *key*
   happens to be the first key in the underlying hash table.

.. function:: void *json_object_iter_next(json_t *object, void *iter)

   Returns an iterator pointing to the next key-value pair in *object*
   after *iter*, or *NULL* if the whole object has been iterated
   through.

.. function:: const char *json_object_iter_key(void *iter)

   Extract the associated key from *iter*.

.. function:: size_t json_object_iter_key_len(void *iter)

   Extract the associated key length from *iter*.

   .. versionadded:: 2.14

.. function:: json_t *json_object_iter_value(void *iter)

   .. refcounting:: borrow

   Extract the associated value from *iter*.

.. function:: int json_object_iter_set(json_t *object, void *iter, json_t *value)

   Set the value of the key-value pair in *object*, that is pointed to
   by *iter*, to *value*.

.. function:: int json_object_iter_set_new(json_t *object, void *iter, json_t *value)

   Like :func:`json_object_iter_set()`, but steals the reference to
   *value*. This is useful when *value* is newly created and not used
   after the call.

.. function:: void *json_object_key_to_iter(const char *key)

   Like :func:`json_object_iter_at()`, but much faster. Only works for
   values returned by :func:`json_object_iter_key()`. Using other keys
   will lead to segfaults. This function is used internally to
   implement :func:`json_object_foreach`. Example::

     /* obj is a JSON object */
     const char *key;
     json_t *value;
  
     void *iter = json_object_iter(obj);
     while(iter)
     {
         key = json_object_iter_key(iter);
         value = json_object_iter_value(iter);
         /* use key and value ... */
         iter = json_object_iter_next(obj, iter);
     }

   .. versionadded:: 2.3

.. function:: void json_object_seed(size_t seed)

    Seed the hash function used in Jansson's hashtable implementation.
    The seed is used to randomize the hash function so that an
    attacker cannot control its output.

    If *seed* is 0, Jansson generates the seed itself by reading
    random data from the operating system's entropy sources. If no
    entropy sources are available, falls back to using a combination
    of the current timestamp (with microsecond precision if possible)
    and the process ID.

    If called at all, this function must be called before any calls to
    :func:`json_object()`, either explicit or implicit. If this
    function is not called by the user, the first call to
    :func:`json_object()` (either explicit or implicit) seeds the hash
    function. See :ref:`thread-safety` for notes on thread safety.

    If repeatable results are required, for e.g. unit tests, the hash
    function can be "unrandomized" by calling :func:`json_object_seed`
    with a constant value on program startup, e.g.
    ``json_object_seed(1)``.

    .. versionadded:: 2.6


Error reporting
===============

Jansson uses a single struct type to pass error information to the
user. See sections :ref:`apiref-decoding`, :ref:`apiref-pack` and
:ref:`apiref-unpack` for functions that pass error information using
this struct.

.. type:: json_error_t

   .. member:: char text[]

      The error message (in UTF-8), or an empty string if a message is
      not available.

      The last byte of this array contains a numeric error code.  Use
      :func:`json_error_code()` to extract this code.

   .. member:: char source[]

      Source of the error. This can be (a part of) the file name or a
      special identifier in angle brackets (e.g. ``<string>``).

   .. member:: int line

      The line number on which the error occurred.

   .. member:: int column

      The column on which the error occurred. Note that this is the
      *character column*, not the byte column, i.e. a multibyte UTF-8
      character counts as one column.

   .. member:: int position

      The position in bytes from the start of the input. This is
      useful for debugging Unicode encoding problems.

The normal use of :type:`json_error_t` is to allocate it on the stack,
and pass a pointer to a function. Example::

   int main() {
       json_t *json;
       json_error_t error;

       json = json_load_file("/path/to/file.json", 0, &error);
       if(!json) {
           /* the error variable contains error information */
       }
       ...
   }

Also note that if the call succeeded (``json != NULL`` in the above
example), the contents of ``error`` are generally left unspecified.
The decoding functions write to the ``position`` member also on
success. See :ref:`apiref-decoding` for more info.

All functions also accept *NULL* as the :type:`json_error_t` pointer,
in which case no error information is returned to the caller.

.. c:enum:: json_error_code

   An enumeration containing numeric error codes.  The following errors are
   currently defined:

   ``json_error_unknown``

       Unknown error.  This should only be returned for non-errorneous
       :type:`json_error_t` structures.

   ``json_error_out_of_memory``

       The library couldn’t allocate any heap memory.

   ``json_error_stack_overflow``

       Nesting too deep.

   ``json_error_cannot_open_file``

       Couldn’t open input file.

   ``json_error_invalid_argument``

       A function argument was invalid.

   ``json_error_invalid_utf8``

       The input string isn’t valid UTF-8.

   ``json_error_premature_end_of_input``

       The input ended in the middle of a JSON value.

   ``json_error_end_of_input_expected``

       There was some text after the end of a JSON value.  See the
       ``JSON_DISABLE_EOF_CHECK`` flag.

   ``json_error_invalid_syntax``

       JSON syntax error.

   ``json_error_invalid_format``

       Invalid format string for packing or unpacking.

   ``json_error_wrong_type``

       When packing or unpacking, the actual type of a value differed from the
       one specified in the format string.

   ``json_error_null_character``

       A null character was detected in a JSON string.  See the
       ``JSON_ALLOW_NUL`` flag.

   ``json_error_null_value``

       When packing or unpacking, some key or value was ``NULL``.

   ``json_error_null_byte_in_key``

       An object key would contain a null byte.  Jansson can’t represent such
       keys; see :ref:`rfc-conformance`.

   ``json_error_duplicate_key``

       Duplicate key in object.  See the ``JSON_REJECT_DUPLICATES`` flag.

   ``json_error_numeric_overflow``

       When converting a JSON number to a C numeric type, a numeric overflow
       was detected.

   ``json_error_item_not_found``

       Key in object not found.

   ``json_error_index_out_of_range``

       Array index is out of range.

   .. versionadded:: 2.11

.. c:function:: enum json_error_code json_error_code(const json_error_t *error)

   Returns the error code embedded in ``error->text``.

   .. versionadded:: 2.11


Encoding
========

This section describes the functions that can be used to encode
values to JSON. By default, only objects and arrays can be encoded
directly, since they are the only valid *root* values of a JSON text.
To encode any JSON value, use the ``JSON_ENCODE_ANY`` flag (see
below).

By default, the output has no newlines, and spaces are used between
array and object elements for a readable output. This behavior can be
altered by using the ``JSON_INDENT`` and ``JSON_COMPACT`` flags
described below. A newline is never appended to the end of the encoded
JSON data.

Each function takes a *flags* parameter that controls some aspects of
how the data is encoded. Its default value is 0. The following macros
can be ORed together to obtain *flags*.

``JSON_INDENT(n)``
   Pretty-print the result, using newlines between array and object
   items, and indenting with *n* spaces. The valid range for *n* is
   between 0 and 31 (inclusive), other values result in an undefined
   output. If ``JSON_INDENT`` is not used or *n* is 0, no newlines are
   inserted between array and object items.

   The ``JSON_MAX_INDENT`` constant defines the maximum indentation
   that can be used, and its value is 31.

   .. versionchanged:: 2.7
      Added ``JSON_MAX_INDENT``.

``JSON_COMPACT``
   This flag enables a compact representation, i.e. sets the separator
   between array and object items to ``","`` and between object keys
   and values to ``":"``. Without this flag, the corresponding
   separators are ``", "`` and ``": "`` for more readable output.

``JSON_ENSURE_ASCII``
   If this flag is used, the output is guaranteed to consist only of
   ASCII characters. This is achieved by escaping all Unicode
   characters outside the ASCII range.

``JSON_SORT_KEYS``
   If this flag is used, all the objects in output are sorted by key.
   This is useful e.g. if two JSON texts are diffed or visually
   compared.

``JSON_PRESERVE_ORDER``
   **Deprecated since version 2.8:** Order of object keys
   is always preserved.

   Prior to version 2.8: If this flag is used, object keys in the
   output are sorted into the same order in which they were first
   inserted to the object. For example, decoding a JSON text and then
   encoding with this flag preserves the order of object keys.

``JSON_ENCODE_ANY``
   Specifying this flag makes it possible to encode any JSON value on
   its own. Without it, only objects and arrays can be passed as the
   *json* value to the encoding functions.

   **Note:** Encoding any value may be useful in some scenarios, but
   it's generally discouraged as it violates strict compatibility with
   :rfc:`4627`. If you use this flag, don't expect interoperability
   with other JSON systems.

   .. versionadded:: 2.1

``JSON_ESCAPE_SLASH``
   Escape the ``/`` characters in strings with ``\/``.

   .. versionadded:: 2.4

``JSON_REAL_PRECISION(n)``
   Output all real numbers with at most *n* digits of precision. The
   valid range for *n* is between 0 and 31 (inclusive), and other
   values result in an undefined behavior.

   By default, the precision is 17, to correctly and losslessly encode
   all IEEE 754 double precision floating point numbers.

   .. versionadded:: 2.7

``JSON_EMBED``
   If this flag is used, the opening and closing characters of the top-level
   array ('[', ']') or object ('{', '}') are omitted during encoding. This
   flag is useful when concatenating multiple arrays or objects into a stream.

   .. versionadded:: 2.10

These functions output UTF-8:

.. function:: char *json_dumps(const json_t *json, size_t flags)

   Returns the JSON representation of *json* as a string, or *NULL* on
   error. *flags* is described above. The return value must be freed
   by the caller using :func:`free()`. Note that if you have called
   :func:`json_set_alloc_funcs()` to override :func:`free()`, you should
   call your custom free function instead to free the return value.

.. function:: size_t json_dumpb(const json_t *json, char *buffer, size_t size, size_t flags)

   Writes the JSON representation of *json* to the *buffer* of
   *size* bytes. Returns the number of bytes that would be written
   or 0 on error. *flags* is described above. *buffer* is not
   null-terminated.

   This function never writes more than *size* bytes. If the return
   value is greater than *size*, the contents of the *buffer* are
   undefined. This behavior enables you to specify a NULL *buffer*
   to determine the length of the encoding. For example::

       size_t size = json_dumpb(json, NULL, 0, 0);
       if (size == 0)
           return -1;

       char *buf = alloca(size);

       size = json_dumpb(json, buf, size, 0);

   .. versionadded:: 2.10

.. function:: int json_dumpf(const json_t *json, FILE *output, size_t flags)

   Write the JSON representation of *json* to the stream *output*.
   *flags* is described above. Returns 0 on success and -1 on error.
   If an error occurs, something may have already been written to
   *output*. In this case, the output is undefined and most likely not
   valid JSON.

.. function:: int json_dumpfd(const json_t *json, int output, size_t flags)

   Write the JSON representation of *json* to the stream *output*.
   *flags* is described above. Returns 0 on success and -1 on error.
   If an error occurs, something may have already been written to
   *output*. In this case, the output is undefined and most likely not
   valid JSON.

   It is important to note that this function can only succeed on stream
   file descriptors (such as SOCK_STREAM). Using this function on a
   non-stream file descriptor will result in undefined behavior. For
   non-stream file descriptors, see instead :func:`json_dumpb()`.

   This function requires POSIX and fails on all non-POSIX systems.

   .. versionadded:: 2.10

.. function:: int json_dump_file(const json_t *json, const char *path, size_t flags)

   Write the JSON representation of *json* to the file *path*. If
   *path* already exists, it is overwritten. *flags* is described
   above. Returns 0 on success and -1 on error.

.. type:: json_dump_callback_t

   A typedef for a function that's called by
   :func:`json_dump_callback()`::

       typedef int (*json_dump_callback_t)(const char *buffer, size_t size, void *data);

   *buffer* points to a buffer containing a chunk of output, *size* is
   the length of the buffer, and *data* is the corresponding
   :func:`json_dump_callback()` argument passed through.

   *buffer* is guaranteed to be a valid UTF-8 string (i.e. multi-byte
   code unit sequences are preserved). *buffer* never contains
   embedded null bytes.

   On error, the function should return -1 to stop the encoding
   process. On success, it should return 0.

   .. versionadded:: 2.2

.. function:: int json_dump_callback(const json_t *json, json_dump_callback_t callback, void *data, size_t flags)

   Call *callback* repeatedly, passing a chunk of the JSON
   representation of *json* each time. *flags* is described above.
   Returns 0 on success and -1 on error.

   .. versionadded:: 2.2


.. _apiref-decoding:

Decoding
========

This section describes the functions that can be used to decode JSON
text to the Jansson representation of JSON data. The JSON
specification requires that a JSON text is either a serialized array
or object, and this requirement is also enforced with the following
functions. In other words, the top level value in the JSON text being
decoded must be either array or object. To decode any JSON value, use
the ``JSON_DECODE_ANY`` flag (see below).

See :ref:`rfc-conformance` for a discussion on Jansson's conformance
to the JSON specification. It explains many design decisions that
affect especially the behavior of the decoder.

Each function takes a *flags* parameter that can be used to control
the behavior of the decoder. Its default value is 0. The following
macros can be ORed together to obtain *flags*.

``JSON_REJECT_DUPLICATES``
   Issue a decoding error if any JSON object in the input text
   contains duplicate keys. Without this flag, the value of the last
   occurrence of each key ends up in the result. Key equivalence is
   checked byte-by-byte, without special Unicode comparison
   algorithms.

   .. versionadded:: 2.1

``JSON_DECODE_ANY``
   By default, the decoder expects an array or object as the input.
   With this flag enabled, the decoder accepts any valid JSON value.

   **Note:** Decoding any value may be useful in some scenarios, but
   it's generally discouraged as it violates strict compatibility with
   :rfc:`4627`. If you use this flag, don't expect interoperability
   with other JSON systems.

   .. versionadded:: 2.3

``JSON_DISABLE_EOF_CHECK``
   By default, the decoder expects that its whole input constitutes a
   valid JSON text, and issues an error if there's extra data after
   the otherwise valid JSON input. With this flag enabled, the decoder
   stops after decoding a valid JSON array or object, and thus allows
   extra data after the JSON text.

   Normally, reading will stop when the last ``]`` or ``}`` in the
   JSON input is encountered. If both ``JSON_DISABLE_EOF_CHECK`` and
   ``JSON_DECODE_ANY`` flags are used, the decoder may read one extra
   UTF-8 code unit (up to 4 bytes of input). For example, decoding
   ``4true`` correctly decodes the integer 4, but also reads the
   ``t``. For this reason, if reading multiple consecutive values that
   are not arrays or objects, they should be separated by at least one
   whitespace character.

   .. versionadded:: 2.1

``JSON_DECODE_INT_AS_REAL``
   JSON defines only one number type. Jansson distinguishes between
   ints and reals. For more information see :ref:`real-vs-integer`.
   With this flag enabled the decoder interprets all numbers as real
   values. Integers that do not have an exact double representation
   will silently result in a loss of precision. Integers that cause
   a double overflow will cause an error.

   .. versionadded:: 2.5

``JSON_ALLOW_NUL``
   Allow ``\u0000`` escape inside string values. This is a safety
   measure; If you know your input can contain null bytes, use this
   flag. If you don't use this flag, you don't have to worry about null
   bytes inside strings unless you explicitly create themselves by
   using e.g. :func:`json_stringn()` or ``s#`` format specifier for
   :func:`json_pack()`.

   Object keys cannot have embedded null bytes even if this flag is
   used.

   .. versionadded:: 2.6

``JSON_USE_BIGINT``
   This will enable the use of a big number package to be used to
   store large integer values, assuming a suitable big number package
   has been registered.  Only those JSON integers whose values can not
   be stored in a :type:`json_int_t` will use the big number
   extension.

   .. versionadded:: 2.6

``JSON_USE_BIGINT_ALWAYS``
   This flag implies ``JSON_USE_BIGINT`` and differs by using the big
   number extension to store all JSON integers, even those that could
   have otherwise been stored in a :type:`json_int_t`.

   .. versionadded:: 2.6

``JSON_USE_BIGREAL``
   This will enable the use of a big number package to be used to
   store large real values, assuming a suitable big number package has
   been registered.  Only those JSON reals whose values can not be
   accurately stored in a :type:`json_real_t`, either because their
   values or exponents are out of range or there would be a loss of
   precision (dropped significant digits), will use the big number
   extension.

   .. versionadded:: 2.6

``JSON_USE_BIGREAL_ALWAYS``
   This flag implies ``JSON_USE_BIGREAL`` and differs by using the big
   number extension to store all JSON reals, even those that could
   have otherwise been accurately stored in a :type:`json_real_t`.

   .. versionadded:: 2.6

Each function also takes an optional :type:`json_error_t` parameter
that is filled with error information if decoding fails. It's also
updated on success; the number of bytes of input read is written to
its ``position`` field. This is especially useful when using
``JSON_DISABLE_EOF_CHECK`` to read multiple consecutive JSON texts.

.. versionadded:: 2.3
   Number of bytes of input read is written to the ``position`` field
   of the :type:`json_error_t` structure.

If no error or position information is needed, you can pass *NULL*.

.. function:: json_t *json_loads(const char *input, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON string *input* and returns the array or object it
   contains, or *NULL* on error, in which case *error* is filled with
   information about the error. *flags* is described above.

.. function:: json_t *json_loadb(const char *buffer, size_t buflen, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON string *buffer*, whose length is *buflen*, and
   returns the array or object it contains, or *NULL* on error, in
   which case *error* is filled with information about the error. This
   is similar to :func:`json_loads()` except that the string doesn't
   need to be null-terminated. *flags* is described above.

   .. versionadded:: 2.1

.. function:: json_t *json_loadf(FILE *input, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text in stream *input* and returns the array or
   object it contains, or *NULL* on error, in which case *error* is
   filled with information about the error. *flags* is described
   above.

   This function will start reading the input from whatever position
   the input file was in, without attempting to seek first. If an error
   occurs, the file position will be left indeterminate. On success,
   the file position will be at EOF, unless ``JSON_DISABLE_EOF_CHECK``
   flag was used. In this case, the file position will be at the first
   character after the last ``]`` or ``}`` in the JSON input. This
   allows calling :func:`json_loadf()` on the same ``FILE`` object
   multiple times, if the input consists of consecutive JSON texts,
   possibly separated by whitespace.

.. function:: json_t *json_loadfd(int input, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text in stream *input* and returns the array or
   object it contains, or *NULL* on error, in which case *error* is
   filled with information about the error. *flags* is described
   above.

   This function will start reading the input from whatever position
   the input file descriptor was in, without attempting to seek first.
   If an error occurs, the file position will be left indeterminate.
   On success, the file position will be at EOF, unless
   ``JSON_DISABLE_EOF_CHECK`` flag was used. In this case, the file
   descriptor's position will be at the first character after the last
   ``]`` or ``}`` in the JSON input. This allows calling
   :func:`json_loadfd()` on the same file descriptor multiple times,
   if the input consists of consecutive JSON texts, possibly separated
   by whitespace.

   It is important to note that this function can only succeed on stream
   file descriptors (such as SOCK_STREAM). Using this function on a
   non-stream file descriptor will result in undefined behavior. For
   non-stream file descriptors, see instead :func:`json_loadb()`. In
   addition, please note that this function cannot be used on non-blocking 
   file descriptors (such as a non-blocking socket). Using this function 
   on non-blocking file descriptors has a high risk of data loss because 
   it does not support resuming.

   This function requires POSIX and fails on all non-POSIX systems.

   .. versionadded:: 2.10

.. function:: json_t *json_load_file(const char *path, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text in file *path* and returns the array or
   object it contains, or *NULL* on error, in which case *error* is
   filled with information about the error. *flags* is described
   above.

.. type:: json_load_callback_t

   A typedef for a function that's called by
   :func:`json_load_callback()` to read a chunk of input data::

       typedef size_t (*json_load_callback_t)(void *buffer, size_t buflen, void *data);

   *buffer* points to a buffer of *buflen* bytes, and *data* is the
   corresponding :func:`json_load_callback()` argument passed through.

   On success, the function should write at most *buflen* bytes to
   *buffer*, and return the number of bytes written; a returned value
   of 0 indicates that no data was produced and that the end of file
   has been reached. On error, the function should return
   ``(size_t)-1`` to abort the decoding process.

   In UTF-8, some code points are encoded as multi-byte sequences. The
   callback function doesn't need to worry about this, as Jansson
   handles it at a higher level. For example, you can safely read a
   fixed number of bytes from a network connection without having to
   care about code unit sequences broken apart by the chunk
   boundaries.

   .. versionadded:: 2.4

.. function:: json_t *json_load_callback(json_load_callback_t callback, void *data, size_t flags, json_error_t *error)

   .. refcounting:: new

   Decodes the JSON text produced by repeated calls to *callback*, and
   returns the array or object it contains, or *NULL* on error, in
   which case *error* is filled with information about the error.
   *data* is passed through to *callback* on each call. *flags* is
   described above.

   .. versionadded:: 2.4


.. _apiref-pack:

Building Values
===============

This section describes functions that help to create, or *pack*,
complex JSON values, especially nested objects and arrays. Value
building is based on a *format string* that is used to tell the
functions about the expected arguments.

For example, the format string ``"i"`` specifies a single integer
value, while the format string ``"[ssb]"`` or the equivalent ``"[s, s,
b]"`` specifies an array value with two strings and a boolean as its
items::

    /* Create the JSON integer 42 */
    json_pack("i", 42);

    /* Create the JSON array ["foo", "bar", true] */
    json_pack("[ssb]", "foo", "bar", 1);

Here's the full list of format specifiers. The type in parentheses
denotes the resulting JSON type, and the type in brackets (if any)
denotes the C type that is expected as the corresponding argument or
arguments.

``s`` (string) [const char \*]
    Convert a null terminated UTF-8 string to a JSON string.

``s?`` (string) [const char \*]
    Like ``s``, but if the argument is *NULL*, output a JSON null
    value.

    .. versionadded:: 2.8

``s*`` (string) [const char \*]
    Like ``s``, but if the argument is *NULL*, do not output any value.
    This format can only be used inside an object or an array. If used
    inside an object, the corresponding key is additionally suppressed
    when the value is omitted. See below for an example.

    .. versionadded:: 2.11

``s#`` (string) [const char \*, int]
    Convert a UTF-8 buffer of a given length to a JSON string. The
    length must not be negative; a negative length results in a
    ``json_error_invalid_argument`` error.

    .. versionadded:: 2.5

``s%`` (string) [const char \*, size_t]
    Like ``s#`` but the length argument is of type :type:`size_t`.

    .. versionadded:: 2.6

``+`` [const char \*]
    Like ``s``, but concatenate to the previous string. Only valid
    after ``s``, ``s#``, ``+`` or ``+#``.

    .. versionadded:: 2.5

``+#`` [const char \*, int]
    Like ``s#``, but concatenate to the previous string. Only valid
    after ``s``, ``s#``, ``+`` or ``+#``.

    .. versionadded:: 2.5

``+%`` (string) [const char \*, size_t]
    Like ``+#`` but the length argument is of type :type:`size_t`.

    .. versionadded:: 2.6

``n`` (null)
    Output a JSON null value. No argument is consumed.

``b`` (boolean) [int]
    Convert a C ``int`` to JSON boolean value. Zero is converted
    to ``false`` and non-zero to ``true``.

``i`` (integer) [int]
    Convert a C ``int`` to JSON integer.

``I`` (integer) [json_int_t]
    Convert a C :type:`json_int_t` to JSON integer.

``z`` (big integer) [json_bigz_t]
    Convert a C :type:`json_bigz_const_t` (a constant-pointer version
    of a :type:`json_bigz_t`) to a JSON integer. The provided value
    will be copied, no reference to it will be maintained. You must
    have already registered a suitable big number package extension.

``f`` (real) [double]
    Convert a C ``double`` to JSON real.

``r`` (big real) [json_bigr_t]
    Convert a C :type:`json_bigr_const_t` (a constant-pointer version
    of a :type:`json_bigr_t`) to a JSON real. The provided value will
    be copied, no reference to it will be maintained. You must have
    already registered a suitable big number package extension.

``o`` (any value) [json_t \*]
    Output any given JSON value as-is. If the value is added to an
    array or object, the reference to the value passed to ``o`` is
    stolen by the container.

``O`` (any value) [json_t \*]
    Like ``o``, but the argument's reference count is incremented.
    This is useful if you pack into an array or object and want to
    keep the reference for the JSON value consumed by ``O`` to
    yourself.

``o?``, ``O?`` (any value) [json_t \*]
    Like ``o`` and ``O``, respectively, but if the argument is
    *NULL*, output a JSON null value.

    .. versionadded:: 2.8

``o*``, ``O*`` (any value) [json_t \*]
    Like ``o`` and ``O``, respectively, but if the argument is
    *NULL*, do not output any value. This format can only be used
    inside an object or an array. If used inside an object, the
    corresponding key is additionally suppressed. See below for an
    example.

    .. versionadded:: 2.11

``[fmt]`` (array)
    Build an array with contents from the inner format string. ``fmt``
    may contain objects and arrays, i.e. recursive value building is
    supported.

``{fmt}`` (object)
    Build an object with contents from the inner format string
    ``fmt``. The first, third, etc. format specifier represent a key,
    and must be a string (see ``s``, ``s#``, ``+`` and ``+#`` above),
    as object keys are always strings. The second, fourth, etc. format
    specifier represent a value. Any value may be an object or array,
    i.e. recursive value building is supported.

Whitespace, ``:`` and ``,`` are ignored.

.. function:: json_t *json_pack(const char *fmt, ...)

   .. refcounting:: new

   Build a new JSON value according to the format string *fmt*. For
   each format specifier (except for ``{}[]n``), one or more arguments
   are consumed and used to build the corresponding value. Returns
   *NULL* on error.

.. function:: json_t *json_pack_ex(json_error_t *error, size_t flags, const char *fmt, ...)
              json_t *json_vpack_ex(json_error_t *error, size_t flags, const char *fmt, va_list ap)

   .. refcounting:: new

   Like :func:`json_pack()`, but an in the case of an error, an error
   message is written to *error*, if it's not *NULL*. The *flags*
   parameter is currently unused and should be set to 0.

   As only the errors in format string (and out-of-memory errors) can
   be caught by the packer, these two functions are most likely only
   useful for debugging format strings.

More examples::

  /* Build an empty JSON object */
  json_pack("{}");

  /* Build the JSON object {"foo": 42, "bar": 7} */
  json_pack("{sisi}", "foo", 42, "bar", 7);

  /* Like above, ':', ',' and whitespace are ignored */
  json_pack("{s:i, s:i}", "foo", 42, "bar", 7);

  /* Build the JSON array [[1, 2], {"cool": true}] */
  json_pack("[[i,i],{s:b}]", 1, 2, "cool", 1);

  /* Build a string from a non-null terminated buffer */
  char buffer[4] = {'t', 'e', 's', 't'};
  json_pack("s#", buffer, 4);

  /* Concatenate strings together to build the JSON string "foobarbaz" */
  json_pack("s++", "foo", "bar", "baz");

  /* Create an empty object or array when optional members are missing */
  json_pack("{s:s*,s:o*,s:O*}", "foo", NULL, "bar", NULL, "baz", NULL);
  json_pack("[s*,o*,O*]", NULL, NULL, NULL);


.. _apiref-unpack:

Parsing and Validating Values
=============================

This section describes functions that help to validate complex values
and extract, or *unpack*, data from them. Like :ref:`building values
<apiref-pack>`, this is also based on format strings.

While a JSON value is unpacked, the type specified in the format
string is checked to match that of the JSON value. This is the
validation part of the process. In addition to this, the unpacking
functions can also check that all items of arrays and objects are
unpacked. This check be enabled with the format specifier ``!`` or by
using the flag ``JSON_STRICT``. See below for details.

Here's the full list of format specifiers. The type in parentheses
denotes the JSON type, and the type in brackets (if any) denotes the C
type whose address should be passed.

``s`` (string) [const char \*]
    Convert a JSON string to a pointer to a null terminated UTF-8
    string. The resulting string is extracted by using
    :func:`json_string_value()` internally, so it exists as long as
    there are still references to the corresponding JSON string.

``s%`` (string) [const char \*, size_t \*]
    Convert a JSON string to a pointer to a null terminated UTF-8
    string and its length.

    .. versionadded:: 2.6

``n`` (null)
    Expect a JSON null value. Nothing is extracted.

``b`` (boolean) [int]
    Convert a JSON boolean value to a C ``int``, so that ``true``
    is converted to 1 and ``false`` to 0.

``i`` (integer) [int]
    Convert a JSON integer to C ``int``.

``I`` (integer) [json_int_t]
    Convert a JSON integer to C :type:`json_int_t`.

``z`` (big integer) [json_bigz_t]
    Convert a JSON big integer to a C :type:`json_bigz_t`. The
    returned pointer will reference a newly allocated big number; you
    are responsible for eventually freeing it. You must have already
    registered a suitable big number package extension.

``Z`` (any integer) [json_bigz_t]
    Like ``z``, except that both plain integers and big integers will
    be accepted. When extracting values, a big integer will always be
    returned.

``f`` (real) [double]
    Convert a JSON real to C ``double``.

``F`` (integer or real) [double]
    Convert a JSON number (integer or real) to C ``double``.

``r`` (big real) [json_bigr_t]
    Convert a JSON big real to a C :type:`json_bigr_t`. The returned
    pointer will reference newly allocated big number; you are
    responsible for eventually freeing it. You must have already
    registered a suitable big number package extension.

``R`` (any integer) [json_bigr_t]
    Like``r``, except that both plain reals and big reals will be
    accepted. When extracting values, a big real will always be
    returned.

``o`` (any value) [json_t \*]
    Store a JSON value with no conversion to a :type:`json_t` pointer.

``O`` (any value) [json_t \*]
    Like ``o``, but the JSON value's reference count is incremented.
    Storage pointers should be initialized NULL before using unpack.
    The caller is responsible for releasing all references incremented
    by unpack, even when an error occurs.

``v`` (any scalar) [json_t \*]
    Store any JSON scalar value (any type except lists or objects)
    with no conversion to a :type:`json_t` pointer.

``V`` (any scalar) [json_t \*]
    Like ``v``, but the JSON value's reference count is incremented.

``[fmt]`` (array)
    Convert each item in the JSON array according to the inner format
    string. ``fmt`` may contain objects and arrays, i.e. recursive
    value extraction is supported.

``{fmt}`` (object)
    Convert each item in the JSON object according to the inner format
    string ``fmt``. The first, third, etc. format specifier represent
    a key, and must be ``s``. The corresponding argument to unpack
    functions is read as the object key. The second, fourth, etc.
    format specifier represent a value and is written to the address
    given as the corresponding argument. **Note** that every other
    argument is read from and every other is written to.

    ``fmt`` may contain objects and arrays as values, i.e. recursive
    value extraction is supported.

    .. versionadded:: 2.3
       Any ``s`` representing a key may be suffixed with a ``?`` to
       make the key optional. If the key is not found, nothing is
       extracted. See below for an example.

``!``
    This special format specifier is used to enable the check that
    all object and array items are accessed, on a per-value basis. It
    must appear inside an array or object as the last format specifier
    before the closing bracket or brace. To enable the check globally,
    use the ``JSON_STRICT`` unpacking flag.

``*``
    This special format specifier is the opposite of ``!``. If the
    ``JSON_STRICT`` flag is used, ``*`` can be used to disable the
    strict check on a per-value basis. It must appear inside an array
    or object as the last format specifier before the closing bracket
    or brace.

Whitespace, ``:`` and ``,`` are ignored.

.. function:: int json_unpack(json_t *root, const char *fmt, ...)

   Validate and unpack the JSON value *root* according to the format
   string *fmt*. Returns 0 on success and -1 on failure.

.. function:: int json_unpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, ...)
              int json_vunpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, va_list ap)

   Validate and unpack the JSON value *root* according to the format
   string *fmt*. If an error occurs and *error* is not *NULL*, write
   error information to *error*. *flags* can be used to control the
   behaviour of the unpacker, see below for the flags. Returns 0 on
   success and -1 on failure.

.. note::

   The unpacking will halt at the first error, which may leave some of
   the variables that you designated to hold unpacked values in an
   uninitialized state.  In partciular, as the big number formats
   (``z``, ``Z``, ``r``, and ``R``) return pointers to newly allocated
   memory, it is good practice to always initialize those
   corresponding variables to *NULL* prior to unpacking, and to
   remember to free the memory for those (if not null) afterwards,
   regardless if the unpacking succeeded or resulted in an error.

.. note::

   The first argument of all unpack functions is ``json_t *root``
   instead of ``const json_t *root``, because the use of ``O`` format
   specifier causes the reference count of ``root``, or some value
   reachable from ``root``, to be increased. Furthermore, the ``o``
   format specifier may be used to extract a value as-is, which allows
   modifying the structure or contents of a value reachable from
   ``root``.

   If the ``O`` and ``o`` format specifiers are not used, it's
   perfectly safe to cast a ``const json_t *`` variable to plain
   ``json_t *`` when used with these functions.

The following unpacking flags are available:

``JSON_STRICT``
    Enable the extra validation step checking that all object and
    array items are unpacked. This is equivalent to appending the
    format specifier ``!`` to the end of every array and object in the
    format string.

``JSON_VALIDATE_ONLY``
    Don't extract any data, just validate the JSON value against the
    given format string. Note that object keys must still be specified
    after the format string.

Examples::

    /* root is the JSON integer 42 */
    int myint;
    json_unpack(root, "i", &myint);
    assert(myint == 42);

    /* root is the JSON object {"foo": "bar", "quux": true} */
    const char *str;
    int boolean;
    json_unpack(root, "{s:s, s:b}", "foo", &str, "quux", &boolean);
    assert(strcmp(str, "bar") == 0 && boolean == 1);

    /* root is the JSON array [[1, 2], {"baz": null} */
    json_error_t error;
    json_unpack_ex(root, &error, JSON_VALIDATE_ONLY, "[[i,i], {s:n}]", "baz");
    /* returns 0 for validation success, nothing is extracted */

    /* root is the JSON array [1, 2, 3, 4, 5] */
    int myint1, myint2;
    json_unpack(root, "[ii!]", &myint1, &myint2);
    /* returns -1 for failed validation */

    /* root is an empty JSON object */
    int myint = 0, myint2 = 0, myint3 = 0;
    json_unpack(root, "{s?i, s?[ii]}",
                "foo", &myint1,
                "bar", &myint2, &myint3);
    /* myint1, myint2 or myint3 is no touched as "foo" and "bar" don't exist */


Equality
========

Testing for equality of two JSON values cannot, in general, be
achieved using the ``==`` operator. Equality in the terms of the
``==`` operator states that the two :type:`json_t` pointers point to
exactly the same JSON value. However, two JSON values can be equal not
only if they are exactly the same value, but also if they have equal
"contents":

* Two integer or two real values are equal if their contained numeric
  values are equal. Comparisons between big number and corresponding
  regular number types are allowable, and are likewise determined based
  upon their equivalent numeric values. However, an integer value is
  never equal to a real value.

* Two strings are equal if their contained UTF-8 strings are equal,
  byte by byte. Unicode comparison algorithms are not implemented.

* Two arrays are equal if they have the same number of elements and
  each element in the first array is equal to the corresponding
  element in the second array.

* Two objects are equal if they have exactly the same keys and the
  value for each key in the first object is equal to the value of the
  corresponding key in the second object.

* Two true, false or null values have no "contents", so they are equal
  if their types are equal. (Because these values are singletons,
  their equality can actually be tested with ``==``.)

* Values nested more deeply than ``JSON_PARSER_MAX_DEPTH`` are not
  recursed into, and thus cosidered unequal, to avoid stack exhaustion.

.. function:: int json_equal(json_t *value1, json_t *value2)

   Returns 1 if *value1* and *value2* are equal, as defined above.
   Returns 0 if they are unequal or one or both of the pointers are
   *NULL*.

Copying
=======

Because of reference counting, passing JSON values around doesn't
require copying them. But sometimes a fresh copy of a JSON value is
needed. For example, if you need to modify an array, but still want to
use the original afterwards, you should take a copy of it first.

Jansson supports two kinds of copying: shallow and deep. There is a
difference between these methods only for arrays and objects. Shallow
copying only copies the first level value (array or object) and uses
the same child values in the copied value. Deep copying makes a fresh
copy of the child values, too. Moreover, all the child values are deep
copied in a recursive fashion.

Copying objects preserves the insertion order of keys.

.. function:: json_t *json_copy(json_t *value)

   .. refcounting:: new

   Returns a shallow copy of *value*, or *NULL* on error.

.. function:: json_t *json_deep_copy(const json_t *value)

   .. refcounting:: new

   Returns a deep copy of *value*, or *NULL* on error. Copying fails if
   *value* is nested more deeply than ``JSON_PARSER_MAX_DEPTH``, to avoid
   stack exhaustion.


.. _apiref-custom-memory-allocation:

Custom Memory Allocation
========================

By default, Jansson uses :func:`malloc()`, :func:`realloc()` and :func:`free()` for
memory allocation. These functions can be overridden if custom
behavior is needed.

.. type:: json_malloc_t

   A typedef for a function pointer with :func:`malloc()`'s
   signature::

       typedef void *(*json_malloc_t)(size_t);

.. type:: json_realloc_t

   A typedef for a function pointer with :func:`realloc()`'s
   signature::

       typedef void *(*json_realloc_t)(void*, size_t);


   .. versionadded:: 2.15

.. type:: json_free_t

   A typedef for a function pointer with :func:`free()`'s
   signature::

       typedef void (*json_free_t)(void *);

.. type:: json_realloc_t

   A typedef for a function pointer with :func:`realloc()`'s
   signature::

       typedef void (*json_realloc_t)(void *, size_t);

   Jansson does not itself use a realloc function, though it will pass
   it on to big number extensions that may require a realloc.

.. type:: json_overwrite_t

   A typedef for a function pointer that will securely overwrite
   a region of memory that has a signature::

       typedef void (*json_overwrite_t)(void *, size_t);

.. function:: void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn)

   Use *malloc_fn* instead of :func:`malloc()` and *free_fn* instead
   of :func:`free()`. This function has to be called before any other
   Jansson's API functions to ensure that all memory operations use
   the same functions.

.. function:: void json_set_alloc_funcs2(json_malloc_t malloc_fn, json_relloc_t relloc_fn, json_free_t free_fn)

   Use *malloc_fn* instead of :func:`malloc()`, *realloc_fn* instead of :func:`realloc()` and *free_fn* instead
   of :func:`free()`. This function has to be called before any other
   Jansson's API functions to ensure that all memory operations use
   the same functions.

   .. versionadded:: 2.15

.. function:: void json_get_alloc_funcs(json_malloc_t *malloc_fn, json_free_t *free_fn)

   Fetch the current malloc_fn and free_fn used. Either parameter
   may be NULL.

   .. versionadded:: 2.8

.. function:: void json_get_alloc_funcs2(json_malloc_t *malloc_fn, json_realloc_t *realloc_fn, json_free_t *free_fn)

   Fetch the current malloc_fn,realloc_fn and free_fn used. Either parameter
   may be NULL.

   .. versionadded:: 2.15

**Examples:**

Circumvent problems with different CRT heaps on Windows by using
application's :func:`malloc()` and :func:`free()`::

    json_set_alloc_funcs(malloc, free);

Use the `Boehm's conservative garbage collector`_ for memory
operations::

    json_set_alloc_funcs(GC_malloc, GC_free);
         or
    json_set_alloc_funcs2(GC_malloc, GC_realloc, GC_free);


   When realloc is not defined is it emulated using malloc and free.


.. _Boehm's conservative garbage collector: http://www.hboehm.info/gc/

Allow storing sensitive data (e.g. passwords or encryption keys) in
JSON structures by zeroing all memory when freed. You do not need to
provide the secure_realloc function unless you use a big number
extension which uses realloc::

    static void secure_overwrite(void *ptr, size_t size)
    {
        guaranteed_memset(ptr, 0, size);
    }

    static void *secure_malloc(size_t size)
    {
        /* Store the memory area size in the beginning of the block */
        void *ptr = malloc(size + 8);
        *((size_t *)ptr) = size;
        return ptr + 8;
    }

    static void secure_free(void *ptr)
    {
        size_t size;

        ptr -= 8;
        size = *((size_t *)ptr);

        secure_overwrite(ptr, size + 8);
        free(ptr);
    }

    static void *secure_realloc(void *ptr, size_t size)
    {
        size_t oldsize;

        if(ptr == NULL)
            return secure_malloc(size);
        if(size == 0) {
            secure_free(ptr);
            return NULL;
        }

        oldsize  = *((size_t *)(ptr - 8));

        if( oldsize > size ) {
            secure_overwrite(ptr+size, oldsize-size);
        }
        else if( oldsize < size ) {
            void *newptr = secure_malloc(size);
            memcpy(newptr, ptr, size);
            secure_free(ptr);
            ptr = newptr;
        }
        return ptr;
    }

    int main()
    {
        json_set_alloc_funcs(secure_malloc, secure_free);
        json_set_realloc_func(secure_realloc);
        json_set_overwrite_func(secure_overwrite);
        /* ... */
    }

For more information about the issues of storing sensitive data in
memory, see
http://www.dwheeler.com/secure-programs/Secure-Programs-HOWTO/protect-secrets.html.
The page also explains the :func:`guaranteed_memset()` function used
in the example and gives a sample implementation for it.

.. _fixed_length_keys:

Fixed-Length keys
=================

The Jansson API allows work with fixed-length keys. This can be useful in the following cases:

* The key is contained inside a buffer and is not null-terminated. In this case creating a new temporary buffer is not needed.
* The key contains U+0000 inside it.

List of API for fixed-length keys:

* :c:func:`json_object_getn`
* :c:func:`json_object_setn`
* :c:func:`json_object_setn_nocheck`
* :c:func:`json_object_setn_new`
* :c:func:`json_object_setn_new_nocheck`
* :c:func:`json_object_deln`
* :c:func:`json_object_iter_key_len`
* :c:func:`json_object_keylen_foreach`
* :c:func:`json_object_keylen_foreach_safe`

**Examples:**

Try to write a new function to get :c:struct:`json_t` by path separated by ``.``

This requires:

* string iterator (no need to modify the input for better performance)
* API for working with fixed-size keys

The iterator::

    struct string {
        const char *string;
        size_t length;
    };

    size_t string_try_next(struct string *str, const char *delimiter) {
        str->string += strspn(str->string, delimiter);
        str->length = strcspn(str->string, delimiter);
        return str->length;
    }

    #define string_foreach(_string, _delimiter) \
            for (; string_try_next(&(_string), _delimiter); (_string).string += (_string).length)


The function::

    json_t *json_object_get_by_path(json_t *object, const char *path) {
        struct string str;
        json_t *out = object;

        str.string = path;

        string_foreach(str, ".") {
            out = json_object_getn(out, str.string, str.length);
            if (out == NULL)
                return NULL;
        }

        return out;
    }

And usage::

    int main(void) {
        json_t *obj = json_pack("{s:{s:{s:b}}}", "a", "b", "c", 1);

        json_t *c = json_object_get_by_path(obj, "a.b.c");
        assert(json_is_true(c));

        json_decref(obj);

.. _apiref-big-number-extension:

Big Number Extensions
=====================

It is possible to extend Jansson so that it may support numeric values
that are larger or more precise than may be represented by the native
C types. Big number extensions may be independently provided for
integer values and real values.

The extension API is designed to be generic so that many different big
number packages may be used.  Each extension is enabled by registering
a set of user-supplied callback functions that perform basic
operations on a big number.  These callbacks will often be thin
wrapper functions around the routines provided by the big number
package being used.

For more information on big numbers and a list of some software
packages which support them see
http://en.wikipedia.org/wiki/Arbitrary-precision_arithmetic


Big number types and functions
------------------------------

Within Jansson, a big number type is represented as an opaque pointer
type of ``json_bigz_t`` and ``json_bigr_t`` for big integers and big
reals respectively.  There are corresponding constant-pointer types
as well, ``json_bigz_const_t`` and ``json_bigr_const_t``.

By default these pointer types are declared as ``void *`` or ``void
const *``.  To allow for better type safety, the user may provide a
more specific type name for these pointers by defining a macro prior
to including the Jansson header file; for example if using GMP::

    #define JSON_BIGZ_TYPE mpz_t
    #define JSON_BIGR_TYPE mpf_t
    #include <jansson.h>

Then the ``json_bigz_t`` type will be equivalent to ``mpz_t *``,
and similar for the real types.

.. note:: Jansson adopts the convention of using the letter *Z* to mean
    integer and *R* to mean a real number. Do not let this cause you
    confusion, as some big number packages may use *F* for reals
    instead.

.. function:: json_t *json_biginteger(json_bigz_const_t value);

   .. refcounting:: new

   Returns a new JSON big integer, or *NULL* on error. The passed-in
   value is copied.

.. function:: json_bigz_const_t json_biginteger_value(const json_t *biginteger)

   Returns the pointer to the value of *biginteger*, or *NULL* if it
   is not a JSON big integer. Note that the returned pointer is a
   reference to the existing value and not a copy; use caution if
   retaining this reference.

.. function:: int json_biginteger_set(json_t *integer, json_bigz_const_t value)

   Sets the associated value of *biginteger* to *value*. Returns 0 on
   success and -1 if *biginteger* is not a JSON big integer. A copy is
   made of *value*.

.. function:: json_t *json_bigreal(json_bigr_const_t value);

   .. refcounting:: new

   Returns a new JSON big real, or *NULL* on error. The passed-in
   value is copied.

.. function:: json_bigr_const_t json_bigreal_value(const json_t *bigreal)

   Returns the pointer to the value of *bigreal*, or *NULL* if it is
   not a JSON big real. Note that the returned pointer is a reference
   to the existing value and not a copy; use caution if retaining this
   reference.

.. function:: int json_bigreal_set(json_t *bigreal, json_bigr_const_t value)

   Sets the associated value of *bigreal* to *value*. Returns 0 on
   success and -1 if *bigreal* is not a JSON big real. A copy is
   made of *value*.

Sample bignum packages
----------------------

Included with the Jansson distribution are a sampling of extensions
for supporting several popular big number formats. These samples are C
code files which may be included and compiled into your own
application.

*C long double:* The C standard provides a native type ``long double``
which may have greater range and precision than a plain ``double``
for real numbers. To use it add the sample file ``json_bignum_ldbl.c``
into your project sources. Then arrange your own source similar to::

    #define JSON_BIGR_TYPE long double
    #include <jansson.h>

    int main()
    {
        json_use_ldbl_for_bigreals();
        ...
    }

*GNU Quadmath:* The GNU libquadmath library provides support for
quad-precision floating-point numbers, which may have even greater
range and precision than a ``long double``. Quadmath is built into
newer versions of the GCC compiler on some platforms, so that it acts
like a native type. To use it add the sample file
``json_bignum_quad.c`` into your project sources. Then arrange your
own source similar to::

    #include <quadmath.h>
    #define JSON_BIGR_TYPE __float128
    #include <jansson.h>

    int main()
    {
        json_use_quad_for_bigreals();
        ...
    }

Note that you may need to link your project with an additional system
library typically named *libquadmath* (use the ``-lquadmath`` linker option
in Unix-like environments).  For more information see
http://gcc.gnu.org/onlinedocs/libquadmath/


*GNU Multiprecision Library (GMP):* The GMP package supports arbitrary
sized integers and arbitrary precision real numbers.  To use it add
the sample file ``json_bignum_gmp.c`` into your project sources. Then
arrange your own source similar to::

    #include <gmp.h>
    #define JSON_BIGZ_TYPE mpz_t
    #define JSON_BIGR_TYPE mpf_t
    #include <jansson.h>

    int main()
    {
        json_use_gmp_for_bigintegers();
        json_use_gmp_for_bigreals();
        mpf_set_default_prec( 1024 ); /* precision in bits */
        ...
    }

Notice that you may wish to call the GMP function
`mpf_set_default_prec` to set up the default number of bits
of precision that are kept. Jansson will always attempt to preserve
all of the digits, but this GMP default if not set appropriately may
still result in loss of significant digits.

You will need to link your project with the GMP library (use the
``-lgmp`` linker option in Unix-like environments).  For more
information see http://gmplib.org/

*OpenSSL Big Numbers (BN):* The OpenSSL cryptographic library provides
generic support for arbitrary sized integers.  To use it add the
sample file ``json_bignum_openssl.c`` into your project sources. Then
arrange your own source similar to::

    #include <openssl/bn.h>
    #define JSON_BIGZ_TYPE BIGNUM
    #include <jansson.h>

    int main()
    {
        json_use_openssl_for_bigintegers();
        ...
    }

You will need to link your project with the OpenSSL *crypto* library
(use the ``-lcrypto`` linker option in Unix-like environments).  For
more information see http://www.openssl.org/docs/crypto/bn.html


The callback functions
----------------------

To use a big number package you must provide a set of callback
functions that manipulate big number values. There are six callbacks
needed: copy, delete, compare, convert to string, convert from string,
and up-convert from a native number type.  The callbacks are generally
the same for both big integers and big reals except for the specific
type signatures.


When invoked by Jansson, all of the callback function will be provided
with a ``json_memory_funcs_t`` argument that contains the set of
function pointers for common memory-related routines. The callback
functions should make use of these functions when possible so that
both Jansson and the big number package use compatible memory
management routines. The members of the ``json_memory_funcs_t``
include::

    malloc_fn(size_t size)
    free_fn(void *ptr)
    realloc_fn(void *ptr, size_t size)
    overwrite_fn(void *ptr, size_t size)

Each big number package must provide the following callback functions,
shown here for big integers:

.. type:: json_bigint_copy_t

   A typedef for a function pointer that will make a copy of a big integer value. It has a signature::

       typedef json_bigz_t (*json_bigint_copy_t)(json_bigz_const_t bignum, const json_memory_funcs_t *memfuncs)

.. type:: json_bigint_del_t

   A typedef for a function pointer that will delete a big integer value. It has a signature::

       typedef void (*json_bigint_del_t)(json_bigz_t bignum, const json_memory_funcs_t *memfuncs)

.. type:: json_bigint_cmp_t

    A typedef for a function pointer that will numerically compare two big integer values, returning -1 (less-than), 0 (equal), or +1 (greater-than). It has a signature::

        typedef int (*json_bigint_cmp_t)(json_bigz_const_t bignum1, json_bigz_const_t bignum2, const json_memory_funcs_t *memfuncs)

.. type:: json_bigint_to_str_t

    A typedef for a function pointer that will convert a big number
    value into a decimal string format. The resulting format must
    adhere strictly to the JSON standard syntax, e.g., "+0" or ".3"
    are invalid. It has a signature::

        typedef int (*json_bigint_to_str_t)(json_bigz_const_t bignum, char *buffer, size_t size, const json_memory_funcs_t *memfuncs)

    The function will be provided a buffer, identified with ``buffer``
    and ``size``, into which it should write the string. The string
    must be null terminated.  The length of the string excluding the
    null byte must be returned.

    If the resulting string is too large to fit into the provided
    buffer, then the function should return the number of bytes needed
    (excluding the null terminator), after which the function will be
    called again with a larger buffer. In this case it is not
    necessary to write into the buffer or insure it is null
    terminated.

.. type:: json_bigint_from_str_t

    A typedef for a function pointer that will convert a decimal
    string repreentation of a number (in standard JSON syntax) into a
    big number value. It has the signature::

        typedef json_bigz_t (*json_bigint_from_str_t)(const char *value, const json_memory_funcs_t *memfuncs)

    The returned value should be a newly-allocated big number type. If
    the string value can not be converted then *NULL* should be
    returned.

.. type:: json_bigint_from_int_t

    A typedef for a function pointer that will convert a standard
    native type of :type:`json_int_t` into a big number value. It has the signture::

        typedef json_bigz_t (*json_bigint_from_int_t)(json_int_t value, const json_memory_funcs_t *memfuncs)

    The returned value should be a newly-allocated big number type.

To register the callback functions, they must first be assembled into a structure of type :type:`json_bigint_funcs_t` or :type:`json_bigreal_funcs_t`, for example::

    json_bigint_funcs_t callbacks;
    callbacks.copy_fn = my_copy_function;
    callbacks.delete_fn = my_delete_function;
    callbacks.compare_fn = my_compare_function;
    callbacks.to_string_fn = my_to_string_function;
    callbacks.from_string_fn = my_from_string_function;
    callbacks.from_int_fn = my_from_int_function;

Callbacks for big reals are similar except the last member is named ``from_real_fn`` instead of ``from_int_fn``.

Then the whole set of callbacks are registered with a single API call.

.. function:: void json_set_biginteger_funcs(const json_bigint_funcs_t* functions)

     Registers the set of callback functions to use for manipulating
     big integer values. Passing *NULL* will unregister any callback
     functions and disable the use of big integers.  This function
     should be called before any other functions that may involve big
     integers, such as loading JSON.

.. function:: void json_set_bigreal_funcs(const json_bigreal_funcs_t* functions)

     Registers the set of callback functions to use for manipulating
     big real values. Passing *NULL* will unregister any callback
     functions and disable the use of big reals.  This function should
     be called before any other functions that may involve big
     integers, such as loading JSON.

Example using long double
-------------------------

The following example code shows how to use the big number extensions
to support the native C type of ``long double`` for big real numbers.
This is essentially the same as what is in the supplied
``json_bignum_ldbl.c`` sample::

    #include <string.h>
    #include <stdlib.h>
    #include <float.h>
    #include <errno.h>

    #define JSON_BIGR_TYPE long double
    #include <jansson.h>

    /* These typedefs are now in effect:
         json_bigr_t        =>  long double *
         json_bigr_const_t  =>  long double const *
    */

    static int json_bigreal_ldbl_compare(json_bigr_const_t r1, json_bigr_const_t r2,
                                         const json_memory_funcs_t *memfuncs)
    {
        const long double * f1 = r1;
        const long double * f2 = r2;

        if( *f1 == *f2 ) return 0;
        if( *f1 < *f2 )  return -1;
        return 1;
    }

    static json_bigr_t json_bigreal_ldbl_copy(json_bigr_const_t r,
                                              const json_memory_funcs_t *memfuncs)
    {
        const long double * f1 = r;
        long double * f2;

        f2 = memfuncs->malloc_fn( sizeof(long double) );
        if(!f2)
            return NULL;
        *f2 = *f1;
        return f2;
    }

    static void json_bigreal_ldbl_delete(json_bigr_t r,
                                         const json_memory_funcs_t *memfuncs)
    {
        long double * f = r;

        memfuncs->free_fn( f );
        return;
    }

    static json_bigr_t json_bigreal_ldbl_from_real(double value,
                                                   const json_memory_funcs_t *memfuncs)
    {
        long double * f;

        f = memfuncs->malloc_fn( sizeof(long double) );
        if(!f)
            return NULL;
        *f = value;
        return f;
    }

    static json_bigr_t json_bigreal_ldbl_from_str(const char *value,
                                                  const json_memory_funcs_t *memfuncs)
    {
        long double f0;
        long double *f1;

        errno = 0;
        f0 = strtold( value, NULL );

        f1 = json_bigreal_ldbl_copy( &f0, memfuncs );
	memfuncs->overwrite_fn( &f0, sizeof(long double) );
        return f1;
    }

    static int json_bigreal_ldbl_to_str(json_bigr_const_t r, char *buffer, size_t size,
                                        const json_memory_funcs_t *memfuncs)
    {
        const long double *f = r;
        int outsize;

        outsize = snprintf( buffer, size, "%.*Lg", LDBL_DIG, *f );
        return outsize;
    }

    int json_use_ldbl_for_bigreals()
    {
        static json_bigreal_funcs_t funcs;
        funcs.copy_fn        = json_bigreal_ldbl_copy;
        funcs.delete_fn      = json_bigreal_ldbl_delete;
        funcs.compare_fn     = json_bigreal_ldbl_compare;
        funcs.to_string_fn   = json_bigreal_ldbl_to_str;
        funcs.from_string_fn = json_bigreal_ldbl_from_str;
        funcs.from_real_fn   = json_bigreal_ldbl_from_real;

        json_set_bigreal_funcs( &funcs );
        return 0;
    }

    int main()
    {
        json_use_ldbl_for_bigreals();
        ...
    }
