#ifndef __UTILS_H__
#define __UTILS_H__

void quick_sort( void** array, int start, int end, int ( *is_greater )( void*, void* ) );
void dfs( void* node, int ( *traverse )( void* ), void* ( *iterator )( void*, void* ) );

// sort iarray which stores indices cooresponding to varray which stores values
void quick_sort2( int* iarray, int offset, void** varray, int start, int end, int ( *is_greater )( void** varray, int, int ) );

#endif
