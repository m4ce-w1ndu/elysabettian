//
//  debug.h
//  Elysabettian
//
//  Created by Simone Rolando on 07/07/21.
//

#ifndef debug_h
#define debug_h

#include "chunk.h"

void disassemble_chunk(Chunk* chunk, const char* name);
int disassemble_instruction(Chunk* chunk, int offset);

#endif /* debug_h */
