#ifndef __OPTIMIZE_H__

#include "ltype.h"

#define HINT_CONSTANT_FOLDING       0x00000001
#define HINT_CONSTANT_PROPAGATION   0x00000002

//--------------------------------------------------
// functions
//--------------------------------------------------

void flow_analysis( FunctionBlock* fb, OptArg* oa );

void optimize( FunctionBlock* fb, OptArg* oa );

#endif
