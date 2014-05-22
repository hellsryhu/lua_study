#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "utils.h"
#include "optimize.h"

//--------------------------------------------------
// control flow analysis functions
//--------------------------------------------------

#define CODE_BLOCK_EXIT( id ) \
    { \
        cb->exit = id; \
        list_add_tail( &cb->node, &code_block_node ); \
    }

#define CODE_BLOCK_ENTRY( id ) \
    { \
        if( id < fb->instruction_list.size ) { \
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) ); \
            memset( cb, 0, sizeof( CodeBlock ) ); \
            cb->entry = id; \
        }\
        else \
            cb = 0; \
    }

int is_greater_to( void* a, void* b )
{
    Jump* ja = ( Jump* )a;
    Jump* jb = ( Jump* )b;
    return ja->to > jb->to;
}

int get_code_block( FunctionBlock* fb, int instruction_id )
{
    int i;
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( instruction_id >= cb->entry && instruction_id <= cb->exit )
            return i;
    }
    return -1;
}

void create_successors( FunctionBlock* fb, CodeBlock* cb, int first, int last, struct list_head* pos, Jump** jmps_from )
{
    int* to_blocks = ( int* )malloc( fb->num_code_block*sizeof( int ) );
    memset( to_blocks, 0, fb->num_code_block*sizeof( int ) );

    int i;
    for( i = first; i < last; i++ ) {
        int to_block = get_code_block( fb, jmps_from[i]->to );
        if( to_block >= 0 )
            to_blocks[to_block]++;
    }

    int size = 0;
    int* blk = to_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            size++;
    }

    cb->num_succ = size;
    cb->successors = malloc( size*sizeof( CodeBlock* ) );
    CodeBlock** succ = ( CodeBlock** )cb->successors;
    blk = to_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            *succ++ = fb->code_block[i];
    }
    free( to_blocks );
}
 
void create_predecessors( FunctionBlock* fb, CodeBlock* cb, int first, int last, struct list_head* pos, Jump** jmps_to )
{
    int* from_blocks = ( int* )malloc( fb->num_code_block*sizeof( int ) );
    memset( from_blocks, 0, fb->num_code_block*sizeof( int ) );

    int i;
    for( i = first; i < last; i++ ) {
        int from_block = get_code_block( fb, jmps_to[i]->from );
        if( from_block >= 0 )
            from_blocks[from_block]++;
    }

    int size = 0;
    int* blk = from_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            size++;
    }

    cb->num_pred = size;
    cb->predecessors = malloc( size*sizeof( CodeBlock* ) );
    CodeBlock** pred = ( CodeBlock** )cb->predecessors;
    blk = from_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            *pred++ = fb->code_block[i];
    }
    free( from_blocks );
}
   
