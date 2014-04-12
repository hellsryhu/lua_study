#include <unistd.h>
#include "optimize.h"

extern InstructionDesc INSTRUCTION_DESC[];

int main( int argc, char* argv[] )
{
    if( argc < 2 ) {
        printf( "usage: chunk_spy [-fhqsv] lc_filename\n" );
        return 0;
    }

    OptArg oa;
    memset( &oa, 0, sizeof( OptArg ) );

    int ch;
    while( ( ch = getopt( argc, argv, "fhqsv" ) ) != EOF ) {
        switch( ch ) {
            case 'f':
                oa.flow = 1;
                break;
            case 'h':
                oa.header = 1;
                break;
            case 'q':
                oa.quiet = 1;
                break;
            case 's':
                oa.summary = 1;
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
    Summary smr;
    memset( &smr, 0, sizeof( Summary ) );
    read_function( f, &fb, 0, &smr );

    if( oa.flow )
        flow_analysis( &fb, &oa );

    if( !oa.quiet )
        format_function( &fb, &oa );

    if( oa.summary ) {
        printf( "total instruction num: %d\n", smr.total_instruction_num );
        int i;
        for( i = 0; i <= VARARG; i++ ) {
            if( smr.instruction_num[i] ) {
                InstructionDesc* id = &INSTRUCTION_DESC[i];
                printf( "\t%s\t%d\n", id->name, smr.instruction_num[i] );
            }
        }
    }

    fclose( f );
    return 0;
}
