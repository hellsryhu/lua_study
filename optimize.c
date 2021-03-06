#include <math.h>
#include "utils.h"
#include "rwf.h"
#include "analyze.h"
#include "optimize.h"

extern InstructionDesc INSTRUCTION_DESC[];

//--------------------------------------------------
// control flow analysis functions
//--------------------------------------------------

#define CODE_BLOCK_EXIT( id, seq ) \
    { \
        cb->exit = id; \
        cb->succ_sequential = seq; \
        list_add_tail( &cb->node, &code_block_node ); \
    }

#define CODE_BLOCK_ENTRY( id, seq ) \
    { \
        if( id < fb->instruction_list.size ) { \
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) ); \
            memset( cb, 0, sizeof( CodeBlock ) ); \
            cb->entry = id; \
            cb->pred_sequential = seq; \
        }\
        else \
            cb = 0; \
    }

int is_greater_to( void* a, void* b )
{
    Jump* ja = ( Jump* )a;
    Jump* jb = ( Jump* )b;
    return ja->to > jb->to;
}

int get_code_block( FunctionBlock* fb, int instruction_id )
{
    int i;
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( instruction_id >= cb->entry && instruction_id <= cb->exit )
            return i;
    }
    return -1;
}

void create_successors( FunctionBlock* fb, CodeBlock* cb, int first, int last, Jump** jmps_from )
{
    int* to_blocks = ( int* )malloc( fb->num_code_block*sizeof( int ) );
    memset( to_blocks, 0, fb->num_code_block*sizeof( int ) );

    int i;
    for( i = first; i < last; i++ ) {
        int to_block = get_code_block( fb, jmps_from[i]->to );
        if( to_block >= 0 )
            to_blocks[to_block]++;
    }

    if( cb->succ_sequential )
        to_blocks[cb->id]++;

    int size = 0;
    int* blk = to_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            size++;
    }

    cb->num_succ = size;
    cb->successors = malloc( size*sizeof( CodeBlock* ) );
    CodeBlock** succ = ( CodeBlock** )cb->successors;
    blk = to_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            *succ++ = fb->code_block[i];
    }
    free( to_blocks );
}
 
void create_predecessors( FunctionBlock* fb, CodeBlock* cb, int first, int last, Jump** jmps_to )
{
    int* from_blocks = ( int* )malloc( fb->num_code_block*sizeof( int ) );
    memset( from_blocks, 0, fb->num_code_block*sizeof( int ) );

    int i;
    for( i = first; i < last; i++ ) {
        int from_block = get_code_block( fb, jmps_to[i]->from );
        if( from_block >= 0 )
            from_blocks[from_block]++;
    }

    if( cb->pred_sequential )
        from_blocks[cb->id-2]++;

    int size = 0;
    int* blk = from_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            size++;
    }

    cb->num_pred = size;
    cb->predecessors = malloc( size*sizeof( CodeBlock* ) );
    CodeBlock** pred = ( CodeBlock** )cb->predecessors;
    blk = from_blocks;
    for( i = 0; i < fb->num_code_block; i++, blk++ ) {
        if( *blk > 0 )
            *pred++ = fb->code_block[i];
    }
    free( from_blocks );
}
   
void flow_analysis( FunctionBlock* fb, OptArg* oa )
{
    Jump* jmps = malloc( fb->instruction_list.size*sizeof( Jump ) );
    Jump** jmps_from = malloc( fb->instruction_list.size*sizeof( Jump* ) );
    Jump** jmps_to= malloc( fb->instruction_list.size*sizeof( Jump* ) );
    memset( jmps, 0, fb->instruction_list.size*sizeof( Jump ) );
    memset( jmps_from, 0, fb->instruction_list.size*sizeof( Jump* ) );
    memset( jmps_to, 0, fb->instruction_list.size*sizeof( Jump* ) );
    int jmps_idx = 0;

    // get jumps
    struct list_head code_block_node;
    INIT_LIST_HEAD( &code_block_node );
    CodeBlock* cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
    memset( cb, 0, sizeof( CodeBlock ) );
    int i;
    for( i = 0; i < fb->instruction_list.size; i++ ) {
        Instruction* in = &fb->instruction_list.value[i];
        unsigned char op = GET_OP( in->opcode );
        unsigned short C = GET_C( in->opcode );
        int sBx = GET_SBX( in->opcode );
        switch( op ) {
            // end of block
            case LOADBOOL:
                if( C ) {
                    CODE_BLOCK_EXIT( i, 1 );
                    CODE_BLOCK_ENTRY( i+1, 1 );
                    jmps[jmps_idx].from = i;
                    jmps[jmps_idx++].to = i+2;
                }
                break;
            case JMP:
                CODE_BLOCK_EXIT( i, 0 );
                CODE_BLOCK_ENTRY( i+1, 0 );
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+sBx+1;
                break;
            case FORLOOP:
                CODE_BLOCK_EXIT( i, 1 );
                CODE_BLOCK_ENTRY( i+1, 1 );
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+1;
                break;
            case RETURN:
            case TAILCALL:
                CODE_BLOCK_EXIT( i, 0 );
                CODE_BLOCK_ENTRY( i+1, 0 );
                break;
            // start of block
            case FORPREP:
                if( i != 0 ) {
                    CODE_BLOCK_EXIT( i-1, 1 );
                    CODE_BLOCK_ENTRY( i, 1 );
                    jmps[jmps_idx].from = i-1;
                    jmps[jmps_idx++].to = i;
                }
                break;
            // must with jmp
            case TFORLOOP:
            case EQ:
            case LT:
            case LE:
            case TEST:
            case TESTSET:
                jmps[jmps_idx].from = i;
                jmps[jmps_idx++].to = i+2;
                break;
            default:
                break;
        }
    }
    if( cb ) {
        cb->exit = fb->instruction_list.size-1;
        list_add_tail( &cb->node, &code_block_node );
    }

    for( i = 0; i < jmps_idx; i++ ) {
        jmps_from[i] = &jmps[i];
        jmps_to[i] = &jmps[i];
    }
    quick_sort( ( void** )jmps_to, 0, jmps_idx-1, is_greater_to );

    // complete blocks
    struct list_head* pos = code_block_node.next;
    cb = list_entry( pos, CodeBlock, node );
    i = 0;
    while( i < jmps_idx ) {
        if( jmps_to[i]->to == cb->entry )
            i++;
        else if( jmps_to[i]->to <= cb->exit ) {
            CodeBlock* ocb = cb;
            cb = ( CodeBlock* )malloc( sizeof( CodeBlock ) );
            memset( cb, 0, sizeof( CodeBlock ) );
            cb->entry = jmps_to[i]->to;
            cb->exit = ocb->exit;
            cb->pred_sequential = 1;
            cb->succ_sequential = ocb->succ_sequential;

            ocb->exit = jmps_to[i]->to-1;
            ocb->succ_sequential = 1;
            list_add( &cb->node, &ocb->node );

            pos = &cb->node;

            i++;
        }
        else if( jmps_to[i]->to > cb->exit ) {
            pos = pos->next;
            if( pos != &code_block_node ) {
                cb = list_entry( pos, CodeBlock, node );
            }
            else
                break;
        }
    }

    i = 0;
    list_for_each( pos, &code_block_node ) {
        cb = list_entry( pos, CodeBlock, node );
        cb->id = ++i;
    }

    fb->num_code_block = i;
    fb->code_block = ( CodeBlock** )malloc(i*sizeof( CodeBlock* ) );
    CodeBlock** ppcb = fb->code_block;
    list_for_each( pos, &code_block_node )
        *ppcb++ = list_entry( pos, CodeBlock, node );

    // gen successor
    int j = 0;
    cb = fb->code_block[j];
    i = 0;
    int first = 0;
    while( i < jmps_idx ) {
        if( jmps_from[i]->from <= cb->exit )
            i++;
        else {
            create_successors( fb, cb, first, i, jmps_from );

            first = i;

            cb = fb->code_block[++j];
        }
    }
    create_successors( fb, cb, first, i, jmps_from );

    for( i = j+1; i < fb->num_code_block; i++ ) {
        cb = fb->code_block[i];
        create_successors( fb, cb, 0, -1, jmps_from );
    }

    // gen predecessor
    j = 0;
    cb = fb->code_block[j];
    i = 0;
    first = 0;
    while( i < jmps_idx ) {
        if( jmps_to[i]->to <= cb->exit )
            i++;
        else {
            create_predecessors( fb, cb, first, i, jmps_to );

            first = i;

            cb = fb->code_block[++j];
        }
    }
    create_predecessors( fb, cb, first, i, jmps_to );

    for( i = j+1; i < fb->num_code_block; i++ ) {
        cb = fb->code_block[i];
        create_predecessors( fb, cb, 0, -1, jmps_to );
    }

    free( jmps );
    free( jmps_from );
    free( jmps_to );

    FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
    for( i = 0; i < fb->num_func; i++ ) {
        flow_analysis( pfb, oa );
        pfb++;
    }
}

