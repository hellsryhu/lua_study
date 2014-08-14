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

void quick_sort2( int* iarray, int offset, void** varray, int start, int end, int ( *is_greater )( void** varray, int, int ) )
{
    if( start >= end ) return;

    int pivot = iarray[( start+end )/2];
    int m = start;
    int n = end;
    do {
        while( is_greater( varray, pivot-offset, iarray[m]-offset ) && m < end )
            m++;
        while( is_greater( varray, iarray[n]-offset, pivot-offset ) && n > start )
            n--;
        if( m <= n ) {
            int tmp = iarray[m];
            iarray[m] = iarray[n];
            iarray[n] = tmp;
            m++;
            n--;
        }
    } while( m <= n );
    if( m < end )
        quick_sort2( iarray, offset, varray, m, end, is_greater );
    if( n > start )
        quick_sort2( iarray, offset, varray, start, n, is_greater );

}
