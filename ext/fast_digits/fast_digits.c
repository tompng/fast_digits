#include <ruby.h>

static ID id_divmod, id_lt, id_mult, id_plus, id_bit_length;

static VALUE rb_fast_digits(int argc, VALUE *argv, VALUE self) {
  VALUE base = LONG2FIX(10);
  if (argc == 1) base = rb_to_int(argv[0]);
  VALUE bases = rb_ary_new();
  VALUE b = base;
  while (rb_funcall(b, id_lt, 1, self)) {
    rb_ary_push(bases, b);
    b = rb_funcall(b, id_mult, 1, b);
  }
  if (FIX2LONG(rb_funcall(self, id_bit_length, 0)) < 50 * FIX2LONG(rb_funcall(base, id_bit_length, 0))) {
    VALUE digits = rb_ary_new();
    VALUE num = self;
    while (!FIXNUM_P(num) || FIX2LONG(num) > 0) {
      VALUE divmod = rb_funcall(num, id_divmod, 1, base);
      rb_ary_push(digits, RARRAY_AREF(divmod, 1));
      num = RARRAY_AREF(divmod, 0);
    }
    return digits;
  }
  VALUE digits = rb_ary_new_from_args(1, self);
  for(int level=RARRAY_LEN(bases)-1; level>=0; level--) {
    VALUE b = RARRAY_AREF(bases, level);
    int lastindex = RARRAY_LEN(digits) - 1;
    for(int i = lastindex; i >= 0; i--) {
      VALUE n = RARRAY_AREF(digits, i);
      VALUE divmod = rb_funcall(n, id_divmod, 1, b);
      VALUE div = RARRAY_AREF(divmod, 0);
      VALUE mod = RARRAY_AREF(divmod, 1);
      if (!FIXNUM_P(n) && n != mod && n != self) rb_big_resize(n, 0);
      if (i != lastindex || div != LONG2FIX(0)) rb_ary_store(digits, 2 * i + 1,  div);
      rb_ary_store(digits, 2 * i, mod);
    }
  }
  return digits;
}

static int bit_length(int n) {
  for(int i=0; i<32; i++){
    if (n == 0) return i;
    n ^= n&(1<<i);
  }
  return 32;
}

static VALUE rb_from_digits(VALUE klass, VALUE arg) {
  VALUE digits = rb_ary_entry(arg, 0);
  VALUE base = LONG2FIX(10);
  if (RARRAY_LEN(arg) == 2) base = rb_to_int(rb_ary_entry(arg, 1));
  VALUE array = rb_ary_dup(digits);
  int size = RARRAY_LEN(array);
  int levels = bit_length(size);
  VALUE b = base;
  for (int level=0; level<levels; level++) {
    if (level) b = rb_funcall(b, id_mult, 1, b);
    int stride = 2 << level;
    int n = (size + stride - 1) / stride;
    for (int i=0; i<n; i++) {
      VALUE mod = rb_to_int(RARRAY_AREF(array, 2 * i));
      VALUE div = rb_ary_entry(array, 2 * i + 1);
      if (div == Qnil || div == LONG2FIX(0)) {
        RARRAY_ASET(array, i, mod);
      } else {
        div = rb_to_int(div);
        RARRAY_ASET(array, i, rb_funcall(rb_funcall(div, id_mult, 1, b), id_plus, 1, mod));
        if (!FIXNUM_P(mod)) rb_big_resize(mod, 0);
        if (!FIXNUM_P(div)) rb_big_resize(div, 0);
      }
    }
    if (n < size) RARRAY_ASET(array, n, Qnil);
  }
  return RARRAY_AREF(array, 0);
}

void Init_fast_digits(void){
  id_divmod = rb_intern("divmod");
  id_lt = rb_intern("<");
  id_mult = rb_intern("*");
  id_plus = rb_intern("+");
  id_bit_length = rb_intern("bit_length");

  rb_define_alias(rb_cInteger,  "original_digits", "digits");
  rb_define_method(rb_cInteger,  "digits", rb_fast_digits, -1);
  rb_define_singleton_method(rb_cInteger, "from_digits", rb_from_digits, -2);
}
