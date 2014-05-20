#include "chunk_type.h"

//--------------------------------------------------
// constants
//--------------------------------------------------

const char* sABC[] = { "A", "B", "C" };
const char* sABx[] = { "A", "Bx" };
const char* sAsBx[] = { "A", "sBx" };

InstructionDesc INSTRUCTION_DESC[] = {
    { "MOVE     ", iABC, 2, "R(A) := R(B)" },
    { "LOADK    ", iABx, 2, "R(A) := Kst(Bx)" },
    { "LOADBOOL ", iABC, 3, "R(A) := (Bool)B; if (C) PC++" },
    { "LOADNIL  ", iABC, 2, "R(A) := ... := R(B) := nil" },
    { "GETUPVAL ", iABC, 2, "R(A) := UpValue[B]" },
    { "GETGLOBAL", iABx, 2, "R(A) := Gbl[Kst(Bx)]" },
    { "GETTABLE ", iABC, 3, "R(A) := R(B)[RK(C)]" },
    { "SETGLOBAL", iABx, 2, "Gbl[Kst(Bx)] := R(A)" },
    { "SETUPVAL ", iABC, 2, "UpValue[B] := R(A)" },
    { "SETTABLE ", iABC, 3, "R(A)[RK(B)] := RK(C)" },
    { "NEWTABLE ", iABC, 3, "R(A) := {} (size = B,C)" },
    { "SELF     ", iABC, 3, "R(A+1) := R(B); R(A) := R(B)[RK(C)]" },
    { "ADD      ", iABC, 3, "R(A) := RK(B) + RK(C)" },
    { "SUB      ", iABC, 3, "R(A) := RK(B) ¨C RK(C)" },
    { "MUL      ", iABC, 3, "R(A) := RK(B) * RK(C)" },
    { "DIV      ", iABC, 3, "R(A) := RK(B) / RK(C)" },
    { "MOD      ", iABC, 3, "R(A) := RK(B) % RK(C)" },
    { "POW      ", iABC, 3, "R(A) := RK(B) ^ RK(C)" },
    { "UNM      ", iABC, 2, "R(A) := -R(B)" },
    { "NOT      ", iABC, 2, "R(A) := not R(B)" },
    { "LEN      ", iABC, 2, "R(A) := length of R(B)" },
    { "CONCAT   ", iABC, 3, "R(A) := R(B).. ... ..R(C)" },
    { "JMP      ", iAsBx, 1, "PC += sBx" },
    { "EQ       ", iABC, 3, "if ((RK(B) == RK(C)) ~= A) then PC++" },
    { "LT       ", iABC, 3, "if ((RK(B) < RK(C)) ~= A) then PC++" },
    { "LE       ", iABC, 3, "if ((RK(B) <= RK(C)) ~= A) then PC++" },
    { "TEST     ", iABC, 3, "if not (R(A) <=> C) then PC++" },
    { "TESTSET  ", iABC, 3, "if (R(B) <=> C) then R(A) := R(B) else PC++" },
    { "CALL     ", iABC, 3, "R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))" },
    { "TAILCALL ", iABC, 3, "return R(A)(R(A+1), ... ,R(A+B-1))" },
    { "RETURN   ", iABC, 2, "return R(A), ... ,R(A+B-2)" },
    { "FORLOOP  ", iAsBx, 2, "R(A) -= R(A+2); PC += sBx" },
    { "FORPREP  ", iAsBx, 2, "R(A) += R(A+2);if R(A) <?= R(A+1) then { PC += sBx; R(A+3) = R(A) }" },
    { "TFORLOOP ", iABC, 3, "R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then { R(A+2) = R(A+3); } else { PC++; }" },
    { "SETLIST  ", iABC, 3, "R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B" },
    { "CLOSE    ", iABC, 1, "close all variables in the stack up to (>=) R(A)" },
    { "CLOSURE  ", iABx, 2, "R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))" },
    { "VARARG   ", iABC, 2, "R(A), R(A+1), ..., R(A+B-1) = vararg" },
};

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

void read_function( FILE* f, FunctionBlock* fb, int lv, Summary* smr )
{
    read_string( f, &fb->source_name );
    fread( &fb->first_line, 1, 12, f );

    read_instruction( f, &fb->instruction_list );
    smr->total_instruction_num += fb->instruction_list.size;
    int i;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        unsigned char op = in->opcode & 0x3F;
        smr->instruction_num[op]++;
    }

    read_constant( f, &fb->constant_list );
    fread( &fb->num_func, sizeof( int ), 1, f );
    fb->funcs = realloc( fb->funcs, sizeof( FunctionBlock )*fb->num_func );
    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( i = 0; i < fb->num_func; i++, pfb++ ) {
        memset( pfb, 0, sizeof( FunctionBlock ) );
        read_function( f, pfb, lv+1, smr );
    }
    read_linepos( f, &fb->instruction_list );
    read_local( f, &fb->local_list );
    read_upvalue( f, &fb->upvalue_list );
    fb->level = lv;
}

