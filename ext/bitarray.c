#include "ruby.h"
#include <limits.h>
#include <string.h>

#define UINT_BYTES (sizeof(unsigned int))
#define UINT_BITS (UINT_BYTES * CHAR_BIT)

#define bitmask(bit) (1 << ((bit) % UINT_BITS))
#define uint_array_size(bits) (((bits) - 1) / UINT_BITS + 1)
#define bitarray_size(ba) (ba->bits)

struct bitarray {
    long bits;           /* Number of bits. */
    long array_size;     /* Size of the storage array. */
    unsigned int *array; /* Array of unsigned ints, used for bit storage. */
};


/* Low-level bit-manipulation functions.
 * 
 * These function are used by the Ruby interface functions to modify
 * bitarray structures.
 * 
 * Functions that take an index will raise an IndexError if it is out of range.
 * Negative indices count from the end of the array.
 */


/* This function is used by all of the bit-manipulation functions to check
 * indices. It handles negative-index conversion and bounds checking.
 */
static inline long
check_index(struct bitarray *ba, long index)
{
    if (index < 0) index += ba->bits;
    if (index >= ba->bits) {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    }

    return index;
}


/* Set the specified bit to 1. */
static inline void
set_bit(struct bitarray *ba, long index)
{
    index = check_index(ba, index);
    ba->array[index / UINT_BITS] |= bitmask(index);
}


/* Set all bits to 1. */
static inline void
set_all_bits(struct bitarray *ba)
{
    memset(ba->array, 0xff, (ba->array_size * UINT_BYTES));
}


/* Clear the specified bit to 0. */
static inline void
clear_bit(struct bitarray *ba, long index)
{
    index = check_index(ba, index);
    ba->array[index / UINT_BITS] &= ~bitmask(index);
}


/* Clear all bits to 0. */
static inline void
clear_all_bits(struct bitarray *ba)
{
    memset(ba->array, 0x00, (ba->array_size * UINT_BYTES));
}


/* Toggle the state of the specified bit. */
static inline void
toggle_bit(struct bitarray *ba, long index)
{
    index = check_index(ba, index);
    ba->array[index / UINT_BITS] ^= bitmask(index);
}


/* Toggle the state of all bits. */
static inline void 
toggle_all_bits(struct bitarray *ba)
{
    long i;
    for(i = 0; i < ba->array_size; i++) {
        ba->array[i] ^= ~0ul;     /* ~0 = all bits set. */
    }
}


/* Assign the specified value to a bit. If the specified value is invalid,
 * raises an ArgumentError.
 */ 
static inline void
assign_bit(struct bitarray *ba, long index, int value)
{
    if (value == 0) {
        clear_bit(ba, index);
    } else if (value == 1) {
        set_bit(ba, index);
    } else {
        rb_raise(rb_eArgError, "bit value %d out of range", value);
    }
}


/* Get the state of the specified bit. */
static inline int
get_bit(struct bitarray *ba, long index)
{
    index = check_index(ba, index);

    /* We could shift the bit down, but this is easier. We need an unsigned int
     * to prevent overflow.
     */
    unsigned int b = (ba->array[index / UINT_BITS] & bitmask(index));
    if (b > 0) {
        return 1;
    } else {
        return 0;
    }
}


/* Return the number of set bits in the array. */
static inline long
total_set(struct bitarray *ba)
{
    /* This is basically the algorithm from K&R, with a running total for all
     * array elements. There are faster algorithms, but this one is simpler to
     * implement. The running time is proportionate to the number of set bits.
     */
    long count = 0;
    long i;
    unsigned x;
    for (i = 0; i < ba->array_size; i++) {
        x = ba->array[i];
        while (x) {
            x &= x - 1;
            count++;
        }
    }
    return count;
}


/* Initialize an already-allocated bitarray structure. The array is initialized
 * to all zeros.
 */