//--------------------------------------------------
// instruction edit
//--------------------------------------------------

// delete from~to, insert into from
void modify_instruction( FunctionBlock* fb, CodeBlock* cb, int from, int to, Instruction* in, int size )
{
    if( from < cb->entry || from > cb->exit ) return;
    if( to < cb->entry || to > cb->exit ) return;

    int mod_cnt = size;
    if( to >= from )
        mod_cnt += from-to-1;

    // update jmp
    int i;
    Instruction *tmp_in;
    for( i = 0, tmp_in = fb->instruction_list.value; i < fb->instruction_list.size; i++, tmp_in++ ) {
        if( i >= from && i <= to ) continue;

        InstructionDetail ind;
        get_instruction_detail( tmp_in, &ind );
        switch( ind.op ) {
            case JMP:
            case FORLOOP:
            case FORPREP:
                { 
                    int jmp_to = i+ind.sBx+1;
                    if( i < from ) {
                        if( jmp_to > to )
                            jmp_to += mod_cnt;
                        else if( jmp_to >= from )
                            jmp_to = from;
                    }
                    else if( i > to ) {
                        if( jmp_to < from )
                            jmp_to += mod_cnt;
                        else if( jmp_to <= to )
                            jmp_to = to+1;
                    }
                    ind.sBx = jmp_to-i-1;
                }
                make_instruction( tmp_in, &ind );
                break;
            default:
                break;
        }
    }

    // update instruction list
    int old_size = fb->instruction_list.size;
    fb->instruction_list.size += mod_cnt;
    Instruction* new_insts = malloc( sizeof( Instruction )*fb->instruction_list.size );
    if( from > 0 )
        memcpy( new_insts, fb->instruction_list.value, sizeof( Instruction )*from );
    int rem = old_size-to-1;
    if( rem )
        memcpy( new_insts+from+size, fb->instruction_list.value+to+1, sizeof( Instruction )*rem );
    Instruction* ni = &new_insts[from];
    for( i = 0; i < size; i++, ni++, in++ ) {
        ni->opcode = in->opcode;
        ni->line_pos = in->line_pos;
        ni->hint = 0;
    }
    free( fb->instruction_list.value );
    fb->instruction_list.value = new_insts;

    // update local scope
    int unuse_cnt = 0;
    Local *l;
    for( i = 0, l = fb->local_list.value; i < fb->local_list.size; i++, l++ ) {
        if( l->end < from )
            continue;
        else if( l->start > to ) {
            l->start += mod_cnt;
            l->end += mod_cnt;
        }
        else if( l->start < from ) {
            if( l->end <= to )
                l->end = from+size;
            else
                l->end += mod_cnt;
        }
        else {
            if( l->end <= to ) {
                // unused local
                unuse_cnt++;
                l->start = l->end = -1;
            }
            else {
                l->start = from;
                l->end += mod_cnt;
            }
        }
    }
    // remove unused locals
    if( unuse_cnt > 0 ) {
        fb->local_list.size = fb->local_list.size-unuse_cnt;
        Local* new_locals = malloc( sizeof( Local )*( fb->local_list.size ) );
        Local* nl = new_locals;
        for( i = 0, l = fb->local_list.value; i < fb->local_list.size; i++, l++ ) {
            if( l->start != -1 && l->end != -1 ) {
                memcpy( nl, l, sizeof( Local ) );
                nl++;
            }
        }
        free( fb->local_list.value );
        fb->local_list.value = new_locals;
    }

    // update code block
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            if( cb->exit >= to )
                cb->exit += mod_cnt;
            if( cb->entry > from )
                cb->entry += mod_cnt;
        }
    }

    reset_stack_frames( fb );
}

