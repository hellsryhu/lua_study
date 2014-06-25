#include "rwf.h"
#include "analyze.h"

//--------------------------------------------------
// constants
//--------------------------------------------------

extern const char* sABC[];
extern const char* sABx[];
extern const char* sAsBx[];

extern InstructionDesc INSTRUCTION_DESC[];

extern const char* OPTIMIZATION_HINT[];

//--------------------------------------------------
// util functions
//--------------------------------------------------

int is_same_string( String* s1, String* s2 )
{
    if( s1->size != s2->size )
        return 0;
    return memcmp( s1, s2, s1->size ) == 0;
}

//--------------------------------------------------
// read functions
//--------------------------------------------------

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
        il->value[i].hint = 0;
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
    if( !size )
        return;
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
    if( !ll->size )
        return;
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
    if( !ul->size )
        return;
    ul->value = realloc( ul->value, sizeof( String )*ul->size );
    memset( ul->value, 0, sizeof( String )*ul->size );
    int i;
    for( i = 0; i < ul->size; i++ ) {
        String* s = &ul->value[i];
        read_string( f, s );
    }
}

void read_function( FILE* f, FunctionBlock* fb, int lv )
{
    read_string( f, &fb->source_name );
    fread( &fb->first_line, 1, 12, f );

    read_instruction( f, &fb->instruction_list );

    read_constant( f, &fb->constant_list );
    fread( &fb->num_func, sizeof( int ), 1, f );
    fb->funcs = realloc( fb->funcs, sizeof( FunctionBlock )*fb->num_func );
    int i;
    FunctionBlock* pfb;
    for( i = 0, pfb = fb->funcs; i < fb->num_func; i++, pfb++ ) {
        memset( pfb, 0, sizeof( FunctionBlock ) );
        read_function( f, pfb, lv+1 );
    }
    read_linepos( f, &fb->instruction_list );
    read_local( f, &fb->local_list );
    read_upvalue( f, &fb->upvalue_list );
    fb->level = lv;

    reset_stack_frames( fb );
}

//--------------------------------------------------
// format output functions
//--------------------------------------------------

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

#define FORMAT_REGISTER( R ) \
    { \
        if( fb->stack_frames ) { \
            int slot = fb->stack_frames[order].slots[R]; \
            if( slot != -1 ) \
                printf( "%s", fb->local_list.value[slot].name.value ); \
            else \
                printf( "r(%d)", R ); \
        } \
        else \
            printf( "r(%d)", R ); \
    }

#define FORMAT_RK( RK ) \
    if( IS_CONST( RK ) ) \
        format_constant( &fb->constant_list.value[RK-CONST_BASE] ); \
    else { \
        FORMAT_REGISTER( RK ) \
    }

#define FORMAT_BOOL( B ) \
    if( B ) \
        printf( "true" ); \
    else \
        printf( "false" );

#define FORMAT_UPVALUE( UV ) \
    printf( "%s", fb->upvalue_list.value[UV].value );
    
void format_constant( Constant* c )
{
    switch( c->type ) {
        case LUA_TNIL:
            printf( "nil" );
            break;
        case LUA_TBOOLEAN:
            FORMAT_BOOL( c->boolean );
            break;
        case LUA_TNUMBER:
            {
                if( c->number < 0 )
                    printf( "(" );
                double fractional = c->number-( long long )( c->number );
                if( fractional == 0 )
                    printf( "%lld", ( long long )( c->number ) );
                else
                    printf( "%f", c->number );
                if( c->number < 0 )
                    printf( ")" );
            }
            break;
        case LUA_TSTRING:
            printf( "\"%s\"", c->string.value );
            break;
        default:
            break;
    }
}

