//
//  object.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef object_h
#define object_h

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "table.h"

#define OBJECT_TYPE(value)          (AS_OBJECT(value)->type)

/* Type checks */
/* Initializers and class member functions */
#define IS_BOUND_FUN(value)         is_obj_type(value, OBJ_BOUND_FUN)
/* Classes */
#define IS_CLASS(value)             is_obj_type(value, OBJ_CLASS)
/* Closures */
#define IS_CLOSURE(value)           is_obj_type(value, OBJ_CLOSURE)
/* Functions */
#define IS_FUNCTION(value)          is_obj_type(value, OBJ_FUNCTION)
/* Instances of classes */
#define IS_INSTANCE(value)          is_obj_type(value, OBJ_INSTANCE)
/* Native types*/
#define IS_NATIVE(value)            is_obj_type(value, OBJ_NATIVE)
/* Strings */
#define IS_STRING(value)            is_obj_type(value, OBJ_STRING)

/* Conversions from object types to effetive values */
/* Member function of class */
#define AS_BOUND_FUN(value)         ((ObjBoundFun*)AS_OBJECT(value))
/* Classes */
#define AS_CLASS(value)             ((ObjClass*)AS_OBJECT(value))
/* Closures */
#define AS_CLOSURE(value)           ((ObjClosure*)AS_OBJECT(value))
/* Functions */
#define AS_FUNCTION(value)          ((ObjFunction*)AS_OBJECT(value))
/* Instances of classes */
#define AS_INSTANCE(value)          ((ObjInstance*)AS_OBJECT(value))
/* Native types */
#define AS_NATIVE(value)            (((ObjNative*)AS_OBJECT(value))->function)
/* Strings */
#define AS_STRING(value)            ((ObjString*)AS_OBJECT(value))
/* C Strings */
#define AS_CSTRING(value)           (((ObjString*)AS_OBJECT(value))->chars)

typedef enum ObjType {
    OBJ_BOUND_FUN, OBJ_CLASS,
    OBJ_CLOSURE, OBJ_FUNCTION,
    OBJ_INSTANCE, OBJ_NATIVE,
    OBJ_STRING, OBJ_UPVALUE
} ObjType;

typedef struct Object {
    ObjType type;
    bool is_marked;
    struct Object* next;
} Object;

typedef struct ObjFunction {
    Object object;
    int arity;
    int upvalue_count;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFun)(int arg_count, Value* args);

typedef struct ObjNative {
    Object object;
    NativeFun function;
} ObjNative;

typedef struct ObjString {
    Object object;
    int length;
    char* chars;
    uint32_t hash;
} ObjString;

typedef struct ObjUpvalue {
    Object object;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct ObjClosure {
    Object object;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalue_count;
} ObjClosure;

typedef struct ObjClass {
    Object object;
    ObjString* name;
    Table member_functions;
} ObjClass;

typedef struct ObjInstance {
    Object object;
    ObjClass* class;
    Table fields;
} ObjInstance;

typedef struct ObjBoundFun {
    Object object;
    Value receiver;
    ObjClosure* function;
} ObjBoundFun;

ObjClass* new_class(ObjString* name);
ObjBoundFun* new_bound_fun(Value receiver, ObjClosure* function);
ObjClass* new_class(ObjString* name);
ObjClosure* new_closure(ObjFunction* function);
ObjFunction* new_function(void);
ObjInstance* new_instance(ObjClass* class);
ObjNative* new_native(NativeFun function);
ObjString* take_string(char* chars, int len);
ObjString* copy_string(const char* chars, int len);
ObjUpvalue* new_upvalue(Value* slot);

void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif /* object_h */
