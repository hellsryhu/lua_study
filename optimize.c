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
        list_add_tail( &cb->node, &fb->code_block_node ); \
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
    INIT_LIST_HEAD( &fb->code_block_node );
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
        list_add_tail( &cb->node, &fb->code_block_node );
    }

    for( i = 0; i < jmps_idx; i++ ) {
        jmps_from[i] = &jmps[i];
        jmps_to[i] = &jmps[i];
    }
    quick_sort( ( void** )jmps_to, 0, jmps_idx-1, is_greater_to );

    // complete blocks
    struct list_head* pos = fb->code_block_node.next;
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
            if( pos != &fb->code_block_node ) {
                cb = list_entry( pos, CodeBlock, node );
            }
            else
                break;
        }
    }

    i = 0;
    list_for_each( pos, &fb->code_block_node ) {
        cb = list_entry( pos, CodeBlock, node );
        cb->id = ++i;
    }

    fb->num_code_block = i;
    fb->code_block = ( CodeBlock** )malloc(i*sizeof( CodeBlock* ) );
    CodeBlock** ppcb = fb->code_block;
    list_for_each( pos, &fb->code_block_node )
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

void dead_code_elimination( FunctionBlock* fb )
{
    CodeBlock* cb = fb->code_block[0];
    dfs( cb, dce_traverse, dce_iterator );
}

void optimize( FunctionBlock* fb )
{
    dead_code_elimination( fb );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    int i;
    for( i = 0; i < fb->num_func; i++ ) {
        optimize( pfb );
        pfb++;
    }
}

