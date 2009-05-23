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
set_bit(struct bit_array *ba, ptrdiff_t bit)
{
    if (bit < 0) bit += ba->bits;
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
clear_bit(struct bit_array *ba, ptrdiff_t bit)
{
    if (bit < 0) bit += ba->bits;
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
toggle_bit(struct bit_array *ba, ptrdiff_t bit)
{
    if (bit < 0) bit += ba->bits;
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] ^= bitmask(bit);
    return 1;
}


/* Assign the specified value to a bit. Return 1 on success, 0 on failure. */
static int
assign_bit(struct bit_array *ba, ptrdiff_t bit, int value)
{
    if (value == 0) {
        return clear_bit(ba, bit);
    } else if (value == 1) {
        return set_bit(ba, bit);
    } else {
        return -1;
    }
}


/* Get the state of the specified bit. Return -1 on failure. */
static int
get_bit(struct bit_array *ba, ptrdiff_t bit)
{
    if (bit < 0) bit += ba->bits;
    if (bit >= ba->bits) {
        return -1;
    }

    unsigned int b = (ba->array[bit / UINT_BITS] & bitmask(bit));
    if (b > 0) {
        return 1;
    } else {
        return 0;
    }
}


/* Ruby Interface Functions */

static VALUE rb_bitarray_class;


static void
rb_bitarray_free(struct bit_array *ba)
{
    if (ba && ba->array) {
        ruby_xfree(ba->array);
    }
    ruby_xfree(ba);
}


static VALUE
rb_bitarray_alloc(VALUE klass)
{
    struct bit_array *ba;
    return Data_Make_Struct(rb_bitarray_class, struct bit_array, NULL,
            rb_bitarray_free, ba);
}


static VALUE
rb_bitarray_initialize(VALUE self, VALUE size)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t bits = NUM2SIZET(size);
    size_t array_size = ((bits - 1) / UINT_BITS) + 1;

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

    ptrdiff_t index = NUM2SSIZET(bit);

    if (set_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", (long)index);
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

    ptrdiff_t index = NUM2SSIZET(bit);

    if (clear_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", (long)index);
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

    ptrdiff_t index = NUM2SSIZET(bit);

    int bit_value = get_bit(ba, index);

    if (bit_value >= 0) {
        return INT2NUM(bit_value);
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", (long)index);
    }
}


static VALUE
rb_bitarray_assign_bit(VALUE self, VALUE bit, VALUE value)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    ptrdiff_t index = NUM2SSIZET(bit);
    int bit_value = NUM2INT(value);

    int result = assign_bit(ba, index, bit_value);
    if (result == 1) {
        return self;
    } else if (result == 0) {
        rb_raise(rb_eIndexError, "index %ld out of bit array", (long)index);
    } else {
        rb_raise(rb_eRuntimeError, "bit value %d out of range", bit_value);
    }
}


static VALUE
rb_bitarray_inspect(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    size_t cstr_size = ba->bits + 1;
    char cstr[cstr_size];
    
    size_t i;
    for (i = 0; i < ba->bits; i++) {
        cstr[i] = get_bit(ba, i) + '0';
    }
    cstr[ba->bits] = '\0';

    VALUE str = rb_str_new2(cstr);
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
    rb_define_method(rb_bitarray_class, "[]=", rb_bitarray_assign_bit, 2);
    rb_define_method(rb_bitarray_class, "inspect", rb_bitarray_inspect, 0);
    rb_define_alias(rb_bitarray_class, "to_s", "inspect");
    rb_define_method(rb_bitarray_class, "each", rb_bitarray_each, 0);

    rb_include_module(rb_bitarray_class, rb_mEnumerable);
}

