#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "chunk_type.h"

int main( int argc, char* argv[] )
{
    if( argc < 2 )
        return 0;

    OptArg oa;
    memset( &oa, 0, sizeof( OptArg ) );

    int ch;
    while( ( ch = getopt( argc, argv, "fhv" ) ) != EOF ) {
        switch( ch ) {
            case 'f':
                oa.flow = 1;
                break;
            case 'h':
                oa.header = 1;
                break;
            case 'v':
                oa.verbose = 1;
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
    if( oa.header ) {
        format_luaheader( &lh );
        printf( "\n" );
    }

    FunctionBlock fb;
    INIT_FUNCTION_BLOCK( &fb );
    read_function( f, &fb, 0 );

    if( oa.flow )
        flow_analysis( &fb, &oa );

    format_function( &fb, &oa );

    fclose( f );
    return 0;
}
