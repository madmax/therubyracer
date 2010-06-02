#include "v8_external.h"
#include "rr.h"
#include "v8_ref.h"
using namespace v8;

namespace {
  VALUE ExternalClass;
  VALUE references;
  VALUE _Value(VALUE self) {
    HandleScope scope;
    return (VALUE)V8_Ref_Get<External>(self)->Value();
  }
  VALUE Wrap(VALUE rbclass, VALUE value) {
    HandleScope scope;
    return rr_v8_ref_create(rbclass, rr_v8_external_create(value));
  }
  VALUE Unwrap(VALUE self, VALUE value) {
    if (rb_obj_is_kind_of(value, self)) {
      return _Value(value);
    } else {
      rb_raise(rb_eArgError, "cannot unwrap %s. It is not a kind of %s", RSTRING_PTR(rb_class_name(rb_class_of(value))), RSTRING_PTR(rb_class_name(self)));
      return Qnil;
    }
  }

  void GCWeakReferenceCallback(Persistent<Value> object, void* parameter) {
    // printf("V8 GC!!!!\n");
    Local<External> external(External::Cast(*object));
    rb_hash_delete(references, rb_obj_id((VALUE)external->Value()));
    // V8::AdjustAmountOfExternalAllocatedMemory(-100000000);
  }
}

void rr_init_v8_external() {
  ExternalClass = rr_define_class("External");
  references = rb_hash_new();
  rb_define_const(ExternalClass, "OBJECTS_REFERENCED_FROM_WITHIN_V8", references);
  rr_define_singleton_method(ExternalClass, "Wrap", Wrap, 1);
  rr_define_singleton_method(ExternalClass, "Unwrap", Unwrap, 1);
  rr_define_method(ExternalClass, "Value", _Value, 0);
}

Handle<Value> rr_v8_external_create(VALUE value) {
  rb_hash_aset(references, rb_obj_id(value), value);
  Local<Value> external(External::Wrap((void *)value));
  Persistent<Value> record = Persistent<Value>::New(external);
  // V8::AdjustAmountOfExternalAllocatedMemory(100000000);
  record.MakeWeak(NULL, GCWeakReferenceCallback);
  return external;
}