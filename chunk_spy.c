#include <stdio.h>
#include <memory.h>
#include "chunk_type.h"

int main( int argc, char* argv[] )
{
    if( argc < 2 )
        return 0;

    char* filename = argv[1];
    printf( "filename:\t%s\n", filename );

    FILE* f = fopen( filename, "r" );
    if( !f )
        return 0;

    LuaHeader lh;
    fread( &lh, 1, sizeof( LuaHeader ), f );
    format_luaheader( &lh );
    
    printf( "\n" );

    FunctionBlock fb;
    memset( &fb, 0, sizeof( FunctionBlock ) );
    read_function( f, &fb, 0 );

    format_function( &fb );

    fclose( f );
    return 0;
}
