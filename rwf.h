#ifndef __RWF_H__
#define __RWF_H__

#include "ltype.h"

//--------------------------------------------------
// functions
//--------------------------------------------------

int is_same_string( String* s1, String* s2 );

void read_function( FILE* f, FunctionBlock* fb, int lv );

#define FORMAT_LEVEL \
    for( tmp_lv = 0; tmp_lv < fb->level; tmp_lv++ ) \
        printf( "\t" ); \
    printf

void format_luaheader( LuaHeader* lh );
void format_function( FunctionBlock* fb, OptArg* fo, int recursive, int verbose );

void write_function( FILE* f, FunctionBlock* fb );

#endif
