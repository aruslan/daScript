.. _builtin_functions:


==================
Built-in Functions
==================

.. index::
    single: Built-in Functions
    pair: Global Symbols; Built-in Functions


^^^^^^^^^^^^^^
Misc
^^^^^^^^^^^^^^
.. js:function:: panic

    will cause panic. The program will be determinated if there is no recover.
    Panic is not a error handling mechanism and can not be used as such. It is indeed panic, fatal error.
    It is not supposed that program can completely correctly recover from panic, recover construction is provided so program can try to correcly shut-down or report fatal error.
    If there is no recover withing script, it will be called in calling eval (in C++ callee code).

.. js:function:: print(x)

    print prints any provided argument `x`, provided that type has DataWalker 'to string' (all PODs do have it).

.. js:function:: stackwalk

    stackwalk prints call stack and local variables values

.. js:function:: terminate

    terminates program execution

.. js:function:: breakpoint

    breakpoint will call os_debugbreakpoint, which is link-time unresolved dependency. It's supposed to call breakpoint in debugger tool, as sample implementation does.

.. js:function:: heap_bytes_allocated

    heap_bytes_allocated will return bytes allocated on heap (i.e. really used, not reserved)


.. js:function:: assert(x, str)

    assert will cause application defined assert if `x` argument is false.
    assert can and probably will be removed from release builds.
    That's why assert will not compile, if `x` argument has side effect (for example, calling function with side effects).

.. js:function:: verify(x, str)

    verify will cause application defined assert if `x` argument is false.
    verify check can be removed from release builds, but execution of `x` argument staus.
    That's why verify, unlike assert can have side effects in evaluating `x`

.. js:function:: static_assert(x, str)

    static_assert will cause compiler to stop compilation if `x` argument is false.
    That's why `x` has to be compile-time known constant.
    static_assert will be removed from compiled program.

.. js:function:: debug(x, str)

    debug will print string str and value of x (like print). However, debug also returns value of x, which makes it suitable for debugging expressions::

        let mad = debug(x, "x") * debug(y, "y") + debug(z, "z") // x*y + z


.. js:function:: invoke(block_or_function, arguments)

    invoke will call block or pointer to function (`block_or_function`) with provided list of arguments

^^^^^^^^
Arrays
^^^^^^^^

.. js:function:: push(array_arg, value[, at])

    push will push to dynamic array `array_arg` the content of `value`. `value` has to be of the same type (or const reference to same type) as array values.
    if `at` is provided `value` will be pushed at index `at`, otherwise to the end of array.
    The `content` of value will be copied (assigned) to it.

.. js:function:: emplace(array_arg, value[, at])

    emplace will push to dynamic array `array_arg` the content of `value`. `value` has to be of the same type (or const reference to same type) as array values.
    if `at` is provided `value` will be pushed at index `at`, otherwise to the end of array.
    The `content` of value will be moved (<-) to it.

.. js:function:: resize(array_arg, new_size)

    Resize will resize `array_arg` array to a new size of `new_size`. If new_size is bigger than current, new elements will be zeroed.

.. js:function:: erase(array_arg, at)

    erase will erase `at` index element in `array_arg` array.

.. js:function:: length(array_arg)

    length will return current size of array `array_arg`.


.. js:function:: clear(array_arg)

    clear will clear whole array `array_arg`. The size of `array_arg` after clear is 0.

.. js:function:: capacity(array_arg)

    capacity will return current capacity of array `array_arg`. Capacity is the count of elements, allocating (or pushing) until that size won't cause reallocating dynamic heap.

(see :ref:`Arrays <arrays>`).

^^^^^^^^
Tables
^^^^^^^^

.. js:function:: clear(table_arg)

    clear will clear whole table `table_arg`. The size of `table_arg` after clear is 0.

.. js:function:: capacity(table_arg)

    capacity will return current capacity of table `table_arg`. Capacity is the count of elements, allocating (or pushing) until that size won't cause reallocating dynamic heap.

.. js:function:: erase(table_arg, at)

    erase will erase `at` key element in `table_arg` table.

.. js:function:: length(table_arg)

    length will return current size of table `table_arg`.

.. js:function:: key_exists(table_arg, key)

    will return true if element `key` exists in table `table_arg`.

.. js:function:: find(table_arg, key, block_arg)

    will execute `block_arg` with argument pointer-to-value in `table_arg` pointing to value indexed by `key`, or null if `key` doesn't exist in `table_arg`.


(see :ref:`Tables <tables>`).

^^^^^^^^^
Functions
^^^^^^^^^

.. js:function:: invoke(block_or_function, arguments)

    invoke will call block or pointer to function (`block_or_function`) with provided list of arguments


(see :ref:`Functions <functions>`).