static inline void
initialize_bitarray(struct bitarray *ba, long size)
{
    if (size < 0) {
        ba->bits = 0;
        ba->array_size = 0;
        ba->array = NULL;
        return;
    }

    ba->bits = size;
    ba->array_size = uint_array_size(size);
    ba->array = ruby_xcalloc(ba->array_size, UINT_BYTES);
}


/* Initialize an already-allocated bitarray structure as a copy of another
 * bitarray structure.
 */
static inline void
initialize_bitarray_copy(struct bitarray *new_ba, struct bitarray *orig_ba)
{
    new_ba->bits = orig_ba->bits;
    new_ba->array_size = orig_ba->array_size;
    new_ba->array = ruby_xmalloc(new_ba->array_size * UINT_BYTES);

    memcpy(new_ba->array, orig_ba->array, new_ba->array_size * UINT_BYTES);
}


/* Initialize an already-allocated bitarray structure as the concatenation of
 * two other bitarrays structures.
 */
static void
initialize_bitarray_concat(struct bitarray *new_ba, struct bitarray *x_ba,
        struct bitarray *y_ba)
{
    new_ba->bits = x_ba->bits + y_ba->bits;
    new_ba->array_size = uint_array_size(new_ba->bits);
    new_ba->array = ruby_xmalloc(new_ba->array_size * UINT_BYTES);


    /* For each bit set in x_ba and y_ba, set the corresponding bit in new_ba.
     *
     * First, copy x_ba->array to the beginning of new_ba->array.
     */
    memcpy(new_ba->array, x_ba->array, x_ba->array_size * UINT_BYTES);

    /* Then, if x_ba->bits is a multiple of UINT_BITS, we can just copy
     * y_ba->array onto the end of new_ba->array.
     * 
     * Otherwise, we need to go through y_ba->array bit-by-bit and set the
     * appropriate bits in new_ba->array.
     */
    if ((x_ba->bits % UINT_BITS) == 0) {
        unsigned int *start = new_ba->array + x_ba->array_size;
        memcpy(start, y_ba->array, y_ba->array_size * UINT_BYTES);
    } else {
        long y_index, new_index;
        for (y_index = 0, new_index = x_ba->bits;
                y_index < y_ba->bits;
                y_index++, new_index++)
        {
            if (get_bit(y_ba, y_index) == 1) {
                set_bit(new_ba, new_index);
            } else {
                clear_bit(new_ba, new_index);
            }
        }
    }

}



/* Ruby Interface Functions.
 * 
 * These functions put a Ruby face on top of the lower-level functions. With
 * very few exceptions, they should not access the bitarray structs' members
 * directly.
 *
 * "bitarray" refers to the C structure. "BitArray" refers to the Ruby class.
 *
 * All publicly-accessible functions (those with an rb_define_method call in
 * Init_bitarray) should have RDoc comment headers.
 *
 * When in doubt, see how array.c does things.
 */


/* Our BitArray class. This is initialized in Init_bitarray, but we declare it
 * here because a few functions need it.
 */
static VALUE rb_bitarray_class;


/* This gets called when a BitArray is garbage collected. It frees the memory
 * used by the bitarray struct.
 */
static void
rb_bitarray_free(struct bitarray *ba)
{
    if (ba && ba->array) {
        ruby_xfree(ba->array);
    }
    ruby_xfree(ba);
}


/* This function is called by BitArray.new to allocate a new BitArray.
 * Initialization is done in a seperate function.
 * 
 * This function can be called directly with BitArray.allocate, but that is not
 * very useful.
 */
static VALUE
rb_bitarray_alloc(VALUE klass)
{
    struct bitarray *ba;
    return Data_Make_Struct(rb_bitarray_class, struct bitarray, NULL,
            rb_bitarray_free, ba);
}


/* Initialization helper-function prototypes. These functions are defined after
 * rb_bitarray_initialize.
 */
static VALUE rb_bitarray_from_string(VALUE self, VALUE string);
static VALUE rb_bitarray_from_array(VALUE self, VALUE array);