void delete_instruction( FunctionBlock* fb, CodeBlock* cb, int from, int to )
{
    modify_instruction( fb, cb, from, to, 0, 0 );
}

void insert_instruction( FunctionBlock* fb, CodeBlock* cb, int pos, Instruction* in, int size )
{
    modify_instruction( fb, cb, pos, -1, in, size );
}

int append_constant( FunctionBlock* fb, Constant* c )
{
    int i;
    Constant* pc;
    for( i = 0, pc = fb->constant_list.value; i < fb->constant_list.size; i++, pc++ ) {
        if( c->type == pc->type ) {
            if( c->type == LUA_TSTRING ) {
                if( is_same_string( &c->string, &pc->string ) )
                    return i;
            }
            else {
                if( c->number == pc->number )
                    return i;
            }
        }
    }

    int new_size = fb->constant_list.size+1;
    fb->constant_list.size = new_size;
    fb->constant_list.value = realloc( fb->constant_list.value, new_size*sizeof( Constant ) );
    fb->constant_list.value[new_size-1] = *c;
    return new_size-1;
}

void neaten_constant_list( FunctionBlock* fb )
{
    // get constant usage
    int i;
    Instruction* in;
    char* usage = malloc( fb->constant_list.size );
    memset( usage, 0, fb->constant_list.size );
    for( i = 0, in = fb->instruction_list.value; i < fb->instruction_list.size; i++, in++ ) {
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        switch( ind.op ) {
            case LOADK:
            case GETGLOBAL:
            case SETGLOBAL:
                usage[ind.Bx] = 1;
                break;
            case SETTABLE:
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case MOD:
            case POW:
            case EQ:
            case LT:
            case LE:
                if( IS_CONST( ind.B ) )
                    usage[ind.B-CONST_BASE] = 1;
            case GETTABLE:
            case SELF:
                if( IS_CONST( ind.C ) )
                    usage[ind.C-CONST_BASE] = 1;
                break;
            default:
                break;
        }
    }

    int use_cnt = 0;
    char* u;
    for( i = 0, u = usage; i < fb->constant_list.size; i++, u++ ) {
        if( *u )
            use_cnt++;
    }

    if( use_cnt < fb->constant_list.size ) {
        // realloc consts
        Constant* consts = malloc( use_cnt*sizeof( Constant ) );

        Constant* c;
        Constant* nc = consts;
        int new_idx = 0;
        for( i = 0, u = usage, c = fb->constant_list.value; i < fb->constant_list.size; i++, u++, c++ ) {
            if( *u ) {
                *nc++ = *c;
                *u = new_idx++;
            }
            else if( c->type == LUA_TSTRING )
                free( c->string.value );
        }

        fb->constant_list.size = use_cnt;
        free( fb->constant_list.value );
        fb->constant_list.value = consts;

        // update instructions
        for( i = 0, in = fb->instruction_list.value; i < fb->instruction_list.size; i++, in++ ) {
            InstructionDetail ind;
            get_instruction_detail( in, &ind );
            switch( ind.op ) {
                case LOADK:
                case GETGLOBAL:
                case SETGLOBAL:
                    ind.Bx = usage[ind.Bx];
                    break;
                case SETTABLE:
                case ADD:
                case SUB:
                case MUL:
                case DIV:
                case MOD:
                case POW:
                case EQ:
                case LT:
                case LE:
                    if( IS_CONST( ind.B ) )
                        ind.B = usage[ind.B-CONST_BASE]+CONST_BASE;
                case GETTABLE:
                case SELF:
                    if( IS_CONST( ind.C ) )
                        ind.C = usage[ind.C-CONST_BASE]+CONST_BASE;
                    break;
                default:
                    break;
            }
            make_instruction( in, &ind );
        }
    }

    free( usage );
}

//--------------------------------------------------
// optimize functions
//--------------------------------------------------

int dce_traverse( void* node )
{
    CodeBlock* cb = ( CodeBlock* )node;
    if( cb->reachable )
        return 0;
    cb->reachable = 1;
    return 1;
}

void* dce_iterator( void* node, void* next )
{
    CodeBlock* cb = ( CodeBlock* )node;
    CodeBlock** succ = cb->successors;
    if( next ) {
        int i;
        for( i = 0; i < cb->num_succ-1; i++, succ++ ) {
            if( next == *succ )
                return *( succ+1 );
        }
        return 0;
    }
    else
        return cb->num_succ > 0 ? *succ : 0;
}

int dead_code_elimination( FunctionBlock* fb, OptArg* oa )
{
    CodeBlock* cb = fb->code_block[0];
    dfs( cb, dce_traverse, dce_iterator );

    if( oa->hint ) return 0;

    // beware!!! only set a null pos
    int i;
    int ret = 0;
    CodeBlock** ppcb = fb->code_block;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        cb = *ppcb;
        if( !cb->reachable ) {
            delete_instruction( fb, cb, cb->entry, cb->exit );

            *ppcb = 0;

            free( cb );

            ret++;
        }
    }
    return ret;
}

int is_same_reg( ValueInfo* vi, int reg )
{
    switch( vi->type ) {
        case VT_REGISTER:
            if( vi->val[0] == reg )
                return 1;
            break;
        case VT_2_REGISTER:
            if( vi->val[0] == reg || vi->val[1] == reg )
                return 1;
            break;
        case VT_RANGE_REGISTER:
            if( vi->val[0] <= reg && vi->val[1] >= reg )
                return 1;
            break;
        default:
            return 0;
    }
    return 0;
}

int get_register_depend( CodeBlock* cb, int reg, int order, int lval )
{
    int i = order-1;
    InstructionContext* ic = &cb->instruction_context[i-cb->entry];
    for( ; i >= cb->entry; i--, ic-- ) {
        if( is_same_reg( &ic->lval, reg ) )
            return i;
        if( lval && is_same_reg( &ic->rval, reg ) )
            return i;
    }
    return -1;
}

