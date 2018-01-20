#include <ruby.h>
#include <ruby/intern.h>

static ID id_divmod, id_lt, id_mult, id_plus;

static VALUE rb_fast_digits(int argc, VALUE *argv, VALUE self) {
  VALUE base = LONG2FIX(10);
  if (argc == 1) base = rb_to_int(argv[0]);
  VALUE bases = rb_ary_new();
  VALUE b = base;
  while (rb_funcall(b, id_lt, 1, self)) {
    rb_ary_push(bases, b);
    b = rb_funcall(b, id_mult, 1, b);
  }
  VALUE digits = rb_ary_new_from_args(1, self);
  for(int idx=RARRAY_LEN(bases)-1; idx>=0; idx--) {
    int stride = 1 << idx;
    VALUE b = RARRAY_AREF(bases, idx);
    int pos = RARRAY_LEN(digits) - 1;
    VALUE dm = rb_funcall(RARRAY_AREF(digits, pos), id_divmod, 1, b);
    VALUE d = RARRAY_AREF(dm, 0);
    if (d != LONG2FIX(0)) rb_ary_store(digits, pos + stride,  d);
    rb_ary_store(digits, pos,  RARRAY_AREF(dm, 1));
    while(pos > 0) {
      pos -= 2 * stride;
      dm = rb_funcall(RARRAY_AREF(digits, pos), id_divmod, 1, b);
      rb_ary_store(digits, pos + stride, RARRAY_AREF(dm, 0));
      rb_ary_store(digits, pos, RARRAY_AREF(dm, 1));
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

static VALUE rb_restore_digits(int argc, VALUE *argv, VALUE self) {
  VALUE base = LONG2FIX(10);
  if (argc == 1) base = rb_to_int(argv[0]);
  VALUE array = rb_ary_dup(self);
  int size = RARRAY_LEN(array);
  int levels = bit_length(size);
  VALUE b = base;
  for (int level=0; level<levels; level++) {
    if (level) b = rb_funcall(b, id_mult, 1, b);
    int stride = 2 << level;
    int n = (size + stride - 1) / stride;
    for (int i=0; i<n; i++) {
      VALUE mod = RARRAY_AREF(array, 2 * i);
      VALUE div = rb_ary_entry(array, 2 * i + 1);
      VALUE v = div == Qnil ? mod : rb_funcall(rb_funcall(div, id_mult, 1, b), id_plus, 1, mod);
      RARRAY_ASET(array, i, v);
      if (i) RARRAY_ASET(array, 2 * i, Qnil);
      if (2 * i + 1 < size) RARRAY_ASET(array, 2 * i + 1, Qnil);
    }
  }
  return RARRAY_AREF(array, 0);
}

void Init_fast_digits(void){
  id_divmod = rb_intern("divmod");
  id_lt = rb_intern("<");
  id_mult = rb_intern("*");
  id_plus = rb_intern("+");

  rb_define_alias(rb_cInteger,  "original_digits", "digits");
  rb_define_method(rb_cInteger,  "digits", rb_fast_digits, -1);
  rb_define_method(rb_cArray, "revert_digits", rb_restore_digits, -1);
}