/* call-seq:
 *      BitArray.new(size)
 *      BitArray.new(string)
 *      BitArray.new(array)
 *
 * When called with a size, creates a new BitArray of the specified size, with
 * all bits cleared. When called with a string or an array, creates a new
 * BitArray from the argument.
 *
 * If a string is given, it should consist of ones and zeroes. If there are
 * any other characters in the string, the first invalid character and all
 * following characters will be ignored.
 *
 *   b = BitArray.new("10101010")       => 10101010
 *   b = BitArray.new("1010abcd")       => 1010
 *   b = BitArray.new("abcd")           => 
 *
 * If an array is given, the BitArray is initialized from its elements using
 * the following rules:
 *
 * 1. 0, false, or nil  => 0
 * 2. anything else     => 1
 *
 * Note that the 0 is a number, not a string. "Anything else" means strings,
 * symbols, non-zero numbers, subarrays, etc.
 *
 *   b = BitArray.new([0,0,0,1,1,0])            => 000110
 *   b = BitArray.new([false, true, false])     => 010
 *   b = BitArray.new([:a, :b, :c, [:d, :e]])   => 1111
 */
static VALUE
rb_bitarray_initialize(VALUE self, VALUE arg)
{
    if (TYPE(arg) == T_FIXNUM || TYPE(arg) == T_BIGNUM) {
        struct bitarray *ba;
        Data_Get_Struct(self, struct bitarray, ba);

        long size = NUM2LONG(arg);
        initialize_bitarray(ba, size);
    
        return self;

    } else if (TYPE(arg) == T_STRING) {
        return rb_bitarray_from_string(self, arg);
    } else if (TYPE(arg) == T_ARRAY) { 
        return rb_bitarray_from_array(self, arg);
    } else {
        rb_raise(rb_eArgError, "must be size, string, or array");
    }
}


/* Create a new BitArray from a string. Called by rb_bitarray_initialize. */
static VALUE
rb_bitarray_from_string(VALUE self, VALUE string)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    /* Extract a C-string from arg. */
    long str_len = RSTRING_LEN(string) + 1;
    char cstr[str_len];
    strncpy(cstr, StringValueCStr(string), str_len);

    /* If the string doesn't begin with a '1' or '0', return an empty
     * BitArray.
     */
    if (cstr[0] != '0' && cstr[0] != '1') {
        initialize_bitarray(ba, 0);
        return self;
    }

    /* Otherwise, loop through the string and truncate it at the first invalid
     * character.
     */
    long i;
    for (i = 0; i < str_len; i++) {
        if (cstr[i] != '0' && cstr[i] != '1') {
            cstr[i] = '\0';
            break;
        }
    }

    /* Setup the BitArray structure. */
    initialize_bitarray(ba, strlen(cstr));

    /* Initialize the bit array with the string. */
    for (i = 0; i < ba->bits; i++) {
        if (cstr[i] == '0') {
            clear_bit(ba, i);
        } else {
            set_bit(ba, i);
        }
    }

    return self;
}


/* Create a new BitArray from an Array. Called by rb_bitarray_initialize */
static VALUE
rb_bitarray_from_array(VALUE self, VALUE array)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    long size = RARRAY_LEN(array);
    initialize_bitarray(ba, size);

    VALUE e;
    long i;
    for (i = 0; i < size; i++) {
        e = rb_ary_entry(array, i);

        switch (TYPE(e)) {
            case T_FIXNUM:      /* fixnums and bignums treated the same. */
            case T_BIGNUM:
                NUM2LONG(e) == 0l ? clear_bit(ba, i) : set_bit(ba, i);
                break;
            case T_FALSE:       /* false and nil treated the same. */
            case T_NIL:
                clear_bit(ba, i);
                break;
            default:
                set_bit(ba, i);
        }
    }

    return self;
}


/* call-seq:
 *      bitarray.clone          -> a_bitarray
 *      bitarray.dup            -> a_bitarray
 *
 * Produces a copy of _bitarray_.
 */
