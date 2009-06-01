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
    long bits;         /* Number of bits. */
    long array_size;   /* Size of the storage array. */
    unsigned int *array; /* Array of unsigned ints, used for bit storage. */
};


/* Bit Array manipulation functions. */

/* Set the specified bit to 1. Return 1 on success, 0 on failure. */
static inline int
set_bit(struct bit_array *ba, long bit)
{
    if (bit < 0) bit += ba->bits;
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] |= bitmask(bit);
    return 1;
}


/* Set all bits to 1. */
static inline int
set_all_bits(struct bit_array *ba)
{
    memset(ba->array, 0xff, (ba->array_size * UINT_BYTES));
    return 1;
}


/* Clear the specified bit to 0. Return 1 on success, 0 on failure. */
static inline int
clear_bit(struct bit_array *ba, long bit)
{
    if (bit < 0) bit += ba->bits;
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] &= ~bitmask(bit);
    return 1;
}


/* Clear all bits to 0. */
static inline int
clear_all_bits(struct bit_array *ba)
{
    memset(ba->array, 0x00, (ba->array_size * UINT_BYTES));
    return 1;
}


/* Toggle the state of the specified bit. Return 1 on success, 0 on failure. */
static inline int
toggle_bit(struct bit_array *ba, long bit)
{
    if (bit < 0) bit += ba->bits;
    if (bit >= ba->bits) {
        return 0;
    }

    ba->array[bit / UINT_BITS] ^= bitmask(bit);
    return 1;
}


/* Toggle the state of all bits. */
static inline int
toggle_all_bits(struct bit_array *ba)
{
    long i;
    for(i = 0; i < ba->array_size; i++) {
        ba->array[i] ^= ~0ul;     /* ~0 = all bits set. */
    }
    return 1;
}


/* Assign the specified value to a bit. Return 1 on success, 0 on invalid bit
 * index, and -1 on invalid value. */
static inline int
assign_bit(struct bit_array *ba, long bit, int value)
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
static inline int
get_bit(struct bit_array *ba, long bit)
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


/* Return the number of set bits in the array. */
static inline long
total_set(struct bit_array *ba)
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


/* Ruby Interface Functions. */

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


/* Create a new BitArray from a string. Called by rb_bitarray_initialize. */
static VALUE
rb_bitarray_from_string(VALUE self, VALUE arg)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    /* Extract a C-string from arg. */
    long str_len = RSTRING_LEN(arg) + 1;
    char cstr[str_len];
    strncpy(cstr, StringValueCStr(arg), str_len);

    /* If the string doesn't begin with a '1' or '0', return an empty
     * BitArray.
     */
    if (cstr[0] != '0' && cstr[0] != '1') {
        ba->bits = 0;
        ba->array_size = 0;
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
    ba->bits = strlen(cstr);
    ba->array_size = ((ba->bits - 1) / UINT_BITS) + 1;
    ba->array = ruby_xmalloc(ba->array_size * UINT_BYTES);

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
rb_bitarray_from_array(VALUE self, VALUE arg)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    ba->bits = RARRAY_LEN(arg);
    ba->array_size = ((ba->bits - 1) / UINT_BITS) + 1;
    ba->array = ruby_xmalloc(ba->array_size * UINT_BYTES);

    VALUE e;
    long i;
    for (i = 0; i < ba->bits; i++) {
        e = rb_ary_entry(arg, i);

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
        struct bit_array *ba;
        Data_Get_Struct(self, struct bit_array, ba);

        long bits = NUM2LONG(arg);
        if (bits <= 0) {
            ba->bits = 0;
            ba->array_size = 0;
            return self;
        }
        
        ba->bits = bits;
        ba->array_size = ((bits - 1) / UINT_BITS) + 1;
        ba->array = ruby_xcalloc(ba->array_size, UINT_BYTES);

        return self;

    } else if (TYPE(arg) == T_STRING) {
        return rb_bitarray_from_string(self, arg);
    } else if (TYPE(arg) == T_ARRAY) { 
        return rb_bitarray_from_array(self, arg);
    } else {
        rb_raise(rb_eArgError, "must be size, string, or array");
    }
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
    struct bit_array *new_ba, *orig_ba;
    Data_Get_Struct(self, struct bit_array, new_ba);
    Data_Get_Struct(orig, struct bit_array, orig_ba);

    new_ba->bits = orig_ba->bits;
    new_ba->array_size = orig_ba->array_size;
    new_ba->array = ruby_xcalloc(new_ba->array_size, UINT_BYTES);

    memcpy(new_ba->array, orig_ba->array, (new_ba->array_size * UINT_BYTES));

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
    /* Get the bit_arrays from x and y */
    struct bit_array *x_ba, *y_ba;
    Data_Get_Struct(x, struct bit_array, x_ba);
    Data_Get_Struct(y, struct bit_array, y_ba);

    /* Create a new BitArray, and its bit_array */
    VALUE z;
    struct bit_array *z_ba;
    z = rb_bitarray_alloc(rb_bitarray_class);
    rb_bitarray_initialize(z, LONG2NUM(x_ba->bits + y_ba->bits));
    Data_Get_Struct(z, struct bit_array, z_ba);

    /* For each bit set in x and y, set the corresponding bit in z. First, copy
     * x to the beginning of z. Then, if x->bits is a multiple of UINT_BITS, we
     * can just copy y onto the end of z. Otherwise, we need to go through y
     * bit-by-bit and set the appropriate bits in z.
     */
    memcpy(z_ba->array, x_ba->array, (x_ba->array_size * UINT_BYTES));
    if ((x_ba->bits % UINT_BITS) == 0) {
        unsigned int *start = z_ba->array + x_ba->array_size;
        memcpy(start, y_ba->array, (y_ba->array_size * UINT_BYTES));
    } else {
        long y_index, z_index;
        for (y_index = 0, z_index = x_ba->bits;
                y_index < y_ba->bits;
                y_index++, z_index++)
        {
            if (get_bit(y_ba, y_index) == 1) {
                set_bit(z_ba, z_index);
            } else {
                clear_bit(z_ba, z_index);
            }
        }
    }
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
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    return LONG2NUM(ba->bits);
}


