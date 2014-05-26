#include "analyze.h"

extern InstructionDesc INSTRUCTION_DESC[];

void get_summary( FunctionBlock* fb, Summary* smr )
{
    smr->total_instruction_num += fb->instruction_list.size;
    int i;
    Instruction* in;
    for( i = 0, in = fb->instruction_list.value; i < fb->instruction_list.size; i++, in++ ) {
        unsigned char op = GET_OP( in->opcode );
        smr->instruction_num[op]++;
    }

    FunctionBlock *pfb;
    for( i = 0, pfb = fb->funcs; i < fb->num_func; i++, pfb++ )
        get_summary( pfb, smr );
}

void reset_stack_frames( FunctionBlock* fb )
{
    int i;
    StackFrame* pframe;
    if( fb->stack_frames ) {
        for( i = 0, pframe = fb->stack_frames; i < fb->instruction_list.size; i++, pframe++ )
            free( pframe->slots );
        free( fb->stack_frames );
    }

    // gen stack frames
    fb->stack_frames = malloc( fb->instruction_list.size*sizeof( StackFrame ) );
    memset( fb->stack_frames, 0, fb->instruction_list.size*sizeof( StackFrame ) );
    for( i = 0, pframe = fb->stack_frames; i < fb->instruction_list.size; i++, pframe++ ) {
        pframe->slots = malloc( fb->max_stack_size*sizeof( int ) );
        memset( pframe->slots, -1, fb->max_stack_size*sizeof( int ) );
    }
    Local* l;
    for( i = 0, l = fb->local_list.value; i < fb->local_list.size; i++, l++ ) {
        int j;
        int start = l->start-1;
        start = start < 0 ? 0 : start;
        for( j = l->start, pframe = &fb->stack_frames[start]; j <= l->end; j++, pframe++ )
            pframe->slots[pframe->max_local++] = i;
    }
}

void get_instruction_detail( Instruction* in, InstructionDetail* ind )
{
    ind->op = GET_OP( in->opcode );
    ind->A = GET_A( in->opcode );
    ind->desc = &INSTRUCTION_DESC[ind->op];
    switch( ind->desc->type ) {
        case iABC:
            ind->B = GET_B( in->opcode );
            ind->C = GET_C( in->opcode );
            break;
        case iABx:
            ind->Bx = GET_BX( in->opcode );
            break;
        case iAsBx:
            ind->sBx = GET_SBX( in->opcode );
            break;
    }
    ind->line_pos = in->line_pos;
}

void make_instruction( Instruction* in, InstructionDetail* ind )
{
    switch( ind->desc->type ) {
        case iABC:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ind->B << 23 ) | ( ind->C << 14 );
            break;
        case iABx:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ind->Bx << 14 );
            break;
        case iAsBx:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ( ind->sBx+0x1FFFF ) << 14 );
            break;
    }
    in->line_pos = ind->line_pos;
}