void flow_analysis( FunctionBlock* fb, OptArg* oa )
{
    Jump* jmps = malloc( fb->instruction_list.size*sizeof( Jump ) );
    Jump** jmps_from = malloc( fb->instruction_list.size*sizeof( Jump* ) );
    Jump** jmps_to= malloc( fb->instruction_list.size*sizeof( Jump* ) );
    memset( jmps, 0, fb->instruction_list.size*sizeof( Jump ) );
    memset( jmps_from, 0, fb->instruction_list.size*sizeof( Jump* ) );
    memset( jmps_to, 0, fb->instruction_list.size*sizeof( Jump* ) );
    int jmps_idx = 0;

    // get jumps
    struct list_head code_block_node;
    INIT_LIST_HEAD( &code_block_node );
    CodeBlock* cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
    memset( cb, 0, sizeof( CodeBlock ) );
    int i;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        unsigned char op = in->opcode & 0x3F;
        unsigned short C = ( in->opcode >> 14 ) & 0x1FF;
        int sBx = ( in->opcode >> 14 )-0x1FFFF;
        switch( op ) {
            // end of block
            case LOADBOOL:
                if( C ) {
                    CODE_BLOCK_EXIT( i );
                    CODE_BLOCK_ENTRY( i+1 );
                    jmps[jmps_idx].from = i;
                    jmps[jmps_idx++].to = i+2;
                }
                break;
            case JMP:
                CODE_BLOCK_EXIT( i );
                CODE_BLOCK_ENTRY( i+1 );
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+sBx+1;
                break;
            case FORLOOP:
                CODE_BLOCK_EXIT( i );
                CODE_BLOCK_ENTRY( i+1 );
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+1;
                break;
            case RETURN:
            case TAILCALL:
                CODE_BLOCK_EXIT( i );
                CODE_BLOCK_ENTRY( i+1 );
                break;
            // start of block
            case FORPREP:
                if( i != 0 ) {
                    CODE_BLOCK_EXIT( i-1 );
                    CODE_BLOCK_ENTRY( i );
                    jmps[jmps_idx].from = i-1;
                    jmps[jmps_idx++].to = i;
                }
                break;
            // must with jmp
            case TFORLOOP:
            case EQ:
            case LT:
            case LE:
            case TEST:
            case TESTSET:
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+2;
                break;
            default:
                break;
        }
    }
    if( cb ) {
        cb->exit = fb->instruction_list.size-1;
        list_add_tail( &cb->node, &code_block_node );
    }

    for( i = 0; i < jmps_idx; i++ ) {
        jmps_from[i] = &jmps[i];
        jmps_to[i] = &jmps[i];
    }
    quick_sort( ( void** )jmps_to, 0, jmps_idx-1, is_greater_to );

    // complete blocks
    struct list_head* pos = code_block_node.next;
    cb = list_entry( pos, CodeBlock, node );
    i = 0;
    while( i < jmps_idx ) {
        if( jmps_to[i]->to == cb->entry )
            i++;
        else if( jmps_to[i]->to <= cb->exit ) {
            CodeBlock* ocb = cb;
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
            memset( cb, 0, sizeof( CodeBlock ) );
            cb->entry = jmps_to[i]->to;
            cb->exit = ocb->exit;

            ocb->exit = jmps_to[i]->to-1;
            list_add( &cb->node, &ocb->node );

            pos = &cb->node;

            i++;
        }
        else if( jmps_to[i]->to > cb->exit ) {
            pos = pos->next;
            if( pos != &code_block_node ) {
                cb = list_entry( pos, CodeBlock, node );
            }
            else
                break;
        }
    }

    i = 0;
    list_for_each( pos, &code_block_node ) {
        cb = list_entry( pos, CodeBlock, node );
        cb->id = ++i;
    }

    fb->num_code_block = i;
    fb->code_block = ( CodeBlock** )malloc(i*sizeof( CodeBlock* ) );
    CodeBlock** ppcb = fb->code_block;
    list_for_each( pos, &code_block_node )
        *ppcb++ = list_entry( pos, CodeBlock, node );

    // gen successor
    int j = 0;
    cb = fb->code_block[j];
    i = 0;
    int first = 0;
    while( i < jmps_idx ) {
        if( jmps_from[i]->from <= cb->exit )
            i++;
        else {
            create_successors( fb, cb, first, i, pos, jmps_from );

            first = i;

            cb = fb->code_block[++j];
        }
    }
    create_successors( fb, cb, first, i, pos, jmps_from );

    for( i = j+1; i < fb->num_code_block; i++ ) {
        cb = fb->code_block[i];
        create_successors( fb, cb, 0, -1, pos, jmps_from );
    }

    // gen predecessor
    j = 0;
    cb = fb->code_block[j];
    i = 0;
    first = 0;
    while( i < jmps_idx ) {
        if( jmps_to[i]->to <= cb->exit )
            i++;
        else {
            create_predecessors( fb, cb, first, i, pos, jmps_to );

            first = i;

            cb = fb->code_block[++j];
        }
    }
    create_predecessors( fb, cb, first, i, pos, jmps_to );

    for( i = j+1; i < fb->num_code_block; i++ ) {
        cb = fb->code_block[i];
        create_predecessors( fb, cb, 0, -1, pos, jmps_to );
    }

    free( jmps );
    free( jmps_from );
    free( jmps_to );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( i = 0; i < fb->num_func; i++ ) {
        flow_analysis( pfb, oa );
        pfb++;
    }
}

//--------------------------------------------------
// instruction edit
//--------------------------------------------------

