//
//  compiler.h
//  Elysabettian
//
//  Created by Simone Rolando on 07/07/21.
//

#ifndef compiler_h
#define compiler_h

#include "object.h"
#include "vm.h"

ObjFunction* compile(const char* source);
void mark_compiler_roots(void);

#endif /* compiler_h */
