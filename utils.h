#ifndef __UTILS_H__
#define __UTILS_H__

void quick_sort( void** array, int start, int end, int ( *is_greater )( void*, void* ) );
void dfs( void* node, int ( *traverse )( void* ), void* ( *iterator )( void*, void* ) );

#endif