int get_upvalue_depend( CodeBlock* cb, int uv, int order, int lval )
{
    int i = order-1;
    InstructionContext* ic = &cb->instruction_context[i];
    for( ; i >= cb->entry; i--, ic-- ) {
        if( ic->lval.type == VT_UPVALUE && ic->lval.val[0] == uv )
            return i;
        if( lval && ic->rval.type == VT_UPVALUE && ic->rval.val[0] == uv )
            return i;
    }
    return -1;
}

int is_same_table_elem( Constant* consts, ValueInfo* vi, int reg, int rk_key, int lval )
{
    switch( vi->type ) {
        case VT_GLOBAL:
            if( rk_key < 0 ) {
                Constant* c = &consts[vi->val[0]];
                if( c->type == LUA_TNUMBER && c->number == -rk_key )    // same key
                    return 1;
            }
            else if( IS_CONST( rk_key ) && rk_key-CONST_BASE == vi->val[0] )
                return 1;
            break;
        case VT_TABLE:
            if( rk_key < 0 && IS_CONST( vi->val[1] ) ) {
                Constant* c = &consts[vi->val[1]-CONST_BASE];
                if( c->type == LUA_TNUMBER && c->number == -rk_key )    // same key
                    return 1;
            }
            else if( IS_CONST( rk_key ) && rk_key == vi->val[1] )
                return 1;
            break;
        case VT_TABLE_RANGE:
            if( rk_key < 0 ) {
                int start = vi->val[1];
                int end = start+( vi->val[0] & 0xFFFF );
                if( -rk_key >= start && -rk_key <= end )
                    return 1;
            }
            else if( IS_CONST( rk_key ) ) {
                Constant* c = &consts[rk_key];
                int start = vi->val[1];
                int end = start+( vi->val[0] & 0xFFFF );
                if( c->type == LUA_TNUMBER && c->number >= start && c->number <= end )    // same key
                    return 1;
            }
            break;
        case VT_REGISTER:
            if( lval && ( vi->val[0] == reg ) )
                return 1;
            break;
        case VT_2_REGISTER:
            if( lval && ( reg == vi->val[0] || reg == vi->val[1] ) )
                return 1;
            break;
        case VT_RANGE_REGISTER:
            if( lval && ( reg >= vi->val[0] && reg <= vi->val[1] ) )
                return 1;
            break;
        default:
            return 0;
    }
    return 0;
}

// different register may save same table
// different register may save same key
// so we must consider table depends closest table changes
int get_table_depend( FunctionBlock* fb, CodeBlock* cb, int reg, int key, int order, int lval )
{
    int i = order-1;
    InstructionContext* ic = &cb->instruction_context[i];
    for( ; i >= cb->entry; i--, ic-- ) {
        if( is_same_table_elem( fb->constant_list.value, &ic->lval, reg, key, 1 ) )
            return i;
        if( lval && is_same_table_elem( fb->constant_list.value, &ic->rval, reg, key, 0 ) )
            return i;
    }
    return -1;
}

int get_return_depend( FunctionBlock* fb, CodeBlock* cb, int order )
{
    int i = order-1;
    Instruction* in = &fb->instruction_list.value[i];
    for( ; i >= cb->entry; i--, in-- ) {
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        if( ind.op == SETGLOBAL || ind.op == SETTABLE || ind.op == SETUPVAL || ind.op == CALL || ind.op == SETLIST )
            return i;
    }
    return -1;
}

#define VALUE_INFO( vi, vt, val0, val1 ) \
    ( vi ).type = ( vt ); \
    ( vi ).val[0] = ( val0 ); \
    ( vi ).val[1] = ( val1 );

#define ALLOC_DEPENDS( size ) \
    { \
        ic->num_depend = ( size ); \
        ic->depends = ( size ) > 0 ? malloc( sizeof( int )*( size ) ) : 0; \
    }

#define DEPEND( idx, line ) \
    { \
        int nline = line; \
        ic->depends[idx] = nline; \
        if( nline >= 0 ) { \
            InstructionContext* dic = &cb->instruction_context[nline-cb->entry]; \
            dic->num_dependent++; \
        } \
    }

#define RK_DEPEND( RK, idx, lval ) \
    if( IS_CONST( RK ) ) \
        DEPEND( idx, -1 ) \
    else \
        DEPEND( idx, get_register_depend( cb, ( RK ), i, lval ) )
     
void mark_level( CodeBlock* cb, int order, int lv, int* levels )
{
    int cur_lv = levels[order-cb->entry];
    if( cur_lv < lv )
        levels[order-cb->entry] = lv;

    int k = 0;
    InstructionContext* ic = &cb->instruction_context[order-cb->entry];
    int* pdp = ic->dependents;
    for( ; k < ic->num_dependent; k++, pdp++ ) {
        int dp = *pdp;
        mark_level( cb, dp, lv+1, levels );
    }
}

int is_greater_order( void** varray, int index_a, int index_b )
{
    int a = ( int )varray[index_a];
    int b = ( int )varray[index_b];
    if( a > b )
        return 1;
    else if( a < b )
        return 0;
    else
        return index_a > index_b;
}
   

