#ifndef __ANALYZE_H__
#define __ANALYZE_H__

#include "ltype.h"

//--------------------------------------------------
// functions
//--------------------------------------------------

void get_summary( FunctionBlock* fb, Summary* smr );

void reset_stack_frames( FunctionBlock* fb );

void get_instruction_detail( Instruction* in, InstructionDetail* ind );
void make_instruction( Instruction* in, InstructionDetail* ind );

#endif