static VALUE
rb_bitarray_initialize_copy(VALUE self, VALUE orig)
{
    struct bitarray *new_ba, *orig_ba;
    Data_Get_Struct(self, struct bitarray, new_ba);
    Data_Get_Struct(orig, struct bitarray, orig_ba);

    initialize_bitarray_copy(new_ba, orig_ba);

    return self;
}


/* call-seq:
 *      bitarray + other_bitarray       -> a_bitarray
 *
 * Concatenation---Return a new BitArray built by concatenating the two
 * BitArrays.
 */
static VALUE
rb_bitarray_concat(VALUE x, VALUE y)
{
    /* Get the bitarrays from x and y */
    struct bitarray *x_ba, *y_ba;
    Data_Get_Struct(x, struct bitarray, x_ba);
    Data_Get_Struct(y, struct bitarray, y_ba);

    /* Create a new BitArray, and its bitarray structure*/
    VALUE z = rb_bitarray_alloc(rb_bitarray_class);
    struct bitarray *z_ba;
    Data_Get_Struct(z, struct bitarray, z_ba);

    initialize_bitarray_concat(z_ba, x_ba, y_ba);

    return z;
}


/* call-seq:
 *      bitarray.size           -> int
 *      bitarray.length         -> int
 *
 * Returns the number of bits in _bitarray_.
 */
static VALUE
rb_bitarray_size(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    return LONG2NUM(bitarray_size(ba));
}


/* call-seq:
 *      bitarray.total_set      -> int
 *
 * Return the number of set (1) bits in _bitarray_.
 */
static VALUE
rb_bitarray_total_set(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    return LONG2NUM(total_set(ba));
}


/* call-seq:
 *      bitarray.set_bit(index)       -> bitarray
 *
 * Sets the bit at _index_ to 1. Negative indices count backwards from the end
 * of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_set_bit(VALUE self, VALUE index)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);
    
    set_bit(ba, NUM2LONG(index));
    return self;
}


/* call-seq:
 *      bitarray.set_all_bits           -> bitarray
 *
 * Sets all bits to 1.
 */
static VALUE
rb_bitarray_set_all_bits(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    set_all_bits(ba);
    return self;
}


/* call-seq:
 *      bitarray.clear_bit(index)       -> bitarray
 *
 * Sets the bit at _index_ to 0. Negative indices count backwards from the end
 * of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_clear_bit(VALUE self, VALUE index)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    clear_bit(ba, NUM2LONG(index));
    return self;
}


/* call-seq:
 *      bitarray.clear_all_bits         -> bitarray
 *
 * Sets all bits to 0.
 */
static VALUE
rb_bitarray_clear_all_bits(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    clear_all_bits(ba);
    return self;
}


/* call-seq:
 *      bitarray.toggle_bit(index)       -> bitarray
 *
 * Toggles the bit at _index_ to 0. Negative indices count backwards from the
 * end of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_toggle_bit(VALUE self, VALUE index)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    toggle_bit(ba, NUM2LONG(index));
    return self;
}


/* call-seq:
 *      bitarray.toggle_all_bits         -> bitarray
 *
 * Toggle all bits.
 */
static VALUE
rb_bitarray_toggle_all_bits(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    toggle_all_bits(ba);
    return self;
}


/* Bit-reference helper-function prototypes. These are defined after
 * rb_bitarray_bitref.
 */
static inline VALUE rb_bitarray_get_bit(VALUE self, long index);
static VALUE rb_bitarray_subseq(VALUE self, long beg, long len);


/* call-seq:
 *      bitarray[index]         -> value
 *      bitarray[beg, len]      -> a_bitarray
 *      bitarray[range]         -> a_bitarray
 *
 * Bit Reference---Returns the bit at _index_, or returns a subarray starting
 * at _beg_, and continuing for _len_ bits, or returns a subarray specified by
 * _range_.  _Negative indices count backwards from the end of _bitarray_. If
 * _index_ is greater than the capacity of _bitarray_, an +IndexError+ is
 * raised.
 */