// delete from~to, insert into from
void modify_instruction( FunctionBlock* fb, CodeBlock* cb, int from, int to, Instruction* in, int size )
{
    if( from < cb->entry || from > cb->exit ) return;
    if( to < cb->entry || to > cb->exit ) return;

    int mod_cnt = size;
    if( to >= from )
        mod_cnt += from-to-1;

    // update jmp
    int i;
    Instruction *tmp_in;
    for( i = 0, tmp_in = fb->instruction_list.value; i < fb->instruction_list.size; i++, tmp_in++ ) {
        if( i >= from && i <= to ) continue;

        InstructionDetail ind;
        get_instruction_detail( tmp_in, &ind );
        switch( ind.op ) {
            case JMP:
            case FORLOOP:
            case FORPREP:
                { 
                    int jmp_to = i+ind.sBx+1;
                    if( i < from ) {
                        if( jmp_to > to )
                            jmp_to += mod_cnt;
                        else if( jmp_to >= from )
                            jmp_to = from;
                    }
                    else if( i > to ) {
                        if( jmp_to < from )
                            jmp_to += mod_cnt;
                        else if( jmp_to <= to )
                            jmp_to = to+1;
                    }
                    ind.sBx = jmp_to-i-1;
                }
                make_instruction( tmp_in, &ind );
                break;
            default:
                break;
        }
    }

    // update instruction list
    int old_size = fb->instruction_list.size;
    fb->instruction_list.size += mod_cnt;
    Instruction* new_insts = malloc( sizeof( Instruction )*fb->instruction_list.size );
    if( from > 0 )
        memcpy( new_insts, fb->instruction_list.value, sizeof( Instruction )*from );
    int rem = old_size-to-1;
    if( rem )
        memcpy( new_insts+from+size, fb->instruction_list.value+to+1, sizeof( Instruction )*rem );
    Instruction* ni = &new_insts[from];
    for( i = 0; i < size; i++, ni++, in++ ) {
        ni->opcode = in->opcode;
        ni->line_pos = in->line_pos;
        ni->hint = 0;
    }
    free( fb->instruction_list.value );
    fb->instruction_list.value = new_insts;

    // update local scope
    int unuse_cnt = 0;
    Local *l;
    for( i = 0, l = fb->local_list.value; i < fb->local_list.size; i++, l++ ) {
        if( l->end < from )
            continue;
        else if( l->start > to ) {
            l->start += mod_cnt;
            l->end += mod_cnt;
        }
        else if( l->start < from ) {
            if( l->end <= to )
                l->end = from+size;
            else
                l->end += mod_cnt;
        }
        else {
            if( l->end <= to ) {
                // unused local
                unuse_cnt++;
                l->start = l->end = -1;
            }
            else {
                l->start = from;
                l->end += mod_cnt;
            }
        }
    }
    // remove unused locals
    if( unuse_cnt > 0 ) {
        fb->local_list.size = fb->local_list.size-unuse_cnt;
        Local* new_locals = malloc( sizeof( Local )*( fb->local_list.size ) );
        Local* nl = new_locals;
        for( i = 0, l = fb->local_list.value; i < fb->local_list.size; i++, l++ ) {
            if( l->start != -1 && l->end != -1 ) {
                memcpy( nl, l, sizeof( Local ) );
                nl++;
            }
        }
        free( fb->local_list.value );
        fb->local_list.value = new_locals;
    }

    // update code block
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            if( cb->exit >= to )
                cb->exit += mod_cnt;
            if( cb->entry >= from )
                cb->entry += mod_cnt;
        }
    }
}

void delete_instruction( FunctionBlock* fb, CodeBlock* cb, int from, int to )
{
    modify_instruction( fb, cb, from, to, 0, 0 );
}

void insert_instruction( FunctionBlock* fb, CodeBlock* cb, int pos, Instruction* in, int size )
{
    modify_instruction( fb, cb, pos, -1, in, size );
}

int append_constant( FunctionBlock* fb, Constant* c )
{
    int i;
    Constant* pc;
    for( i = 0, pc = fb->constant_list.value; i < fb->constant_list.size; i++, pc++ ) {
        if( c->type == pc->type ) {
            if( c->type == LUA_TSTRING ) {
                if( is_same_string( &c->string, &pc->string ) )
                    return i;
            }
            else {
                if( c->number == pc->number )
                    return i;
            }
        }
    }

    int new_size = fb->constant_list.size+1;
    fb->constant_list.size = new_size;
    fb->constant_list.value = realloc( fb->constant_list.value, new_size*sizeof( Constant ) );
    fb->constant_list.value[new_size-1] = *c;
    return new_size-1;
}

//--------------------------------------------------
// optimize functions
//--------------------------------------------------

int dce_traverse( void* node )
{
    CodeBlock* cb = ( CodeBlock* )node;
    if( cb->reachable )
        return 0;
    cb->reachable = 1;
    return 1;
}

void* dce_iterator( void* node, void* next )
{
    CodeBlock* cb = ( CodeBlock* )node;
    CodeBlock** succ = cb->successors;
    if( next ) {
        int i;
        for( i = 0; i < cb->num_succ-1; i++, succ++ ) {
            if( next == *succ )
                return *( succ+1 );
        }
        return 0;
    }
    else
        return cb->num_succ > 0 ? *succ : 0;
}

void dead_code_elimination( FunctionBlock* fb, OptArg* oa )
{
    CodeBlock* cb = fb->code_block[0];
    dfs( cb, dce_traverse, dce_iterator );

    if( oa->hint ) return;

    // beware!!! only set a null pos
    int i;
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        cb = *ppcb;
        if( !cb->reachable ) {
            delete_instruction( fb, cb, cb->entry, cb->exit );

            *ppcb = 0;

            free( cb );
        }
    }
}

