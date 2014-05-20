#ifndef __OPTIMIZE_H__

#include "chunk_type.h"

void flow_analysis( FunctionBlock* fb, OptArg* oa );

void optimize( FunctionBlock* fb, OptArg* oa );

void delete_instruction( FunctionBlock* f, int from, int to );

#endif
