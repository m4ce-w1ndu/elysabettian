//
//  value.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef value_h
#define value_h

#include <string.h>
#include <stdbool.h>

#include "common.h"

/* Object type forward declaration */
typedef struct Object Object;
/* String object */
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT            ((uint64_t)0x8000000000000000)
#define QNAN                ((uint64_t)0x7ffc000000000000)

#define TAG_NULL            1
#define TAG_FALSE           2
#define TAG_TRUE            3

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NULL(value)      ((value) == NULL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJECT(value)    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    value_to_num(value)
#define AS_OBJECT(value)    ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b)         ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL           ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL            ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NULL_VAL            ((Value)(uint64_t)(QNAN | TAG_NULL))

#define NUMBER_VAL(num)     num_to_value(num)
#define OBJECT_VAL(obj)     (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double value_to_num(Value value)
{
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value num_to_value(double num)
{
    Value val;
    memcpy(&val, &num, sizeof(double));
    return val;
}

#else

typedef enum ValueType {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJECT
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object* object;
    } as;
} Value;

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NULL(value)      ((value).type == VAL_NULL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJECT(value)    ((value.type) == VAL_OBJECT)

#define AS_OBJECT(value)    ((value).as.object)
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NULL_VAL            ((Value){VAL_NULL, {.number = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER,{.number = value}})
#define OBJECT_VAL(value)   ((Value){VAL_OBJECT,{.object = (Obj*)value}})

#endif

typedef struct ValueArray {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool values_equal(Value a, Value b);

void init_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void free_value_array(ValueArray* array);

void print_value(Value value);

#endif /* value_h */