#define OPT_CF_ARITH            0x01
#define OPT_CF_HIGH_PRIORITY    0x02
#define OPT_CF_COMMUTABLE       0x04

#define OPT_CF_IS_LOCAL( R, iid ) ( fb->stack_frames ? fb->stack_frames[iid].slots[R] != -1 : 0 )

#define OPT_CF_CHECK_FROM \
    if( opt_from ) { \
        opt_to = j-1; \
        if( !oa->hint ) { \
            Instruction tmp_in; \
            make_instruction( &tmp_in, &prev ); \
            modify_instruction( fb, cb, opt_from, opt_to, &tmp_in, 1 ); \
            format_function( fb, oa ); \
            j -= ( opt_to-opt_from ); \
        } \
        opt_from = 0; \
        prev_flag = 0; \
    }

void constant_binary_arith_op( Constant* c1, Constant *c2, int op )
{
    if( c1->type != LUA_TNUMBER || c2->type != LUA_TNUMBER ) return;

    switch( op ) {
        case ADD:
            c1->number += c2->number;
            break;
        case MUL:
            c1->number *= c2->number;
            break;
    }
}

void constant_folding( FunctionBlock* fb, OptArg* oa )
{
    if( !oa->associative_law ) return;

    CodeBlock** ppcb = fb->code_block;
    int i;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            InstructionDetail curr, prev;
            char prev_flag = 0;
            int opt_from = 0, opt_to = 0;
            int j;
            for( j = cb->entry; j <= cb->exit; j++ ) {
                Instruction* in = &fb->instruction_list.value[j];
                get_instruction_detail( in, &curr );

                char flag = 0;
                switch( curr.op ) {
                    case ADD:
                        flag = OPT_CF_ARITH | OPT_CF_COMMUTABLE;
                        break;
                    case SUB:
                        flag = OPT_CF_ARITH;
                        break;
                    case MUL:
                        flag = OPT_CF_ARITH | OPT_CF_HIGH_PRIORITY | OPT_CF_COMMUTABLE;
                        break;
                    case DIV:
                        flag = OPT_CF_ARITH | OPT_CF_COMMUTABLE;
                        break;
                    default:
                        break;
                }

                // is arith op, has const param
                if( flag && ( IS_CONST( curr.B ) || IS_CONST( curr.C ) ) ) {
                    // same priority
                    // prev output reg is curr input reg
                    // prev output reg is an temporary slot
                    if( ( prev_flag & OPT_CF_HIGH_PRIORITY ) == ( flag & OPT_CF_HIGH_PRIORITY )
                        && ( prev.A == curr.B || prev.A == curr.C )
                        && !OPT_CF_IS_LOCAL( prev.A, j-1 ) ) {
                        if( oa->hint )
                            in->hint |= HINT_CONSTANT_FOLDING;
                        else {
                            // merge op
                            Constant *c1, *c2;
                            int prev_R;
                            if( IS_CONST( prev.B ) ) {
                                c1 = &fb->constant_list.value[prev.B-CONST_BASE];
                                prev_R = prev.C;
                            }
                            else {
                                c1 = &fb->constant_list.value[prev.C-CONST_BASE];
                                prev_R = prev.B;
                            }
                            if( IS_CONST( curr.B ) )
                                c2 = &fb->constant_list.value[curr.B-CONST_BASE];
                            else
                                c2 = &fb->constant_list.value[curr.C-CONST_BASE];
                            
                            Constant nc;
                            nc = *c1;
                            constant_binary_arith_op( &nc, c2, curr.op );
                            int cid = append_constant( fb, &nc );

                            if( IS_CONST( curr.B ) ) {
                                curr.B = cid+CONST_BASE;
                                curr.C = prev_R;
                            }
                            else {
                                curr.C = cid+CONST_BASE;
                                curr.B = prev_R;
                            }
                        }
                    }
                    else {
                        OPT_CF_CHECK_FROM;
                    }

                    if( !opt_from )
                        opt_from = j;

                    prev = curr;
                    prev_flag = flag;
                }

                if( !flag ) {
                    prev_flag = 0;

                    OPT_CF_CHECK_FROM;
                }
            }
            OPT_CF_CHECK_FROM;
        }
    }
}

void optimize( FunctionBlock* fb, OptArg* oa )
{
    // general optimization
    dead_code_elimination( fb, oa );

    // local optimization
    constant_folding( fb, oa );

    int i = 0;
    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( ; i < fb->num_func; i++, pfb++ )
        optimize( pfb, oa );
}