void get_instruction_detail( Instruction* in, InstructionDetail* ind )
{
    ind->op = in->opcode & 0x3F;
    ind->A = ( in->opcode >> 6 ) & 0xFF;
    ind->desc = &INSTRUCTION_DESC[ind->op];
    switch( ind->desc->type ) {
        case iABC:
            ind->B = in->opcode >> 23;
            ind->C = ( in->opcode >> 14 ) & 0x1FF;
            break;
        case iABx:
            ind->Bx = in->opcode >> 14;
            break;
        case iAsBx:
            ind->sBx = ( in->opcode >> 14 )-0x1FFFF;
            break;
    }
}

void make_instruction( Instruction* in, InstructionDetail* ind )
{
    switch( ind->desc->type ) {
        case iABC:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ind->B << 23 ) | ( ind->C << 14 );
            break;
        case iABx:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ind->Bx << 14 );
            break;
        case iAsBx:
            in->opcode = ind->op | ( ind->A << 6 ) | ( ( ind->sBx+0x1FFFF ) << 14 );
            break;
    }
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

#define FORMAT_LEVEL \
    for( tmp_lv = 0; tmp_lv < fb->level; tmp_lv++ ) \
        printf( "\t" ); \
    printf

#define FORMAT_RK( RK ) \
    if( RK >= 0x100 ) \
        format_constant( &fb->constant_list.value[RK-0x100], 0 ); \
    else \
        printf( "-" );
    
void format_constant( Constant* c, int global )
{
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
            if( global )
                printf( "%s", c->string.value );
            else
                printf( "\"%s\"", c->string.value );
            break;
        default:
            break;
    }
}

void format_instruction( FunctionBlock* fb, Instruction* in, int order, OptArg* oa )
{
    int tmp_lv;
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
    switch( ind.op ) {
        case LOADK:
            printf( "\t; " );
            format_constant( &fb->constant_list.value[ind.Bx], 0 );
            break;
        case GETGLOBAL:
        case SETGLOBAL:
            printf( "\t; " );
            format_constant( &fb->constant_list.value[ind.Bx], 1 );
            break;
        case GETUPVAL:
        case SETUPVAL:
            printf( "\t; %s", fb->upvalue_list.value[ind.B].value );
            break;
        case GETTABLE:
            printf( "\t; " );
            FORMAT_RK( ind.C );
            break;
        case SETTABLE:
            printf( "\t; " );
            FORMAT_RK( ind.B );
            printf( " " );
            FORMAT_RK( ind.C );
            break;
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case MOD:
        case POW:
            printf( "\t; " );
            FORMAT_RK( ind.B );
            printf( " " );
            FORMAT_RK( ind.C );
            break;
        case JMP:
            printf( "\t; to %d", order+ind.sBx+1 );
            break;
        case SELF:
            printf( "\t; " );
            FORMAT_RK( ind.C );
        case EQ:
        case LT:
        case LE:
            printf( "\t; " );
            FORMAT_RK( ind.B );
            printf( " " );
            FORMAT_RK( ind.C );
            break;
        case FORPREP:
        case FORLOOP:
            printf( "\t; to %d", order+ind.sBx+1 );
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

void format_function( FunctionBlock* fb, OptArg* oa )
{
    int i;
    int tmp_lv;
    if( fb->source_name.size > 0 ) {
        FORMAT_LEVEL( "source name:\t%s\n", fb->source_name.value );
    }
    else {
        FORMAT_LEVEL( "line defined:\t%d ~ %d\n", fb->first_line, fb->last_line );
    }
    //FORMAT_LEVEL( "number of upvalues:\t%d\n", fb->num_upvalue );
    FORMAT_LEVEL( "number of parameters:\t%d\n", fb->num_parameter );
    FORMAT_LEVEL( "is_vararg flag:\t%d\n", fb->is_vararg );
    FORMAT_LEVEL( "maximum stack size:\t%d\n", fb->max_stack_size );

    if( fb->constant_list.size > 0 ) {
        FORMAT_LEVEL( "constant list:\n" );
        for( i = 0; i < fb->constant_list.size; i++ ) {
            FORMAT_LEVEL( "\t%d.\t", i );
            Constant* c = &fb->constant_list.value[i];
            format_constant( c, 0 );
            printf( "\n" );
        }
    }

    if( fb->local_list.size > 0 ) {
        FORMAT_LEVEL( "local list:\n" );
        for( i = 0; i < fb->local_list.size; i++ ) {
            Local* l = &fb->local_list.value[i];
            FORMAT_LEVEL( "\t%d.\t%s\t(%d,%d)\n", i, l->name.value, l->start, l->end );
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
        CodeBlock* cb = fb->code_block ? fb->code_block[cb_idx] : 0;
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

    if( fb->num_func > 0 ) {
        FORMAT_LEVEL( "function prototype list:\n" );

        FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
        for( i = 0; i < fb->num_func; i++ ) {
            format_function( pfb, oa );
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