void create_instruction_context( FunctionBlock* fb, CodeBlock* cb )
{
    cb->instruction_context = malloc( sizeof( InstructionContext )*CODE_BLOCK_LEN( cb ) );
    memset( cb->instruction_context, 0, sizeof( InstructionContext )*CODE_BLOCK_LEN( cb ) );
    
    // get affects
    int i = cb->entry;
    Instruction* in = &fb->instruction_list.value[i];
    InstructionContext* ic = &cb->instruction_context[i-cb->entry];
    for( ; i <= cb->exit; i++, in++, ic++ ) {
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        switch( ind.op ) {
            case MOVE:
            case UNM:
            case NOT:
            case LEN:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_REGISTER, ind.B, 0 );
                break;
            case LOADK:
            case LOADBOOL:
            case NEWTABLE:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                break;
            case LOADNIL:
                VALUE_INFO( ic->lval, VT_RANGE_REGISTER, ind.A, ind.B );
                break;
            case GETUPVAL:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_UPVALUE, ind.B, 0 );
                break;
            case GETGLOBAL:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_GLOBAL, ind.Bx, 0 );
                break;
            case GETTABLE:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_TABLE, ind.B, ind.C );
                break;
            case SETGLOBAL:
                VALUE_INFO( ic->lval, VT_GLOBAL, ind.Bx, 0 );
                VALUE_INFO( ic->rval, VT_REGISTER, ind.A, 0 );
                break;
            case SETUPVAL:
                VALUE_INFO( ic->lval, VT_UPVALUE, ind.B, 0 );
                VALUE_INFO( ic->rval, VT_REGISTER, ind.A, 0 );
                break;
            case SETTABLE:
                VALUE_INFO( ic->lval, VT_TABLE, ind.A, ind.B );
                VALUE_INFO( ic->rval, VT_2_REGISTER, ind.A, ind.C );
                break;
            case SELF:
                VALUE_INFO( ic->lval, VT_2_REGISTER, ind.A, ind.A+1 );
                VALUE_INFO( ic->rval, VT_TABLE, ind.B, ind.C );
                break;
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case MOD:
            case POW:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_2_REGISTER, ind.B, ind.C );
                break;
            case CONCAT:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.B, ind.C );
                break;
            case EQ:
            case LT:
            case LE:
                VALUE_INFO( ic->rval, VT_2_REGISTER, ind.B, ind.C );
                break;
            case TEST:
                VALUE_INFO( ic->rval, VT_REGISTER, ind.A, 0 );
                break;
            case TESTSET:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                VALUE_INFO( ic->rval, VT_REGISTER, ind.B, 0 );
                break;
            case CALL:
                VALUE_INFO( ic->lval, VT_RANGE_REGISTER, ind.A, ind.A+ind.C-2 );
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+ind.B-1 );
                break;
            case TAILCALL:
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+ind.B-1 );
                break;
            case RETURN:
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+ind.B-2 );
                break;
            case FORLOOP:
                VALUE_INFO( ic->lval, VT_2_REGISTER, ind.A, ind.A+3 );
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+2 );
                break;
            case FORPREP:
                VALUE_INFO( ic->lval, VT_2_REGISTER, ind.A, ind.A+2 );
                VALUE_INFO( ic->rval, VT_REGISTER, ind.A, 0 );
                break;
            case TFORLOOP:
                VALUE_INFO( ic->lval, VT_RANGE_REGISTER, ind.A+2, ind.A+ind.C+2 );
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+3 );
                break;
            case SETLIST:
                VALUE_INFO( ic->lval, VT_TABLE_RANGE, ( ind.A << 16 )+ind.B, ( ind.C-1 )*FPF );
                VALUE_INFO( ic->rval, VT_RANGE_REGISTER, ind.A, ind.A+ind.B );
                break;
            case CLOSURE:
                VALUE_INFO( ic->lval, VT_REGISTER, ind.A, 0 );
                break;
            case VARARG:
                VALUE_INFO( ic->lval, VT_RANGE_REGISTER, ind.A, ind.A+ind.B-1 );
                VALUE_INFO( ic->rval, VT_VARARG, 0, 0 );
                break;
            default:
                break;
        }
    }

    // get depends
    i = cb->entry;
    in = &fb->instruction_list.value[i];
    ic = &cb->instruction_context[0];
    int closure_idx = 0;
    int closure_cnt = 0;
    for( ; i <= cb->exit; i++, in++, ic++ ) {
        InstructionDetail ind;
        get_instruction_detail( in, &ind );
        switch( ind.op ) {
            case MOVE:
                if( closure_cnt > 0 ) {
                    ALLOC_DEPENDS( 1 );
                    closure_cnt--;
                    DEPEND( 0, closure_idx );
                }
                else {
                    ALLOC_DEPENDS( 2 );
                    DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                    DEPEND( 1, get_register_depend( cb, ind.B, i, 0 ) );
                }
                break;
            case LOADK:
            case LOADBOOL:
            case NEWTABLE:
                ALLOC_DEPENDS( 1 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                break;
            case LOADNIL:
                ALLOC_DEPENDS( ind.B-ind.A+1 );
                {
                    int r = ind.A;
                    int j = 0;
                    for( ; r <= ind.B; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 1 ) )
                }
                break;
            case GETUPVAL:
                if( closure_cnt > 0 ) {
                    ALLOC_DEPENDS( 1 );
                    closure_cnt--;
                    DEPEND( 0, closure_idx );
                }
                else {
                    ALLOC_DEPENDS( 2 );
                    DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                    DEPEND( 1, get_upvalue_depend( cb, ind.B, i, 0 ) );
                }
                break;
            case GETGLOBAL:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_table_depend( fb, cb, -1, ind.Bx+CONST_BASE, i, 0 ) );
                break;
            case GETTABLE:
                ALLOC_DEPENDS( 3 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_table_depend( fb, cb, ind.B, ind.C, i, 0 ) );
                RK_DEPEND( ind.C, 2, 0 );
                break;
            case SETGLOBAL:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 0 ) );
                DEPEND( 1, get_table_depend( fb, cb, -1, ind.Bx+CONST_BASE, i, 1 ) );
                break;
            case SETUPVAL:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 0 ) );
                DEPEND( 1, get_upvalue_depend( cb, ind.B, i, 1 ) );
                break;
            case SETTABLE:
                ALLOC_DEPENDS( 3 );
                DEPEND( 0, get_table_depend( fb, cb, ind.A, ind.B, i, 1 ) );
                DEPEND( 1, get_register_depend( cb, ind.B, i, 0 ) );
                RK_DEPEND( ind.C, 2, 0 );
                break;
            case SELF:
                ALLOC_DEPENDS( 4 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_register_depend( cb, ind.A+1, i, 1 ) );
                DEPEND( 2, get_table_depend( fb, cb, ind.B, ind.C, i, 0 ) );
                RK_DEPEND( ind.C, 3, 0 );
                break;
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case MOD:
            case POW:
                ALLOC_DEPENDS( 3 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                RK_DEPEND( ind.B, 1, 0 );
                RK_DEPEND( ind.C, 2, 0 );
                break;
            case UNM:
            case NOT:
            case LEN:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                RK_DEPEND( ind.B, 1, 0 );
                break;
            case CONCAT:
                ALLOC_DEPENDS( ind.C-ind.B+2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                {
                    int j = 1;
                    int r = ind.B;
                    for( ; r <= ind.C; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) )
                }
                break;
            case JMP:
                ALLOC_DEPENDS( 1 );
                DEPEND( 0, i-1 < cb->entry ? -1 : i-1 );
                break;
            case EQ:
            case LT:
            case LE:
                ALLOC_DEPENDS( 2 );
                RK_DEPEND( ind.B, 0, 0 );
                RK_DEPEND( ind.C, 1, 0 );
                break;
            case TEST:
                ALLOC_DEPENDS( 1 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 0 ) );
                break;
            case TESTSET:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_register_depend( cb, ind.B, i, 0 ) );
                break;
            case CALL:
                {
                    int size = 0;
                    if( ind.C > 1 )
                        size += ind.C-1;
                    if( ind.B > 1 )
                        size += ind.B;
                    ALLOC_DEPENDS( size );
                    int j = 0;
                    int r = ind.A;
                    for( ; r < ind.A+ind.C-1; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 1 ) )
                    r = ind.A;
                    for( ; r < ind.A+ind.B; j++, r++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) )
                }
                break;
            case TAILCALL:
                {
                    int size = 2;
                    if( ind.B > 1 )
                        size += ind.B;
                    ALLOC_DEPENDS( size );
                    DEPEND( 0, get_return_depend( fb, cb, i ) );
                    DEPEND( 1, get_register_depend( cb, ind.A, i, 0 ) );
                    int j = 2;
                    int r = ind.A+1;
                    for( ; r < ind.A+ind.B; j++, r++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) )
                }
                break;
            case RETURN:
                {
                    int size = 1;
                    if( ind.B > 1 )
                        size += ind.B-1;
                    ALLOC_DEPENDS( size );
                    DEPEND( 0, get_return_depend( fb, cb, i ) );
                    int j = 1;
                    int r = ind.A;
                    for( ; r < ind.A+ind.B-1; j++, r++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) )
                }
                break;
            case FORLOOP:
                ALLOC_DEPENDS( 4 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_register_depend( cb, ind.A+3, i, 1 ) );
                DEPEND( 2, get_register_depend( cb, ind.A+1, i, 0 ) );
                DEPEND( 3, get_register_depend( cb, ind.A+2, i, 0 ) );
                break;
            case FORPREP:
                ALLOC_DEPENDS( 2 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                DEPEND( 1, get_register_depend( cb, ind.A+2, i, 0 ) );
                break;
            case TFORLOOP:
                ALLOC_DEPENDS( ind.C+4 );
                {
                    int j = 0;
                    int r = ind.A+2;
                    for( ; r <= ind.A+ind.C+2; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 1 ) )
                    r = ind.A;
                    for( ; r <= ind.A+3; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) )
                }
                break;
            case SETLIST:
                ALLOC_DEPENDS( ( ind.B<<1 )+1 );
                {
                    int j = 0;
                    int r = 1;
                    for( ; r <= ind.B; j++, r++ )
                        DEPEND( j, get_table_depend( fb, cb, ind.A, -( ( ind.C-1 )*FPF+r ), i, 1 ) );
                    r = ind.A;
                    for( ; r <= ind.A+ind.B; j++, r++ )
                        DEPEND( j, get_register_depend( cb, r, i, 0 ) );
                }
                break;
            case CLOSURE:
                ALLOC_DEPENDS( 1 );
                DEPEND( 0, get_register_depend( cb, ind.A, i, 1 ) );
                closure_idx = i;
                {
                    FunctionBlock* sub_fb = &( ( FunctionBlock* )( fb->funcs ) )[ind.Bx];
                    closure_cnt = sub_fb->upvalue_list.size;
                }
                break;
            case VARARG:
                ALLOC_DEPENDS( ind.B );
                {
                    int j = 0;
                    int r = ind.A;
                    for( ; r < ind.B; r++, j++ )
                        DEPEND( j, get_register_depend( cb, r, i, 1 ) );
                }
                break;
            default:
                break;
        }
    }

    // get dependents
    int** cursors = malloc( sizeof( int* )*CODE_BLOCK_LEN( cb ) );
    i = cb->entry;
    ic = &cb->instruction_context[0];
    for( ; i <= cb->exit; i++, ic++ ) {
        if( ic->num_dependent > 0 ) {
            int size = sizeof( int )*( ic->num_dependent );
            ic->dependents = malloc( sizeof( int )*size );
        }
        else
            ic->dependents = 0;
        cursors[i-cb->entry] = ic->dependents;
    }
    i = cb->entry;
    ic = &cb->instruction_context[0];
    for( ; i <= cb->exit; i++, ic++ ) {
        int j = 0;
        int* pdp = ic->depends;
        for( ; j < ic->num_depend; j++, pdp++ ) {
            int dp = *pdp;
            if( dp != -1 ) {
                dp -= cb->entry;
                *( cursors[dp] ) = i;
                cursors[dp]++;
            }
        }
    }
    free( cursors );

    // get execute levels
    cb->exe_levels = malloc( sizeof( int )*CODE_BLOCK_LEN( cb ) );
    memset( cb->exe_levels, 0, sizeof( int )*CODE_BLOCK_LEN( cb ) );

    for( i = cb->entry; i <= cb->exit; i++, ic++ )
        mark_level( cb, i, 1, cb->exe_levels );

    cb->exe_order = malloc( sizeof( int )*CODE_BLOCK_LEN( cb ) );
    int* po = cb->exe_order;
    for( i = cb->entry; i <= cb->exit; i++, po++ )
        *po = i;
    quick_sort2( cb->exe_order, cb->entry, ( void** )cb->exe_levels, 0, cb->exit-cb->entry, is_greater_order );
}