void format_instruction( FunctionBlock* fb, Instruction* in, int order, OptArg* oa )
{
    int tmp_lv;
    if( oa->hint ) {
        int i;
        for( i = 0; i < 32; i++ ) {
            int mask = 1 << i;
            if( in->hint & mask ) {
                FORMAT_LEVEL( "\t! %s\n", OPTIMIZATION_HINT[i] );
            }
        }
    }
    InstructionDetail ind;
    get_instruction_detail( in, &ind );
    FORMAT_LEVEL( "\t%d.\t[%d]\t%s\t", order, in->line_pos, ind.desc->name );
    switch( ind.desc->type ) {
        case iABC:
            printf( "%d", ind.A );
            if( ind.desc->param_num >= 2 )
                printf( " %d", ind.B );
            if( ind.desc->param_num == 3 )
                printf( " %d", ind.C );
            break;
        case iABx:
            printf( "%d", ind.A );
            if( ind.desc->param_num >= 2 )
                printf( " %d", ind.Bx );
            break;
        case iAsBx:
            if( ind.desc->param_num >= 2 )
                printf( "%d ", ind.A );
            printf( "%d", ind.sBx );
            break;
        default:
            break;
    }
    printf( "\t\t; " );
    switch( ind.op ) {
        case MOVE:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_REGISTER( ind.B );
            break;
        case LOADK:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            format_constant( &fb->constant_list.value[ind.Bx] );
            break;
        case LOADBOOL:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_BOOL( ind.B );
            if( ind.C )
                printf( "; goto %d", order+2 );
            break;
        case LOADNIL:
            {
                int r;
                for( r = ind.A; r <= ind.B; r++ ) {
                    FORMAT_REGISTER( r );
                    printf( " = nil" );
                    if( r < ind.B )
                        printf( "; " );
                }
            }
            break;
        case GETUPVAL:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_UPVALUE( ind.B );
            break;
        case GETGLOBAL:
            FORMAT_REGISTER( ind.A );
            printf( " = _G[" );
            format_constant( &fb->constant_list.value[ind.Bx] );
            printf( "]" );
            break;
        case GETTABLE:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_REGISTER( ind.B );
            printf( "[" );
            FORMAT_RK( ind.C );
            printf( "]" );
            break;
        case SETGLOBAL:
            printf( "_G[" );
            format_constant( &fb->constant_list.value[ind.Bx] );
            printf( "] = " );
            FORMAT_REGISTER( ind.A );
            break;
        case SETUPVAL:
            FORMAT_UPVALUE( ind.B );
            printf( " = " );
            FORMAT_REGISTER( ind.A );
            break;
        case SETTABLE:
            FORMAT_REGISTER( ind.A );
            printf( "[" );
            FORMAT_RK( ind.B );
            printf( "] = " );
            FORMAT_RK( ind.C );
            break;
        case NEWTABLE:
            FORMAT_REGISTER( ind.A );
            printf( " = {}" );
            if( ind.B > 0 )
                printf( "; array = %d", ind.B );
            if( ind.C > 0 )
                printf( "; hash = %d", ind.C );
            break;
        case SELF:
            FORMAT_REGISTER( ind.A+1 );
            printf( " = " );
            FORMAT_REGISTER( ind.B );
            printf( "; " );
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_REGISTER( ind.B );
            printf( "[" );
            FORMAT_RK( ind.C );
            printf( "]" );
            break;
        case ADD:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "+" );
            FORMAT_RK( ind.C );
            break;
        case SUB:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "-" );
            FORMAT_RK( ind.C );
            break;
        case MUL:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "*" );
            FORMAT_RK( ind.C );
            break;
        case DIV:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "/" );
            FORMAT_RK( ind.C );
            break;
        case MOD:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "%%" );
            FORMAT_RK( ind.C );
            break;
        case POW:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_RK( ind.B );
            printf( "^" );
            FORMAT_RK( ind.C );
            break;
        case UNM:
            FORMAT_REGISTER( ind.A );
            printf( " = -" );
            FORMAT_REGISTER( ind.B );
            break;
        case NOT:
            FORMAT_REGISTER( ind.A );
            printf( " = not " );
            FORMAT_REGISTER( ind.B );
            break;
        case LEN:
            FORMAT_REGISTER( ind.A );
            printf( " = len(" );
            FORMAT_REGISTER( ind.B );
            break;
        case CONCAT:
            {
                FORMAT_REGISTER( ind.A );
                printf( " = " );
                int r;
                for( r = ind.B; r <= ind.C; r++ ) {
                    FORMAT_REGISTER( r );
                    if( r < ind.C )
                        printf( ".." );
                }
            }
            break;
        case JMP:
            printf( "goto %d", order+ind.sBx+1 );
            break;
        case EQ:
            printf( "if " );
            FORMAT_RK( ind.B );
            if( ind.A )
                printf( " ~= " );
            else
                printf( " == " );
            FORMAT_RK( ind.C );
            printf( " then goto %d", order+2 );
            break;
        case LT:
            printf( "if " );
            FORMAT_RK( ind.B );
            if( ind.A )
                printf( " >= " );
            else
                printf( " < " );
            FORMAT_RK( ind.C );
            printf( " then goto %d", order+2 );
            break;
        case LE:
            printf( "if " );
            FORMAT_RK( ind.B );
            if( ind.A )
                printf( " > " );
            else
                printf( " < " );
            FORMAT_RK( ind.C );
            printf( " then goto %d", order+2 );
            break;
        case TEST:
            printf( "if " );
            if( ind.C )
                printf( " not " );
            FORMAT_REGISTER( ind.A );
            printf( " then goto %d", order+2 );
            break;
        case TESTSET:
            printf( "if " );
            if( !ind.C )
                printf( " not " );
            FORMAT_REGISTER( ind.B );
            printf( " then " );
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            FORMAT_REGISTER( ind.B );
            printf( " else goto %d", order+2 );
            break;
        case CALL:
            {
                int r;
                if( ind.C == 0 ) {
                    FORMAT_REGISTER( ind.A );
                    printf( " ... = " );
                }
                else if( ind.C > 1 ) {
                    for( r = ind.A; r <= ind.A+ind.C-2; r++ ) {
                        FORMAT_REGISTER( r );
                        if( r < ind.A+ind.C-2 )
                            printf( ", " );
                    }
                    printf( " = " );
                }
                FORMAT_REGISTER( ind.A );
                printf( "(" );
                if( ind.B == 0 ) {
                    printf( " " );
                    FORMAT_REGISTER( ind.A+1 );
                    printf( " ... " );
                }
                else if( ind.B > 1 ) {
                    printf( " " );
                    for( r = ind.A+1; r <= ind.A+ind.B-1; r++ ) {
                        FORMAT_REGISTER( r );
                        if( r < ind.A+ind.B-1 )
                            printf( ", " );
                    }
                    printf( " " );
                }
                printf( ")" );
            }
            break;
        case TAILCALL:
            printf( "return " );
            {
                int r;
                FORMAT_REGISTER( ind.A );
                printf( "(" );
                if( ind.B == 0 ) {
                    printf( " " );
                    FORMAT_REGISTER( ind.A+1 );
                    printf( " ... " );
                }
                else if( ind.B > 1 ) {
                    printf( " " );
                    for( r = ind.A+1; r <= ind.A+ind.B-1; r++ ) {
                        FORMAT_REGISTER( r );
                        if( r < ind.A+ind.B-1 )
                            printf( ", " );
                    }
                    printf( " " );
                }
                printf( ")" );
            }
            break;
        case RETURN:
            printf( "return " );
            {
                int r;
                if( ind.B == 0 ) {
                    FORMAT_REGISTER( ind.A );
                    printf( " ..." );
                }
                else if( ind.B > 1 ) {
                    for( r = ind.A; r <= ind.A+ind.B-2; r++ ) {
                        FORMAT_REGISTER( r );
                        if( r < ind.A+ind.B-2 )
                            printf( ", " );
                    }
                }
            }
            break;
        case FORLOOP:
            FORMAT_REGISTER( ind.A );
            printf( " += " );
            FORMAT_REGISTER( ind.A+2 );
            printf( "; if " );
            FORMAT_REGISTER( ind.A );
            printf( " <= " );
            FORMAT_REGISTER( ind.A+1 );
            printf( " then " );
            FORMAT_REGISTER( ind.A+3 );
            printf( " = " );
            FORMAT_REGISTER( ind.A );
            printf( "; goto %d", order+ind.sBx+1 );
            break;
        case FORPREP:
            FORMAT_REGISTER( ind.A );
            printf( " -= " );
            FORMAT_REGISTER( ind.A+2 );
            printf( "; goto %d", order+ind.sBx+1 );
            break;
        case TFORLOOP:
            {
                int r;
                for( r = ind.A; r <= ind.A+ind.C+2; r++ ) {
                    FORMAT_REGISTER( r );
                    if( r < ind.A+ind.C+2 )
                        printf( ", " );
                }
                printf( " = " );
                FORMAT_REGISTER( ind.A );
                printf( "( " );
                FORMAT_REGISTER( ind.A+1 );
                printf( ", " );
                FORMAT_REGISTER( ind.A+2 );
                printf( " ); if " );
                FORMAT_REGISTER( ind.A+3 );
                printf( " then " );
                FORMAT_REGISTER( ind.A+2 );
                printf( " = " );
                FORMAT_REGISTER( ind.A+3 );
                printf( " else goto %d", order+2 );
            }
            break;
        case SETLIST:
            FORMAT_REGISTER( ind.A );
            printf( "[( %d )*FPF+i] = r(%d+i), 1 <= i <= %d", ind.C-1, ind.A, ind.B );
            break;
        case CLOSE:
            printf( "pop all above " );
            FORMAT_REGISTER( ind.A );
            break;
        case CLOSURE:
            FORMAT_REGISTER( ind.A );
            printf( " = " );
            printf( "closure( KPROTO[%d] )", ind.Bx );
            break;
        case VARARG:
            {
                int r;
                if( ind.B == 0 ) {
                    FORMAT_REGISTER( ind.A );
                    printf( " ... = ..." );
                }
                else {
                    for( r = ind.A; r <= ind.A+ind.B-2; r++ ) {
                        FORMAT_REGISTER( r );
                        if( r < ind.A+ind.B-2 )
                            printf( ", " );
                    }
                    printf( " = ..." );
                }
            } 
            break;
        default:
            break;
    }
    if( oa->verbose ) {
        printf( "\t\t\t\t" );
        int p = 0;
        switch( ind.desc->type ) {
            case iABC:
                for( p = 0; p < ind.desc->param_num; p++ )
                    printf( "%s ", sABC[p] );
                break;
            case iABx:
                for( p = 0; p < ind.desc->param_num; p++ )
                    printf( "%s ", sABx[p] );
                break;
            case iAsBx:
                for( p = 2-ind.desc->param_num; p < ind.desc->param_num; p++ )
                    printf( "%s ", sAsBx[p] );
                break;
            default:
                break;
        }
        printf( "\t%s\n", ind.desc->desc );
    }
    else
        printf( "\n" );
}

