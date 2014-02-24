#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "chunk_type.h"

char* INSTRUCTION_NAME[] = {
    "MOVE",
    "LOADK",
    "LOADBOOL",
    "LOADNIL",
    "GETUPVAL",
    "GETGLOBAL",
    "GETTABLE",
    "SETGLOBAL",
    "SETUPVAL",
    "SETTABLE",
    "NEWTABLE",
    "SELF",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "POW",
    "UNM",
    "NOT",
    "LEN",
    "CONCAT",
    "JMP",
    "EQ",
    "LT",
    "LE",
    "TEST",
    "TESTSET",
    "CALL",
    "TAILCALL",
    "RETURN",
    "FORLOOP",
    "FORPREP",
    "TFORLOOP",
    "SETLIST",
    "CLOSE",
    "CLOSURE",
    "VARARG"
};

char* get_op_name( unsigned int opcode )
{
    return INSTRUCTION_NAME[opcode & 0x3f];
}

void read_string( FILE* f, String* str )
{
    fread( &str->size, sizeof( size_t ), 1, f );
    str->value = realloc( str->value, str->size );
    fread( str->value, 1, str->size, f );
}

void read_instruction( FILE* f, InstructionList* il )
{
    fread( &il->size, sizeof( int ), 1, f );
    il->value = realloc( il->value, sizeof( Instruction )*il->size );
    unsigned int *opcodes = malloc( sizeof( unsigned int )*il->size );
    fread( opcodes, sizeof( unsigned int ), il->size, f );
    int i;
    for( i = 0; i < il->size; i++ ) {
        il->value[i].opcode = opcodes[i];
        il->value[i].line_pos = 0;
    }
    free( opcodes );
}

void read_constant( FILE* f, ConstantList* cl )
{
    fread( &cl->size, sizeof( int ), 1, f );
    cl->value = realloc( cl->value, sizeof( Constant )*cl->size );
    int i;
    for( i = 0; i < cl->size; i++ ) {
        Constant* c = &cl->value[i];
        fread( &c->type, sizeof( unsigned char ), 1, f );
        switch( c->type ) {
            case LUA_TNIL:
                c->nil = 0;
                break;
            case LUA_TBOOLEAN:
                fread( &c->boolean, sizeof( unsigned char ), 1, f );
                break;
            case LUA_TNUMBER:
                fread( &c->number, sizeof( double ), 1, f );
                break;
            case LUA_TSTRING:
                memset( &c->string, 0, sizeof( String ) );
                read_string( f, &c->string );
                break;
            default:
                break;
        }
    }
}

void read_linepos( FILE* f, InstructionList* il )
{
    int size;
    fread( &size, sizeof( int ), 1, f );
    if( size != il->size ) {
        printf( "!!!ERROR!!! instruction size != line pos size\n" );
        exit( 0 );
    }
    int* lineposes = malloc( sizeof( int )*size );
    fread( lineposes, sizeof( int ), size, f );
    int i;
    for( i = 0; i < size; i++ )
        il->value[i].line_pos = lineposes[i];
    free( lineposes );
}

void read_local( FILE* f, LocalList* ll )
{
    fread( &ll->size, sizeof( int ), 1, f );
    ll->value = realloc( ll->value, sizeof( Local )*ll->size );
    memset( ll->value, 0, sizeof( Local )*ll->size );
    int i;
    for( i = 0; i < ll->size; i++ ) {
        Local* l = &ll->value[i];
        read_string( f, &l->name );
        fread( &l->start, sizeof( int ), 2, f );
    }
}

void read_upvalue( FILE* f, UpvalueList* ul )
{
    fread( &ul->size, sizeof( int ), 1, f );
    ul->value = realloc( ul->value, sizeof( String )*ul->size );
    memset( ul->value, 0, sizeof( String )*ul->size );
    int i;
    for( i = 0; i < ul->size; i++ ) {
        String* s = &ul->value[i];
        read_string( f, s );
    }
}

void read_function( FILE* f, FunctionBlock* fb )
{
    read_string( f, &fb->source_name );
    fread( &fb->first_line, 1, 12, f );
    read_instruction( f, &fb->instruction_list );
    read_constant( f, &fb->constant_list );
    fread( &fb->num_func, sizeof( int ), 1, f );
    fb->funcs = realloc( fb->funcs, sizeof( FunctionBlock )*fb->num_func );
    memset( fb->funcs, 0, sizeof( FunctionBlock )*fb->num_func );
    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    int i;
    for( i = 0; i < fb->num_func; i++ ) {
        read_function( f, pfb );
        pfb++;
    }
    read_linepos( f, &fb->instruction_list );
    read_local( f, &fb->local_list );
    read_upvalue( f, &fb->upvalue_list );
}

void format_luaheader( LuaHeader* lh )
{
    printf( "signature:\t0x%x\n", lh->header_signature );
    printf( "version:\t0x%x\n", lh->version );
    printf( "format version:\t%d\n", lh->format_version );
    printf( "endianess flag:\t%d\n", lh->endianess_flag );
    printf( "size of int:\t%d\n", lh->size_int );
    printf( "size of size_t:\t%d\n", lh->size_size_t );
    printf( "size of instruction:\t%d\n", lh->size_instruction );
    printf( "size of lua number:\t%d\n", lh->size_number );
    printf( "integral flag:\t%d\n", lh->integral_flag );
}

void format_function( FunctionBlock* fb )
{
    printf( "source name:\t%s\n", fb->source_name.value );
    printf( "line defined:\t%d\n", fb->first_line );
    printf( "last line defined:\t%d\n", fb->last_line );
    printf( "number of upvalues:\t%d\n", fb->num_upvalue );
    printf( "number of parameters:\t%d\n", fb->num_parameter );
    printf( "is_vararg flag:\t%d\n", fb->is_vararg );
    printf( "maximum stack size:\t%d\n", fb->max_stack_size );

    printf( "instruction list:\n" );
    int i;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        printf( "\t%d\t[%d]\t%s\n", i, in->line_pos, get_op_name( in->opcode ) );
    }

    printf( "constant list:\n" );
    for( i = 0; i < fb->constant_list.size; i++ ) {
        printf( "\t%d\t", i );
        Constant* c = &fb->constant_list.value[i];
        switch( c->type ) {
            case LUA_TNIL:
                printf( "nil" );
                break;
            case LUA_TBOOLEAN:
                if( c->boolean )
                    printf( "true" );
                else
                    printf( "false" );
                break;
            case LUA_TNUMBER:
                {
                    double fractional = c->number-( long long )( c->number );
                    if( fractional == 0 )
                        printf( "%lld", ( long long )( c->number ) );
                    else
                        printf( "%f", c->number );
                }
                break;
            case LUA_TSTRING:
                printf( "%s", c->string.value );
                break;
            default:
                break;
        }
        printf( "\n" );
    }

    printf( "Local list:\n" );
    for( i = 0; i < fb->local_list.size; i++ ) {
        Local* l = &fb->local_list.value[i];
        printf( "\t%d\t%s\t(%d,%d)\n", i, l->name.value, l->start, l->end );
    }

    printf( "Upvalue List:\n" );
    for( i = 0; i < fb->upvalue_list.size; i++ ) {
        String* s = &fb->upvalue_list.value[i];
        printf( "\t%d\t%s\n", i, s->value );
    }

    printf( "\n" );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( i = 0; i < fb->num_func; i++ ) {
        format_function( pfb );
        pfb++;
    }
}
