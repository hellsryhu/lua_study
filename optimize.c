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

static void quick_sort( int* array, int start, int end )
{
    if( start >= end ) return;

    int pivot = ( start+end )/2;
    int i;
    for( i = start; i <= end; i++ ) {
        if( ( i < pivot && array[i] > array[pivot] ) || ( i > pivot && array[i] < array[pivot] ) ) {
            int tmp = array[i];
            array[i] = array[pivot];
            array[pivot] = tmp;
            pivot = i;
        }
    }
    quick_sort( array, start, pivot-1 );
    quick_sort( array, pivot+1, end );
}
    
void flow_analysis( FunctionBlock* fb, OptArg* oa )
{
    int* jmp_to = malloc( fb->instruction_list.size*sizeof( int ) );
    memset( jmp_to, 0, fb->instruction_list.size*sizeof( int ) );
    int jmp_to_idx = 0;

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
                    jmp_to[jmp_to_idx++] = i+2;
                }
                break;
            case JMP:
                CODE_BLOCK_EXIT( i );
                CODE_BLOCK_ENTRY( i+1 );
                jmp_to[jmp_to_idx++] = i+sBx+1;
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
                jmp_to[jmp_to_idx++] = i+2;
                break;
            default:
                break;
        }
    }
    if( cb ) {
        cb->exit = fb->instruction_list.size-1;
        list_add_tail( &cb->node, &fb->code_block );
    }

    quick_sort( jmp_to, 0, jmp_to_idx-1 );

    struct list_head* pos = fb->code_block.next;
    cb = 0;
    if( pos != &fb->code_block )
        cb = list_entry( pos, CodeBlock, node );
    i = 0;
    while( i < jmp_to_idx ) {
        if( jmp_to[i] == cb->entry )
            i++;
        else if( jmp_to[i] <= cb->exit ) {
            CodeBlock* ocb = cb;
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
            memset( cb, 0, sizeof( CodeBlock ) );
            cb->entry = jmp_to[i];
            cb->exit = cb->exit;

            cb->exit = jmp_to[i]-1;
            list_add( &cb->node, &ocb->node );

            i++;
        }
        else if( jmp_to[i] > cb->exit ) {
            pos = pos->next;
            if( pos != &fb->code_block )
                cb = list_entry( pos, CodeBlock, node );
            else
                break;
        }
    }

    free( jmp_to );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( i = 0; i < fb->num_func; i++ ) {
        flow_analysis( pfb, oa );
        pfb++;
    }
}