void format_function( FunctionBlock* fb, OptArg* oa, int recursive, int verbose )
{
    int i;
    int tmp_lv;
    if( verbose ) {
        if( fb->source_name.size > 0 ) {
            FORMAT_LEVEL( "source name:\t%s\n", fb->source_name.value );
        }
        else {
            FORMAT_LEVEL( "line defined:\t%d ~ %d\n", fb->first_line, fb->last_line );
        }
        //FORMAT_LEVEL( "number of upvalues:\t%d\n", fb->num_upvalue );
        FORMAT_LEVEL( "number of parameters:\t%d\n", fb->num_parameter );
        FORMAT_LEVEL( "is_vararg flag:\t%d\n", fb->is_vararg );
    }
    FORMAT_LEVEL( "maximum stack size:\t%d\n", fb->max_stack_size );

    if( fb->constant_list.size > 0 ) {
        FORMAT_LEVEL( "constant list:\n" );
        for( i = 0; i < fb->constant_list.size; i++ ) {
            FORMAT_LEVEL( "\t%d.\t", i );
            Constant* c = &fb->constant_list.value[i];
            format_constant( c );
            printf( "\n" );
        }
    }

    if( fb->local_list.size > 0 ) {
        FORMAT_LEVEL( "local list:\n" );
        for( i = 0; i < fb->local_list.size; i++ ) {
            Local* l = &fb->local_list.value[i];
            FORMAT_LEVEL( "\t%d.\t%s\t(%d,%d)\n", i, l->name.value, l->start-1, l->end-1 );
        }
    }

    if( fb->upvalue_list.size > 0 ) {
        FORMAT_LEVEL( "upvalue list:\n" );
        for( i = 0; i < fb->upvalue_list.size; i++ ) {
            String* s = &fb->upvalue_list.value[i];
            FORMAT_LEVEL( "\t%d.\t%s\n", i, s->value );
        }
    }

    FORMAT_LEVEL( "instruction list:\n" );
    int cb_idx = 0;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        CodeBlock* cb = 0;
        if( fb->code_block && cb_idx < fb->num_code_block ) {
            cb = fb->code_block[cb_idx];
            while( !cb && cb_idx < fb->num_code_block-1 ) {
                cb = fb->code_block[++cb_idx];
            }
        }
        if( oa->block && cb && cb->entry == i ) {
            FORMAT_LEVEL( "\tblock. %d\n", cb->id );

            CodeBlock** ppcb;
            int j;
            if( cb->num_pred > 0 ) {
                FORMAT_LEVEL( "\tpredecessors: " );
                ppcb = ( CodeBlock** )cb->predecessors;
                for( j = 0; j < cb->num_pred; j++ ) {
                    printf( "%d, ", ( *ppcb )->id );
                    ppcb++;
                }
                printf( "\n" );
            }
            if( cb->num_succ > 0 ) {
                FORMAT_LEVEL( "\tsuccessors: " );
                CodeBlock** ppcb = ( CodeBlock** )cb->successors;
                for( j = 0; j < cb->num_succ; j++ ) {
                    printf( "%d, ", ( *ppcb )->id );
                    ppcb++;
                }
                printf( "\n" );
            }

            if( oa->optimize && !cb->reachable ) {
                FORMAT_LEVEL( "\t! unreachable, dead code\n" );
            }

            cb_idx++;
        }
        Instruction* in = &fb->instruction_list.value[i];
        format_instruction( fb, in, i, oa );
    }

    if( recursive && fb->num_func > 0 ) {
        FORMAT_LEVEL( "function prototype list:\n" );

        FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
        for( i = 0; i < fb->num_func; i++ ) {
            format_function( pfb, oa, recursive, verbose );
            pfb++;
            printf( "\n" );
        }
    }
}

