#ifndef __OPTIMIZE_H__

#include "chunk_type.h"

#define HINT_CONSTANT_FOLDING 0x00000001

//--------------------------------------------------
// structs
//--------------------------------------------------

//--------------------------------------------------
// functions
//--------------------------------------------------

void flow_analysis( FunctionBlock* fb, OptArg* oa );

void optimize( FunctionBlock* fb, OptArg* oa );

#endif