int directed_acyclic_graph( FunctionBlock* fb, CodeBlock* cb )
{
    return 0;
}

#define IS_LOCAL( R, iid ) ( fb->stack_frames ? fb->stack_frames[iid].slots[R] != -1 : 0 )

#define OPT_CF_ARITH            0x01
#define OPT_CF_HIGH_PRIORITY    0x02
#define OPT_CF_COMMUTABLE       0x04
#define OPT_CF_MERGE_C1         0x01    // 1st op is commutable
#define OPT_CF_MERGE_BR1        0x02    // 1st op register is B
#define OPT_CF_MERGE_C2         0x04    // 2nd op is commutable
#define OPT_CF_MERGE_BR2        0x08    // 2nd op register is B

#define OPT_CF_CHECK_FROM \
    if( opt_from != -1 ) { \
        opt_to = j-1; \
        if( opt_to > opt_from && !oa->hint ) { \
            Instruction tmp_in; \
            make_instruction( &tmp_in, &prev ); \
            modify_instruction( fb, cb, opt_from, opt_to, &tmp_in, 1 ); \
            j -= ( opt_to-opt_from ); \
            opt++; \
        } \
        opt_from = -1; \
        prev_flag = 0; \
    }

#define OPT_CF_COMMUTE_OP( op ) ( ( op == ADD || op == SUB ) ? ADD : MUL )
#define OPT_CF_NCOMMUTE_OP( op ) ( ( op == ADD || op == SUB ) ? SUB : DIV )

