#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "chunk_type.h"

int main( int argc, char* argv[] )
{
    if( argc < 2 )
        return 0;

    FormatOpt fo;
    memset( &fo, 0, sizeof( FormatOpt ) );

    int ch;
    while( ( ch = getopt( argc, argv, "hv" ) ) != EOF ) {
        switch( ch ) {
            case 'h':
                fo.header = 1;
                break;
            case 'v':
                fo.verbose = 1;
                break;
        }
    }

    char* filename = argv[1];
    if( argc > 2 )
        filename = argv[2];
    printf( "filename:\t%s\n", filename );

    FILE* f = fopen( filename, "r" );
    if( !f )
        return 0;

    LuaHeader lh;
    fread( &lh, 1, sizeof( LuaHeader ), f );
    if( fo.header ) {
        format_luaheader( &lh );
        printf( "\n" );
    }

    FunctionBlock fb;
    memset( &fb, 0, sizeof( FunctionBlock ) );
    read_function( f, &fb, 0 );

    format_function( &fb, &fo );

    fclose( f );
    return 0;
}
