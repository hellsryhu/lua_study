#ifndef __OPTIMIZE_H__

#include "chunk_type.h"

#define HINT_CONSTANT_FOLDING 0x00000001

//--------------------------------------------------
// functions
//--------------------------------------------------

void flow_analysis( FunctionBlock* fb, OptArg* oa );

void delete_instruction( FunctionBlock* fb, int from, int to );
void insert_instruction( FunctionBlock* fb, int pos, Instruction* in );

void optimize( FunctionBlock* fb, OptArg* oa );

#endif