static VALUE
rb_bitarray_bitref(int argc, VALUE *argv, VALUE self)
{
    /* We follow a form similar to rb_ary_aref in array.c */

    /* Two arguments means we have a beginning and a  length */
    if (argc == 2) {
        long beg = NUM2LONG(argv[0]);
        long len = NUM2LONG(argv[1]);
        return rb_bitarray_subseq(self, beg, len);
    } 
    
    /* Make sure we have either 1 or 2 arguments. */
    if (argc != 1) {
        rb_scan_args(argc, argv, "11", 0, 0);
    }

    /* If we have a single argument, it can be either an index, or a range. */
    VALUE arg = argv[0];
    
    /* rb_ary_aref treats a fixnum argument specially, for a speedup in the
     * most common case. We'll do the same.
     */
    if (FIXNUM_P(arg)) {
        return rb_bitarray_get_bit(self, FIX2LONG(arg));
    }

    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);
    /* Next we see if arg is a range. rb_range_beg_len is defined in range.c
     * If arg is not a range, it returns Qfalse. If arg is a range, but it
     * refers to invalid indices, it returns Qnil. Otherwise, it sets beg and
     * end to the appropriate values.
     */
    long beg, len;
    switch (rb_range_beg_len(arg, &beg, &len, ba->bits, 0)) {
        case Qfalse:
            break;
        case Qnil:
            return Qnil;
        default:
            return rb_bitarray_subseq(self, beg, len);
    }
    
    return rb_bitarray_get_bit(self, NUM2LONG(arg));
}


/* Return an individual bit. */
static inline VALUE
rb_bitarray_get_bit(VALUE self, long index)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    return INT2NUM(get_bit(ba, index));
}


/* Create a new BitArray from a subsequence of x. */
static VALUE
rb_bitarray_subseq(VALUE x, long beg, long len)
{

    struct bitarray *x_ba;
    Data_Get_Struct(x, struct bitarray, x_ba);

    /* Quick exit - a negative length, or a beginning past the end of the
     * array returns nil.
     */
    if (beg < 0) {
        beg += bitarray_size(x_ba);
    }
    if (len < 0 || beg > bitarray_size(x_ba)) {
        return Qnil;
    }

    /* Make sure that we don't try getting more bits than x has. We handle this
     * the same way as Array; if beg+len is past the end of x, shorten len.
     */
    if (bitarray_size(x_ba) <  len ||  bitarray_size(x_ba) < (beg + len)) {
        len = bitarray_size(x_ba) - beg;
    }

    /* Create a new BitArray of the appropriate size. */
    VALUE y = rb_bitarray_alloc(rb_bitarray_class);
    rb_bitarray_initialize(y, LONG2NUM(len));
    /* If our length is 0, we can just return now. */
    if (len == 0) {
        return y;
    }
    struct bitarray *y_ba;
    Data_Get_Struct(y, struct bitarray, y_ba);

    /* For each set bit in x[beg..len], set the corresponding bit in y. */
    long x_index, y_index;
    for (x_index = beg, y_index = 0;
            x_index < beg + len;
            x_index++, y_index++)
    {
        if (get_bit(x_ba, x_index) == 1) {
            set_bit(y_ba, y_index);
        }
    }

    return y;
}


/* call-seq:
 *      bitarray[index] = value     -> value
 *
 * Bit Assignment---Sets the bit at _index_. _value_ must be 0 or 1. Negative
 * indices are allowed, and will count backwards from the end of _bitarray_.
 *
 * If _index_ is greater than the capacity of _bitarray_, an +IndexError+ is
 * raised. If _value_ is something other than 0 or 1, a +RuntimeError+ is
 * raised.
 */
static VALUE
rb_bitarray_assign_bit(VALUE self, VALUE index, VALUE value)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    assign_bit(ba, NUM2LONG(index), NUM2INT(value));
    return value; 
}


/* call-seq:
 *      bitarray.inspect        -> string
 *      bitarray.to_s           -> string
 *
 * Create a printable version of _bitarray_.
 */
