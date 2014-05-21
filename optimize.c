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

void delete_instruction( FunctionBlock* fb, int from, int to )
{
    if( from < 0 || from >= fb->instruction_list.size ) return;
    if( to < 0 || to >= fb->instruction_list.size ) return;
    if( from > to ) return;

    int del_cnt = to-from+1;
    int old_size = fb->instruction_list.size;
    // 不能删整个函数，至少要有一个return
    if( del_cnt == old_size ) return;

    // 不能跨code block删
    CodeBlock** ppcb = fb->code_block;
    int i;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb && ( ( from < cb->entry && to > cb->entry ) || ( from < cb->exit && to > cb->exit ) ) ) return;
    }

    // update jmp
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        switch( ind.op ) {
            case JMP:
            case FORLOOP:
            case FORPREP:
                { 
                    int jmp_to = i+ind.sBx+1;
                    if( i < from ) {
                        if( jmp_to > to )
                            jmp_to -= del_cnt;
                        else if( jmp_to >= from )
                            jmp_to = from;
                    }
                    else if( i > to ) {
                        if( jmp_to < from )
                            jmp_to -= del_cnt;
                        else if( jmp_to <= to )
                            jmp_to = to+1;
                    }
                    ind.sBx = jmp_to-i-1;
                }
                make_instruction( in, &ind );
                break;
            default:
                break;
        }
    }

    // update instruction list
    fb->instruction_list.size -= del_cnt;
    Instruction* new_insts = malloc( sizeof( Instruction )*fb->instruction_list.size );
    if( from > 0 )
        memcpy( new_insts, fb->instruction_list.value, sizeof( Instruction )*from );
    int rem = old_size-to-1;
    if( rem )
        memcpy( new_insts+from, fb->instruction_list.value+to+1, sizeof( Instruction )*rem );
    free( fb->instruction_list.value );
    fb->instruction_list.value = new_insts;

    // update local scope
    int unuse_cnt = 0;
    Local *l;
    for( i = 0; i < fb->local_list.size; i++ ) {
        l = &fb->local_list.value[i];
        if( l->end < from )
            continue;
        else if( l->start > to ) {
            l->start -= del_cnt;
            l->end -= del_cnt;
        }
        else if( l->start < from ) {
            if( l->end <= to )
                l->end = from;
            else
                l->end -= del_cnt;
        }
        else {
            if( l->end <= to ) {
                // unused local
                unuse_cnt++;
                l->start = l->end = -1;
            }
            else {
                l->start = from;
                l->end -= del_cnt;
            }
        }

        if( l->start > to )
            l->start -= del_cnt;
        else if( l->start >= from )
            l->start = from;
    }
    // remove unused locals
    if( unuse_cnt > 0 ) {
        fb->local_list.size = fb->local_list.size-unuse_cnt;
        Local* new_locals = malloc( sizeof( Local )*( fb->local_list.size ) );
        int next = 0;
        for( i = 0; i < fb->local_list.size; i++ ) {
            l = &fb->local_list.value[i];
            if( l->start != -1 && l->end != -1 ) {
                Local* nl = &new_locals[next++];
                memcpy( nl, l, sizeof( Local ) );
            }
        }
        free( fb->local_list.value );
        fb->local_list.value = new_locals;
    }

    // update code block
    ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            if( cb->exit >= to )
                cb->exit -= del_cnt;
            if( cb->entry >= from )
                cb->entry -= del_cnt;
        }
    }

    // TODO
    // update constant
}

void insert_instruction( FunctionBlock* fb, int pos, Instruction* in )
{
    // update jmp
    int i;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        switch( ind.op ) {
            case JMP:
            case FORLOOP:
            case FORPREP:
                {
                    int jmp_to = i+ind.sBx+1;
                    if( i < pos && jmp_to >= pos )
                        ind.sBx++;
                    else if( i >= pos && jmp_to < pos )
                        ind.sBx--;
                }
                break;
            default:
                break;
        }
    }

    // update instruction list
    int old_size = fb->instruction_list.size;
    fb->instruction_list.size++;
    Instruction* new_insts = malloc( sizeof( Instruction )*fb->instruction_list.size );
    if( pos > 0 )
        memcpy( new_insts, fb->instruction_list.value, sizeof( Instruction )*pos );
    if( pos < old_size-1 )
        memcpy( new_insts, fb->instruction_list.value+pos+1, sizeof( Instruction )*( old_size-pos ) );
    Instruction* ni = &fb->instruction_list.value[pos];
    ni->opcode = in->opcode;
    ni->line_pos = in->line_pos;

    // update local scope
    for( i = 0; i < fb->local_list.size; i++ ) {
        Local *l = &fb->local_list.value[i];
        if( l->start >= pos ) {
            l->start++;
            l->end++;
        }
        else if( l->start < pos && l->end >= pos )
            l->end++;
    }

    // update code block
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            if( pos >= cb->entry && pos <= cb->exit )
                cb->exit++;
            else if( pos < cb->exit ) {
                cb->entry++;
                cb->exit++;
            }
        }
    }
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
            delete_instruction( fb, cb->entry, cb->exit );

            *ppcb = 0;

            free( cb );
        }
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
            int j;
            Instruction* in = &fb->instruction_list.value[cb->entry];
            Instruction* prev_in = 0;
            InstructionDetail prev, curr;
            for( j = cb->entry; j <= cb->exit; j++, in++ ) {
                char optimizable = 0;
                get_instruction_detail( in, &curr );
                if( curr.op == ADD || curr.op == MUL ) {
                    // associative law
                    // K+R = R+K
                    // K+K is already optimized by luac
                    // R+R is not optimizable
                    if( IS_CONST( curr.B ) && !IS_CONST( curr.C ) ) {
                        unsigned short tmp = curr.C;
                        curr.C = curr.B;
                        curr.B = tmp;
                    }
                    if( !IS_CONST( curr.B ) && IS_CONST( curr.C ) ) {
                        optimizable = 1;

                        if( prev_in && prev.op == curr.op && prev.A == curr.A && curr.A == curr.B ) {
                            in->hint |= HINT_CONSTANT_FOLDING;
                        }
                        prev_in = in;
                        prev = curr;
                    }

                    // K-R = -R+K
                }

                if( !optimizable )
                    prev_in = 0;
            }
        }
    }
}

void optimize( FunctionBlock* fb, OptArg* oa )
{
    // general optimization
    dead_code_elimination( fb, oa );

    // local optimization
    constant_folding( fb, oa );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    int i;
    for( i = 0; i < fb->num_func; i++ ) {
        optimize( pfb, oa );
        pfb++;
    }
}