void constant_binary_arith_op( Constant* c1, Constant *c2, int op )
{
    if( c1->type != LUA_TNUMBER || c2->type != LUA_TNUMBER ) return;

    switch( op ) {
        case ADD:
            c1->number += c2->number;
            break;
        case SUB:
            c1->number -= c2->number;
            break;
        case MUL:
            c1->number *= c2->number;
            break;
        case DIV:
            c1->number /= c2->number;
            break;
        case MOD:
            c1->number = c1->number-floor( c1->number/c2->number )*c2->number;
            break;
        case POW:
            c1->number = pow( c1->number, c2->number );
            break;
        default:
            break;
    }
}

int constant_folding( FunctionBlock* fb, OptArg* oa )
{
    if( !oa->constant_folding ) return 0;

    int opt = 0;
    CodeBlock** ppcb = fb->code_block;
    int i;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            InstructionDetail curr, prev;
            char prev_flag = 0;
            int opt_from = -1, opt_to;
            int j;
            for( j = cb->entry; j <= cb->exit; j++ ) {
                Instruction* in = &fb->instruction_list.value[j];
                get_instruction_detail( in, &curr );

                char flag = 0;
                switch( curr.op ) {
                    case ADD:
                        flag = OPT_CF_ARITH | OPT_CF_COMMUTABLE;
                        break;
                    case SUB:
                        flag = OPT_CF_ARITH;
                        break;
                    case MUL:
                        flag = OPT_CF_ARITH | OPT_CF_HIGH_PRIORITY | OPT_CF_COMMUTABLE;
                        break;
                    case DIV:
                        flag = OPT_CF_ARITH | OPT_CF_HIGH_PRIORITY;
                        break;
                    default:
                        break;
                }

                // is arith op, has const param
                if( flag && ( IS_CONST( curr.B ) || IS_CONST( curr.C ) ) ) {
                    // prev is an optimizable op
                    // same priority
                    // prev output reg is curr input reg
                    // prev output reg is an temporary slot or prev output reg is curr output reg
                    if( prev_flag 
                        && ( prev_flag & OPT_CF_HIGH_PRIORITY ) == ( flag & OPT_CF_HIGH_PRIORITY )
                        && ( prev.A == curr.B || prev.A == curr.C )
                        && ( !IS_LOCAL( prev.A, j-1 ) || prev.A == curr.A ) ) {
                        if( oa->hint )
                            in->hint |= HINT_CONSTANT_FOLDING;
                        else {
                            // merge op
                            int merge_flag = 0;
                            if( prev_flag & OPT_CF_COMMUTABLE )
                                merge_flag |= OPT_CF_MERGE_C1;
                            else if( !IS_CONST( prev.B ) )
                                merge_flag |= OPT_CF_MERGE_BR1;
                            if( flag & OPT_CF_COMMUTABLE )
                                merge_flag |= OPT_CF_MERGE_C2;
                            else if( !IS_CONST( curr.B ) )
                                merge_flag |= OPT_CF_MERGE_BR2;

                            Constant *c1, *c2;
                            int prev_R;
                            if( IS_CONST( prev.B ) ) {
                                c1 = &fb->constant_list.value[prev.B-CONST_BASE];
                                prev_R = prev.C;
                            }
                            else {
                                c1 = &fb->constant_list.value[prev.C-CONST_BASE];
                                prev_R = prev.B;
                            }
                            if( IS_CONST( curr.B ) )
                                c2 = &fb->constant_list.value[curr.B-CONST_BASE];
                            else
                                c2 = &fb->constant_list.value[curr.C-CONST_BASE];

                            Constant nc;
                            int BR = 1;
                            switch( merge_flag ) {
                                case OPT_CF_MERGE_C1 | OPT_CF_MERGE_C2:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, curr.op );
                                    break;
                                case OPT_CF_MERGE_BR1:
                                    BR = 0;
                                case OPT_CF_MERGE_BR1 | OPT_CF_MERGE_BR2:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, OPT_CF_COMMUTE_OP( curr.op ) );
                                    break;
                                case OPT_CF_MERGE_BR2:
                                    BR = 0;
                                case 0:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, curr.op );
                                    break;
                                case OPT_CF_MERGE_BR1 | OPT_CF_MERGE_C2:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, prev.op );
                                    curr.op = prev.op;
                                    break;
                                case OPT_CF_MERGE_C2:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, curr.op );
                                    curr.op = prev.op;
                                    BR = 0;
                                    break;
                                case OPT_CF_MERGE_C1 | OPT_CF_MERGE_BR2:
                                    nc = *c1;
                                    constant_binary_arith_op( &nc, c2, curr.op );
                                    curr.op = prev.op;
                                    break;
                                case OPT_CF_MERGE_C1:
                                    nc = *c2;
                                    constant_binary_arith_op( &nc, c1, curr.op );
                                    BR = 0;
                                    break;
                            };

                            int cid = append_constant( fb, &nc );

                            if( BR ) {
                                curr.B = prev_R;
                                curr.C = cid+CONST_BASE;
                            }
                            else {
                                curr.B = cid+CONST_BASE;
                                curr.C = prev_R;
                            }
                        }
                    }
                    else {
                        OPT_CF_CHECK_FROM;
                    }

                    if( opt_from == -1 )
                        opt_from = j;

                    prev = curr;
                    prev_flag = flag;
                }
                else {
                    prev_flag = 0;

                    OPT_CF_CHECK_FROM;
                }
            }
            OPT_CF_CHECK_FROM;
        }
    }
    return opt;
}

