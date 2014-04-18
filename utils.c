#include <stdio.h>
#include "utils.h"

void quick_sort( void** array, int start, int end, int ( *is_greater )( void*, void* ) )
{
    if( start >= end ) return;

    void* pivot = array[( start+end )/2];
    int m = start;
    int n = end;
    do {
        while( is_greater( pivot, array[m] ) && m < end )
            m++;
        while( is_greater( array[n], pivot ) && n > start )
            n--;
        if( m <= n ) {
            void* tmp = array[m];
            array[m] = array[n];
            array[n] = tmp;
            m++;
            n--;
        }
    } while( m <= n );
    if( m < end )
        quick_sort( array, m, end, is_greater );
    if( n > start )
        quick_sort( array, start, n, is_greater );
}

void dfs( void* node, int ( *traverse )( void* ), void* ( *iterator )( void*, void* ) )
{
    if( !traverse( node ) ) return;

    void* next = 0;
    while( ( next = iterator( node, next ) ) != 0 )
        dfs( next, traverse, iterator );
}
