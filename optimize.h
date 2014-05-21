#ifndef __OPTIMIZE_H__

#include "chunk_type.h"

#define HINT_CONSTANT_FOLDING 0x00000001

#define ADD_OP 0x01
#define ARITH_OP 0x02

//--------------------------------------------------
// functions
//--------------------------------------------------

void flow_analysis( FunctionBlock* fb, OptArg* oa );

void delete_instruction( FunctionBlock* fb, CodeBlock* cb, int from, int to );
void insert_instruction( FunctionBlock* fb, CodeBlock* cb, int pos, Instruction* in, int size );

void optimize( FunctionBlock* fb, OptArg* oa );

#endif
