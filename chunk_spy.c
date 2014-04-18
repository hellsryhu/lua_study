#include <unistd.h>
#include "optimize.h"

extern InstructionDesc INSTRUCTION_DESC[];

void output_usage()
{
    printf( "usage: chunk_spy [options] lc_filename\n" );
    printf( "\t-f       flow analyse\n" );
    printf( "\t-h       show lua header\n" );
    printf( "\t-o name  optimize output to file 'name'\n" );
    printf( "\t-q       quiet mode (show no function list)\n" );
    printf( "\t-s       show instruction summary\n" );
    printf( "\t-v       show opcode verbose info\n" );
}

int main( int argc, char* argv[] )
{
    if( argc < 2 ) {
        output_usage();
        return 0;
    }

    OptArg oa;
    memset( &oa, 0, sizeof( OptArg ) );

    int ch;
    while( ( ch = getopt( argc, argv, "fho:qsv" ) ) != EOF ) {
        switch( ch ) {
            case 'f':
                oa.flow = 1;
                break;
            case 'h':
                oa.header = 1;
                break;
            case 'o':
                oa.flow = 1;
                oa.optimize = 1;
                oa.opt_output = optarg;
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
            default:
                output_usage();
                return 0;
        }
    }

    char* filename = argv[argc-1];
    if( oa.optimize && strcmp( filename, oa.opt_output ) == 0 ) {
        printf( "output file can not be input file\n" );
        output_usage();
        return 0;
    }
    printf( "filename:\t%s\n\n", filename );

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

    fclose( f );

    if( oa.flow )
        flow_analysis( &fb, &oa );

    if( oa.optimize ) {
        optimize( &fb, &oa );
    }

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

    if( oa.optimize ) {
        f = fopen( oa.opt_output, "w+" );

        fwrite( &lh, 1, sizeof( LuaHeader ), f );

        write_function( f, &fb );

        fclose( f );
    }

    return 0;
}