//--------------------------------------------------
// write functions
//--------------------------------------------------

void write_string( FILE* f, String* str )
{
    fwrite( &str->size, sizeof( size_t ), 1, f );
    fwrite( str->value, 1, str->size, f );
}

void write_instruction( FILE* f, InstructionList* il )
{
    fwrite( &il->size, sizeof( int ), 1, f );
    int i;
    for( i = 0; i < il->size; i++ )
        fwrite( &il->value[i].opcode, sizeof( unsigned int ), 1, f );
}

void write_constant( FILE* f, ConstantList* cl )
{
    fwrite( &cl->size, sizeof( int ), 1, f );
    int i;
    for( i = 0; i < cl->size; i++ ) {
        Constant* c = &cl->value[i];
        fwrite( &c->type, sizeof( unsigned char ), 1, f );
        switch( c->type ) {
            case LUA_TBOOLEAN:
                fwrite( &c->boolean, sizeof( unsigned char ), 1, f );
                break;
            case LUA_TNUMBER:
                fwrite( &c->number, sizeof( double ), 1, f );
                break;
            case LUA_TSTRING:
                write_string( f, &c->string );
                break;
            default:
                break;
        }
    }
}

void write_linepos( FILE* f, InstructionList* il )
{
    if( !il->size )
        return;

    if( il->value[0].line_pos == 0 ) {
        int size = 0;
        fwrite( &size, sizeof( int ), 1, f );
        return;
    }
    else {
        fwrite( &il->size, sizeof( int ), 1, f );
        int i;
        for( i = 0; i < il->size; i++ )
            fwrite( &il->value[i].line_pos, sizeof( int ), 1, f );
    }
}

void write_local( FILE* f, LocalList* ll )
{
    fwrite( &ll->size, sizeof( int ), 1, f );
    int i;
    for( i = 0; i < ll->size; i++ ) {
        Local* l = &ll->value[i];
        write_string( f, &l->name );
        fwrite( &l->start, sizeof( int ), 2, f );
    }
}

void write_upvalue( FILE* f, UpvalueList* ul )
{
    fwrite( &ul->size, sizeof( int ), 1, f );
    int i;
    for( i = 0; i < ul->size; i++ ) {
        String* s = &ul->value[i];
        write_string( f, s );
    }
}

void write_function( FILE* f, FunctionBlock* fb )
{
    write_string( f, &fb->source_name );

    fwrite( &fb->first_line, 1, 12, f );

    write_instruction( f, &fb->instruction_list );
    
    write_constant( f, &fb->constant_list );

    fwrite( &fb->num_func, sizeof( int ), 1, f );
    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    int i;
    for( i = 0; i < fb->num_func; i++, pfb++ )
        write_function( f, pfb );

    write_linepos( f, &fb->instruction_list );
    write_local( f, &fb->local_list );
    write_upvalue( f, &fb->upvalue_list );
}