/* call-seq:
 *      bitarray.total_set      -> int
 *
 * Return the number of set (1) bits in _bitarray_.
 */
static VALUE
rb_bitarray_total_set(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long count = total_set(ba);
    return LONG2NUM(count);
}


/* call-seq:
 *      bitarray.set_bit(index)       -> bitarray
 *
 * Sets the bit at _index_ to 1. Negative indices count backwards from the end
 * of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_set_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long index = NUM2LONG(bit);

    if (set_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    }
}


/* call-seq:
 *      bitarray.set_all_bits           -> bitarray
 *
 * Sets all bits to 1.
 */
static VALUE
rb_bitarray_set_all_bits(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    if(set_all_bits(ba)) {
        return self;
    } else {
        rb_bug("BitArray#set_all_bits failed. This should not occur.");
    }
}


/* call-seq:
 *      bitarray.clear_bit(index)       -> bitarray
 *
 * Sets the bit at _index_ to 0. Negative indices count backwards from the end
 * of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_clear_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long index = NUM2LONG(bit);

    if (clear_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    }
}


/* call-seq:
 *      bitarray.clear_all_bits         -> bitarray
 *
 * Sets all bits to 0.
 */
static VALUE
rb_bitarray_clear_all_bits(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    if(clear_all_bits(ba)) {
        return self;
    } else {
        rb_bug("BitArray#clear_all_bits failed. This should not occur.");
    }
}


/* call-seq:
 *      bitarray.toggle_bit(index)       -> bitarray
 *
 * Toggles the bit at _index_ to 0. Negative indices count backwards from the
 * end of _bitarray_. If _index_ is greater than the capacity of _bitarray_, an
 * +IndexError+ is raised.
 */
static VALUE
rb_bitarray_toggle_bit(VALUE self, VALUE bit)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long index = NUM2LONG(bit);

    if (toggle_bit(ba, index)) {
        return self;
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    }
}


/* call-seq:
 *      bitarray.toggle_all_bits         -> bitarray
 *
 * Toggle all bits.
 */
static VALUE
rb_bitarray_toggle_all_bits(VALUE self)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    if(toggle_all_bits(ba)) {
        return self;
    } else {
        rb_bug("BitArray#clear_all_bits failed. This should not occur.");
    }
}

/* Return an individual bit. */
static VALUE
rb_bitarray_get_bit(VALUE self, long index)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    int bit_value = get_bit(ba, index);

    if (bit_value >= 0) {
        return INT2NUM(bit_value);
    } else {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    }
}


/* Create a new BitArray from a subsequence of x. */
static VALUE
rb_bitarray_subseq(VALUE x, long beg, long len)
{

    struct bit_array *x_ba;
    Data_Get_Struct(x, struct bit_array, x_ba);

    /* Quick exit - a negative length, or a beginning past the end of the
     * array returns nil.
     */
    if (beg < 0) {
        beg += x_ba->bits;
    }
    if (len < 0 || beg > x_ba->bits) {
        return Qnil;
    }

    /* Make sure that we don't try getting more bits than x has. We handle this
     * the same way as Array; if beg+len is past the end of x, shorten len.
     */
    if (x_ba->bits <  len ||  x_ba->bits < (beg + len)) {
        len = x_ba->bits - beg;
    }

    /* Create a new BitArray of the appropriate size. */
    VALUE y;
    y = rb_bitarray_alloc(rb_bitarray_class);
    rb_bitarray_initialize(y, LONG2NUM(len));
    /* If our length is 0, we can just return now. */
    if (len == 0) {
        return y;
    }
    struct bit_array *y_ba;
    Data_Get_Struct(y, struct bit_array, y_ba);

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

    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);
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
rb_bitarray_assign_bit(VALUE self, VALUE bit, VALUE value)
{
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long index = NUM2LONG(bit);
    int bit_value = NUM2INT(value);

    int result = assign_bit(ba, index, bit_value);
    if (result == 1) {
        return value;
    } else if (result == 0) {
        rb_raise(rb_eIndexError, "index %ld out of bit array", index);
    } else {
        rb_raise(rb_eRuntimeError, "bit value %d out of range", bit_value);
    }
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
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long cstr_size = ba->bits + 1;
    char cstr[cstr_size];
    
    long i;
    for (i = 0; i < ba->bits; i++) {
        cstr[i] = get_bit(ba, i) + '0';
    }
    cstr[ba->bits] = '\0';

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
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long array_size = ba->bits;
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
    struct bit_array *ba;
    Data_Get_Struct(self, struct bit_array, ba);

    long i;

    RETURN_ENUMERATOR(self, 0, 0);
    for (i = 0; i < ba->bits; i++) {
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

