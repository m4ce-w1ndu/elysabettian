//
//  vm.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef vm_h
#define vm_h

#include "object.h"
#include "table.h"
#include "value.h"

/* Max stack frames */
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct CallFrame {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct VM {
    CallFrame call_frames[STACK_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value* stack_top;
    
    Table globals;
    Table strings;
    
    ObjString* init_strings;
    ObjUpvalue* open_upvalues;
    
    int bytes_allocated;
    int next_gc;
    
    Object* objects;
    int gray_count;
    int gray_capacity;
    Object** gray_stack;
} VM;

typedef enum IResult {
    I_OK, I_COMPILE_ERROR, I_RUNTIME_ERROR
} IResult;

/* virtual machine */
extern VM vm;

/* Initialize virtual machine */
void init_vm(void);
void free_vm(void);

/* Run interpreter */
IResult interpret(const char* source);

void push(Value value);
Value pop(void);

#endif /* vm_h */