static VALUE
rb_bitarray_inspect(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    long cstr_size = bitarray_size(ba) + 1;
    char cstr[cstr_size];
    
    long i;
    for (i = 0; i < bitarray_size(ba); i++) {
        cstr[i] = get_bit(ba, i) + '0';
    }
    cstr[cstr_size - 1] = '\0';

    VALUE str = rb_str_new2(cstr);
    return str;
}


/* call-seq:
 *      bitarray.to_a           -> an_array
 *
 * Creates a Ruby Array from bitarray.
 */
static VALUE
rb_bitarray_to_a(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    long array_size = bitarray_size(ba);
    VALUE c_array[array_size];
    
    int i;
    for (i = 0; i < array_size; i++) {
        c_array[i] = INT2FIX(get_bit(ba, i));
    }

    return rb_ary_new4(array_size, c_array);
}


/* call-seq:
 *      bitarray.each {|bit| block }        -> bitarray
 *
 * Calls +block+ once for each bit in _bitarray_, passing that bit as a
 * parameter.
 *
 *      ba = BitArray.new(10)
 *      ba.each {|bit| print bit, " " }
 *
 * produces:
 *
 *      0 0 0 0 0 0 0 0 0 0
 */
static VALUE
rb_bitarray_each(VALUE self)
{
    struct bitarray *ba;
    Data_Get_Struct(self, struct bitarray, ba);

    long i;

    RETURN_ENUMERATOR(self, 0, 0);
    for (i = 0; i < bitarray_size(ba); i++) {
        int bit_value = get_bit(ba, i);
        rb_yield(INT2NUM(bit_value));
    }
    return self;
}


/* Document-class: BitArray
 *
 * An array of bits. Usage is similar to the standard Array class, but the only
 * allowed elements are 1 and 0. BitArrays are not resizable.
 */
void
Init_bitarray()
{
    rb_bitarray_class = rb_define_class("BitArray", rb_cObject);
    rb_define_alloc_func(rb_bitarray_class, rb_bitarray_alloc);
    rb_define_method(rb_bitarray_class, "initialize",
            rb_bitarray_initialize, 1);
    rb_define_method(rb_bitarray_class, "initialize_copy",
            rb_bitarray_initialize_copy, 1);
    rb_define_method(rb_bitarray_class, "+", rb_bitarray_concat, 1);
    rb_define_method(rb_bitarray_class, "size", rb_bitarray_size, 0);
    rb_define_alias(rb_bitarray_class, "length", "size");
    rb_define_method(rb_bitarray_class, "total_set", rb_bitarray_total_set, 0);
    rb_define_method(rb_bitarray_class, "set_bit", rb_bitarray_set_bit, 1);
    rb_define_method(rb_bitarray_class, "set_all_bits",
            rb_bitarray_set_all_bits, 0);
    rb_define_method(rb_bitarray_class, "clear_bit", rb_bitarray_clear_bit, 1);
    rb_define_method(rb_bitarray_class, "clear_all_bits",
            rb_bitarray_clear_all_bits, 0);
    rb_define_method(rb_bitarray_class, "toggle_bit",
            rb_bitarray_toggle_bit, 1);
    rb_define_method(rb_bitarray_class, "toggle_all_bits",
            rb_bitarray_toggle_all_bits, 0);
    rb_define_method(rb_bitarray_class, "[]", rb_bitarray_bitref, -1);
    rb_define_alias(rb_bitarray_class, "slice", "[]");
    rb_define_method(rb_bitarray_class, "[]=", rb_bitarray_assign_bit, 2);
    rb_define_method(rb_bitarray_class, "to_a", rb_bitarray_to_a, 0);
    rb_define_method(rb_bitarray_class, "inspect", rb_bitarray_inspect, 0);
    rb_define_alias(rb_bitarray_class, "to_s", "inspect");
    rb_define_method(rb_bitarray_class, "each", rb_bitarray_each, 0);

    rb_include_module(rb_bitarray_class, rb_mEnumerable);
}