int constant_propagation( FunctionBlock* fb, OptArg* oa )
{
    int opt = 0;
    CodeBlock** ppcb = fb->code_block;
    int* R2C = malloc( fb->max_stack_size*sizeof( int ) );
    int* r2c = R2C;
    int i;
    for( i = 0; i < fb->max_stack_size; i++, r2c++ )
        *r2c = -1;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            int j;
            for( j = cb->entry; j <= cb->exit; j++ ) {
                Instruction* in = &fb->instruction_list.value[j];
                InstructionDetail curr;
                get_instruction_detail( in, &curr );

                switch( curr.op ) {
                    case MOVE:
                        R2C[curr.A] = R2C[curr.B];
                        if( R2C[curr.A] != -1 ) {
                            if( oa->hint )
                                in->hint |= HINT_CONSTANT_PROPAGATION;
                            else {
                                curr.op = LOADK;
                                curr.desc = &INSTRUCTION_DESC[LOADK];
                                curr.Bx = R2C[curr.A];
                                make_instruction( in, &curr );

                                R2C[curr.A] = curr.Bx;

                                opt++;
                            }
                        }
                        break;
                    case LOADK:
                        R2C[curr.A] = curr.Bx;
                        break;
                    case GETUPVAL:
                    case GETGLOBAL:
                    case GETTABLE:
                    case NEWTABLE:
                    case CONCAT:
                        R2C[curr.A] = -1;
                        break;
                    case SELF:
                        R2C[curr.A] = -1;
                        R2C[curr.A+1] = -1;
                        break;
                    case CALL:
                    case TFORLOOP:
                    case CLOSURE:
                    case VARARG:
                        break;
                    case ADD:
                    case SUB:
                    case MUL:
                    case DIV:
                    case MOD:
                    case POW:
                        {
                            Constant *c1 = 0, *c2 = 0;
                            if( IS_CONST( curr.B ) )
                                c1 = &fb->constant_list.value[curr.B-CONST_BASE];
                            else {
                                if( R2C[curr.B] != -1 ) {
                                    int c = R2C[curr.B];
                                    if( !oa->hint ) {
                                        curr.B = c+CONST_BASE;
                                        make_instruction( in, &curr );
                                    }
                                    c1 = &fb->constant_list.value[c];
                                }
                            }
                            if( IS_CONST( curr.C ) )
                                c2 = &fb->constant_list.value[curr.C-CONST_BASE];
                            else {
                                if( R2C[curr.C] != -1 ) {
                                    int c = R2C[curr.C];
                                    if( !oa->hint ) {
                                        curr.C = c+CONST_BASE;
                                        make_instruction( in, &curr );
                                    }
                                    c2 = &fb->constant_list.value[c];
                                }
                            }
                            if( c1 && c2 ) {
                                if( oa->hint )
                                    in->hint |= HINT_CONSTANT_PROPAGATION;
                                else {
                                    Constant nc = *c1;
                                    constant_binary_arith_op( &nc, c2, curr.op );
                                    curr.op = LOADK;
                                    curr.desc = &INSTRUCTION_DESC[LOADK];
                                    curr.Bx = append_constant( fb, &nc );
                                    make_instruction( in, &curr );

                                    R2C[curr.A] = curr.Bx;

                                    opt++;
                                }
                            }
                            else
                                R2C[curr.A] = -1;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    free( R2C );

    return opt;
}

int redundancy_elimination( FunctionBlock* fb, OptArg* oa )
{
    int opt = 0;
    CodeBlock** ppcb = fb->code_block;
    int i;
    for( i = 0; i < fb->num_code_block; i++, ppcb++ ) {
        CodeBlock* cb = *ppcb;
        if( cb ) {
            int j;
            for( j = cb->entry; j <= cb->exit; j++ ) {
                Instruction* in = &fb->instruction_list.value[j];
                InstructionDetail ind;
                get_instruction_detail( in, &ind );

                switch( ind.op ) {
                    case LOADK:
                        if( !IS_LOCAL( ind.A, j ) ) {
                            if( oa->hint )
                                in->hint |= HINT_REDUNDANCY_ELEMINATION;
                            else {
                                delete_instruction( fb, cb, j, j );
                                j--;
                                opt++;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return opt;
}

void optimize( FunctionBlock* fb, OptArg* oa )
{
    int verbose = 1;
    if( !oa->hint && !oa->opt_output ) {
        verbose = 0;
        oa->optimize = 0;
        format_function( fb, oa, 0, verbose );
        oa->optimize = 1;
    }

    // create instruction flow
    CodeBlock* cb;
    int i = 0;
    for( i = 0; i < fb->num_code_block; i++ ) {
        cb = fb->code_block[i];
        create_instruction_context( fb, cb );
    }

    // general optimization
    int opt = dead_code_elimination( fb, oa );

    // local optimization
    // TODO: DAG
    //opt += constant_propagation( fb, oa );

    int ret = 0;
    while( ( ret = constant_folding( fb, oa ) ) != 0 )
        opt += ret;

    //opt += redundancy_elimination( fb, oa );

    if( !oa->hint )
        neaten_constant_list( fb );

    int tmp_lv;
    if( oa->hint )
        format_function( fb, oa, 0, verbose );
    else {
        if( !oa->opt_output ) {
            if( opt ) {
                FORMAT_LEVEL( "! optimized\n" );
                format_function( fb, oa, 0, verbose );
            }
            else {
                FORMAT_LEVEL( "! no optimization\n" );
            }
        }
    }

    if( fb->num_func > 0 ) {
        FORMAT_LEVEL( "function prototype list:\n" );
        i = 0;
        FunctionBlock* pfb = ( FunctionBlock* )fb->funcs;
        for( ; i < fb->num_func; i++, pfb++ ) {
            optimize( pfb, oa );
            printf( "\n" );
        }
    }
}
