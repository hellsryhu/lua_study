#include <unistd.h>
#include "optimize.h"

extern InstructionDesc INSTRUCTION_DESC[];

void output_usage()
{
    printf( "usage: chunk_spy [options] lc_filename\n" );
    printf( "\t-h       show optimize hint\n" );
    printf( "\t-H       show lua header\n" );
    printf( "\t-o name  optimize output to file 'name'\n" );
    printf( "\t-O       show optimized code\n" );
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
    while( ( ch = getopt( argc, argv, "hHo:Oqsv" ) ) != EOF ) {
        switch( ch ) {
            case 'h':
                oa.optimize = 1;
                oa.hint = 1;
                break;
            case 'H':
                oa.header = 1;
                break;
            case 'o':
                oa.optimize = 1;
                oa.opt_output = optarg;
                break;
            case 'O':
                oa.optimize = 1;
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
    if( oa.opt_output && strcmp( filename, oa.opt_output ) == 0 ) {
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

    if( !oa.quiet ) {
        if( oa.hint ) {
            flow_analysis( &fb, &oa );
            optimize( &fb, &oa );
        }
        format_function( &fb, &oa );
        printf( "\n" );
    }

    if( oa.summary ) {
        printf( "total instruction num: %d\n", smr.total_instruction_num );
        int i;
        for( i = 0; i <= VARARG; i++ ) {
            if( smr.instruction_num[i] ) {
                InstructionDesc* id = &INSTRUCTION_DESC[i];
                printf( "\t%s\t%d\n", id->name, smr.instruction_num[i] );
            }
        }
        printf( "\n" );
    }

    if( oa.optimize && !oa.hint ) {
        flow_analysis( &fb, &oa );

        optimize( &fb, &oa );

        if( oa.opt_output ) {
            f = fopen( oa.opt_output, "w+" );

            fwrite( &lh, 1, sizeof( LuaHeader ), f );

            write_function( f, &fb );

            fclose( f );
        }
        else {
            format_function( &fb, &oa );
        }
    }

    return 0;
}
