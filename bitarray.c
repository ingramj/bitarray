#include "ruby.h"
#include <limits.h>
#include <string.h>

/* Bits are stored in an array of unsigned ints. We'll use a couple of defines
 * for getting the size of unsigned ints in bytes and bits.
 */
#define UINT_BYTES (sizeof(unsigned int))
#define UINT_BITS (UINT_BYTES * CHAR_BIT)

/* bitmask macro for accessing a particular bit inside an array element. */
#define bitmask(bit) (1 << (bit % UINT_BITS))

/* Bit Array structure. */
struct bit_array {
    size_t bits;         /* Number of bits. */
    size_t array_size;   /* Size of the storage array. */
    unsigned int *array; /* Array of unsigned ints, used for bit storage. */
};


/* Bit Array manipulation functions. */

/* Set the specified bit to 1. Return 1 on success, 0 on failure. */
static int
set_bit(struct bit_array *ba, size_t bit)
{
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] |= bitmask(bit);
    return 1;
}


/* Set all bits to 1. */
static int
set_all_bits(struct bit_array *ba)
{
    memset(ba->array, 0xff, (ba->array_size * UINT_BYTES));
    return 1;
}


/* Clear the specified bit to 0. Return 1 on success, 0 on failure. */
static int
clear_bit(struct bit_array *ba, size_t bit)
{
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] &= ~bitmask(bit);
    return 1;
}


/* Clear all bits to 0. */
static int
clear_all_bits(struct bit_array *ba)
{
    memset(ba->array, 0x00, (ba->array_size * UINT_BYTES));
    return 1;
}


/* Toggle the state of the specified bit. Return 1 on success, 0 on failure. */
static int
toggle_bit(struct bit_array *ba, size_t bit)
{
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] ^= bitmask(bit);
    return 1;
}


/* Assign the specified value to a bit. Return 1 on success, 0 on failure. */
static int
assign_bit(struct bit_array *ba, size_t bit, unsigned int value)
{
    if (value == 0) {
        return clear_bit(ba, bit);
    } else if (value == 1) {
        return set_bit(ba, bit);
    } else {
        return 0;
    }
}


/* Get the state of the specified bit. Return -1 on failure. */
static int
get_bit(struct bit_array *ba, size_t bit)
{
    if (bit >= ba->bits) {
        return -1;
    }

    int b = (ba->array[bit / UINT_BITS] & bitmask(bit));
    return (b >> (bit % UINT_BITS));
}


/* Bit Array destruction */
static void
destroy_bit_array(struct bit_array *ba)
{
    if (ba && ba->array) {
        free(ba->array);
    }
    free(ba);
}


/* Ruby Interface Functions */

static VALUE rb_bitarray_class;

static VALUE
rb_bitarray_alloc(VALUE klass)
{
    struct bit_array *ba;
    return Data_Make_Struct(rb_bitarray_class, struct bit_array, NULL,
            destroy_bit_array, ba);
}


static VALUE
rb_bitarray_initialize(VALUE self, VALUE size)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t bits = NUM2SIZET(size);
    size_t array_size = (bits - 1) / UINT_BITS + 1;

    ba->bits = bits;
    ba->array_size = array_size;
    ba->array = ruby_xcalloc(array_size, UINT_BYTES);

    return self;
}


static VALUE
rb_bitarray_size(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    return SIZET2NUM(ba->bits);
}


static VALUE
rb_bitarray_set_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t index = NUM2SIZET(bit);

    if (set_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %lu out of bit array",
                (unsigned long)index);
    }
}


static VALUE
rb_bitarray_set_all_bits(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    if(set_all_bits(ba)) {
        return self;
    } else {    /* This shouldn't ever happen. */
        rb_bug("BitArray#set_all_bits failed. This should not occur.");
    }
}


static VALUE
rb_bitarray_clear_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t index = NUM2SIZET(bit);

    if (clear_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %lu out of bit array",
                (unsigned long)index);
    }
}


static VALUE
rb_bitarray_clear_all_bits(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    if(clear_all_bits(ba)) {
        return self;
    } else {    /* This shouldn't ever happen. */
        rb_bug("BitArray#clear_all_bits failed. This should not occur.");
    }
}


static VALUE
rb_bitarray_get_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t index = NUM2SIZET(bit);
    int bit_value = get_bit(ba, index);

    if (bit_value != -1) {
        return INT2NUM(bit_value);
    } else {
        rb_raise(rb_eIndexError, "index %lu out of bit array",
                (unsigned long)index);
    }
}


static VALUE
rb_bitarray_inspect(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    VALUE str;
    size_t i;

    str = rb_str_new2("[");
    for (i = 0; i < ba->bits; i++) {
        if (i < (ba->bits - 1)) {
            rb_str_catf(str, "%u, ", get_bit(ba, i));
        } else {
            rb_str_catf(str, "%d", get_bit(ba, i));
        }
    }
    rb_str_cat2(str, "]");

    return str;
}


static VALUE
rb_bitarray_each(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t i;

    /* TODO: This was taken from array.c Figure out how it works. */
    RETURN_ENUMERATOR(self, 0, 0);
    for (i = 0; i < ba->bits; i++) {
        int bit_value = get_bit(ba, i);
        rb_yield(INT2NUM(bit_value));
    }
    return self;
}


void
Init_bitarray()
{
    rb_bitarray_class = rb_define_class("BitArray", rb_cObject);
    rb_define_alloc_func(rb_bitarray_class, rb_bitarray_alloc);

    rb_define_method(rb_bitarray_class, "initialize",
            rb_bitarray_initialize, 1);
    rb_define_method(rb_bitarray_class, "size", rb_bitarray_size, 0);
    rb_define_alias(rb_bitarray_class, "length", "size");
    rb_define_method(rb_bitarray_class, "set_bit", rb_bitarray_set_bit, 1);
    rb_define_method(rb_bitarray_class, "set_all_bits",
            rb_bitarray_set_all_bits, 0);
    rb_define_method(rb_bitarray_class, "clear_bit", rb_bitarray_clear_bit, 1);
    rb_define_method(rb_bitarray_class, "clear_all_bits",
            rb_bitarray_clear_all_bits, 0);
    rb_define_method(rb_bitarray_class, "[]", rb_bitarray_get_bit, 1);
    rb_define_method(rb_bitarray_class, "inspect", rb_bitarray_inspect, 0);
    rb_define_method(rb_bitarray_class, "each", rb_bitarray_each, 0);

    rb_include_module(rb_bitarray_class, rb_mEnumerable);
}

