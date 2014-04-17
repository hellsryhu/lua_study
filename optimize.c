#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "optimize.h"

//--------------------------------------------------
// control flow analysis functions
//--------------------------------------------------

#define CODE_BLOCK_EXIT( id ) \
    { \
        cb->exit = id; \
        list_add_tail( &cb->node, &fb->code_block ); \
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

void quick_sort( void** array, int start, int end, int ( *is_greater )( void*, void* ) )
{
    if( start >= end ) return;

    int pivot = ( start+end )/2;
    int i;
    for( i = start; i <= end; i++ ) {
        if( ( i < pivot && is_greater( array[i], array[pivot] ) ) || ( i > pivot && is_greater( array[pivot], array[i] ) ) ) {
            void* tmp = array[i];
            array[i] = array[pivot];
            array[pivot] = tmp;
            pivot = i;
        }
    }
    quick_sort( array, start, pivot-1, is_greater );
    quick_sort( array, pivot+1, end, is_greater );
}

CodeBlock* get_code_block( FunctionBlock* fb, int instruction_id )
{
    struct list_head* pos;
    list_for_each( pos, &fb->code_block ) {
        CodeBlock* cb = list_entry( pos, CodeBlock, node );
        if( instruction_id >= cb->entry && instruction_id <= cb->exit )
            return cb;
    }
    return 0;
}

void create_successors( FunctionBlock* fb, CodeBlock* cb, int first, int last, struct list_head* pos, Jump** jmps_from )
{
    int size = last-first;
    int contain_next = 0;
    int i;
    if( pos != fb->code_block.prev ) {
        // last block
        for( i = first; i < last; i++ ) {
            CodeBlock* tcb = get_code_block( fb, jmps_from[i]->to );
            if( tcb->id == cb->id+1 ) {
                contain_next = 1;
                break;
            }
        }
        if( !contain_next )
            size++;
    }

    cb->successors = malloc( size*sizeof( CodeBlock* ) );
    cb->size_succ = size;
    CodeBlock** succ = ( CodeBlock** )cb->successors;

    for( i = first; i < last; i++ ) {
        int to = jmps_from[i]->to;
        *succ = get_code_block( fb, to );
        succ++;
    }

    if( pos != fb->code_block.prev && !contain_next )
        *succ = list_entry( pos->next, CodeBlock, node );
    
    succ = ( CodeBlock** )cb->successors;
    for( i = 0; i < size; i++ ) {
        //printf( "cb: %d, succ: %d\n", cb->id, ( *succ )->id );
        succ++;
    }
}
 
void create_predecessors( FunctionBlock* fb, CodeBlock* cb, int first, int last, struct list_head* pos, Jump** jmps_to )
{
    int size = last-first;
    int contain_prev = 0;
    int i;
    if( pos != fb->code_block.next ) {
        // first block
        for( i = first; i < last; i++ ) {
            CodeBlock* tcb = get_code_block( fb, jmps_to[i]->from );
            if( tcb->id == cb->id-1 ) {
                contain_prev = 1;
                break;
            }
        }
        if( !contain_prev )
            size++;
    }

    cb->predecessors = malloc( size*sizeof( CodeBlock* ) );
    cb->size_pred = size;
    CodeBlock** pred = ( CodeBlock** )cb->predecessors;

    for( i = first; i < last; i++ ) {
        int from = jmps_to[i]->from;
        *pred = get_code_block( fb, from );
        pred++;
    }

    if( pos != fb->code_block.next && !contain_prev )
        *pred = list_entry( pos->prev, CodeBlock, node );
    
    pred = ( CodeBlock** )cb->predecessors;
    for( i = 0; i < size; i++ ) {
        //printf( "cb: %d, pred: %d\n", cb->id, ( *pred )->id );
        pred++;
    }
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

    INIT_LIST_HEAD( &fb->code_block );
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
        list_add_tail( &cb->node, &fb->code_block );
    }

    for( i = 0; i < jmps_idx; i++ ) {
        jmps_from[i] = &jmps[i];
        jmps_to[i] = &jmps[i];
    }
    quick_sort( ( void** )jmps_to, 0, jmps_idx-1, is_greater_to );

    struct list_head* pos = fb->code_block.next;
    cb = list_entry( pos, CodeBlock, node );
    cb->id = 1;
    i = 0;
    while( i < jmps_idx ) {
        if( jmps_to[i]->to == cb->entry )
            i++;
        else if( jmps_to[i]->to <= cb->exit ) {
            CodeBlock* ocb = cb;
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
            memset( cb, 0, sizeof( CodeBlock ) );
            cb->id = ocb->id+1;
            cb->entry = jmps_to[i]->to;
            cb->exit = ocb->exit;

            ocb->exit = jmps_to[i]->to-1;
            list_add( &cb->node, &ocb->node );

            pos = &cb->node;

            i++;
        }
        else if( jmps_to[i]->to > cb->exit ) {
            pos = pos->next;
            if( pos != &fb->code_block ) {
                CodeBlock* ocb = cb;
                cb = list_entry( pos, CodeBlock, node );
                cb->id = ocb->id+1;
            }
            else
                break;
        }
    }

    /*
    list_for_each( pos, &fb->code_block ) {
        cb = list_entry( pos, CodeBlock, node );
        printf( "cb: %d, entry: %d, exit: %d\n", cb->id, cb->entry, cb->exit );
    }
    */

    // gen successor
    pos = fb->code_block.next;
    cb = list_entry( pos, CodeBlock, node );
    i = 0;
    int first = 0;
    while( i < jmps_idx ) {
        //printf( "i: %d, from: %d, to: %d, cb: %d, entry: %d, exit: %d, to: %d, from: %d\n",
                //i, jmps_from[i]->from, jmps_from[i]->to, cb->id, cb->entry, cb->exit, jmps_to[i]->to, jmps_to[i]->from );
        if( jmps_from[i]->from <= cb->exit )
            i++;
        else {
            create_successors( fb, cb, first, i, pos, jmps_from );

            first = i;

            pos = pos->next;
            if( pos != &fb->code_block )
                cb = list_entry( pos, CodeBlock, node );
        }
    }
    create_successors( fb, cb, first, i, pos, jmps_from );

    while( pos != &fb->code_block ) {
        cb = list_entry( pos, CodeBlock, node );
        create_successors( fb, cb, 0, -1, pos, jmps_from );
        pos = pos->next;
    }

    // gen predecessor
    pos = fb->code_block.next;
    cb = list_entry( pos, CodeBlock, node );
    i = 0;
    first = 0;
    while( i < jmps_idx ) {
        //printf( "i: %d, from: %d, to: %d, cb: %d, entry: %d, exit: %d, to: %d, from: %d\n",
                //i, jmps_from[i]->from, jmps_from[i]->to, cb->id, cb->entry, cb->exit, jmps_to[i]->to, jmps_to[i]->from );
        if( jmps_to[i]->to <= cb->exit )
            i++;
        else {
            create_predecessors( fb, cb, first, i, pos, jmps_to );

            first = i;

            pos = pos->next;
            if( pos != &fb->code_block )
                cb = list_entry( pos, CodeBlock, node );
        }
    }
    create_predecessors( fb, cb, first, i, pos, jmps_to );

    while( pos != &fb->code_block ) {
        cb = list_entry( pos, CodeBlock, node );
        create_predecessors( fb, cb, 0, -1, pos, jmps_to );
        pos = pos->next;
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
