#ifndef __CHUNK_TYPE_H__
#define __CHUNK_TYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "mylist.h"

//--------------------------------------------------
// structs
//--------------------------------------------------

typedef struct
{
    char block;
    char constant_folding;
    char header;
    char hint;
    char optimize;
    char quiet;
    char summary;
    char verbose;
    char* opt_output;
} OptArg;

typedef struct
{
   unsigned int header_signature;
   unsigned char version;
   unsigned char format_version;
   unsigned char endianess_flag;
   unsigned char size_int;
   unsigned char size_size_t;
   unsigned char size_instruction;
   unsigned char size_number;
   unsigned char integral_flag;
} LuaHeader;

typedef struct
{
    size_t size;
    char* value;
} String;

#define GET_OP( O ) ( O & 0x3F )
#define GET_A( O ) ( ( O >> 6 ) & 0xFF )
#define GET_B( O ) ( O >> 23 )
#define GET_C( O ) ( ( O >> 14 ) & 0x1FF )
#define GET_BX( O ) GET_C( O )
#define GET_SBX( O ) ( ( O >> 14 )-0x1FFFF )

enum INSTRUCTION
{
    MOVE = 0,
    LOADK,
    LOADBOOL,
    LOADNIL,
    GETUPVAL,
    GETGLOBAL,
    GETTABLE,
    SETGLOBAL,
    SETUPVAL,
    SETTABLE,
    NEWTABLE,
    SELF,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    UNM,
    NOT,
    LEN,
    CONCAT,
    JMP,
    EQ,
    LT,
    LE,
    TEST,
    TESTSET,
    CALL,
    TAILCALL,
    RETURN,
    FORLOOP,
    FORPREP,
    TFORLOOP,
    SETLIST,
    CLOSE,
    CLOSURE,
    VARARG,
};

enum INSTRUCTION_TYPE
{
    iABC = 0,
    iABx = 1,
    iAsBx = 2,
};

typedef struct
{
    const char* name;
    unsigned char type;
    unsigned char param_num;
    const char* desc;
} InstructionDesc;

typedef struct
{
    unsigned int opcode;
    int line_pos;
    int hint;
} Instruction;

typedef struct
{
    unsigned char op;
    unsigned char A;
    union {
        struct {
            unsigned short B;
            unsigned short C;
        };
        unsigned int Bx;
        int sBx;
    };
    int line_pos;
    InstructionDesc* desc;
} InstructionDetail;

typedef struct
{
    int size;
    Instruction* value;
} InstructionList;

#define CONST_BASE 0x100
#define IS_CONST( RK ) ( RK >= CONST_BASE )

enum CONSTANT_TYPE
{
    LUA_TNIL = 0,
    LUA_TBOOLEAN = 1,
    LUA_TNUMBER = 3,
    LUA_TSTRING = 4,
};

typedef struct
{
    unsigned char type;
    union
    {
        unsigned char nil;
        unsigned char boolean;
        double number;
        String string;
    };
} Constant;

typedef struct
{
    int size;
    Constant* value;
} ConstantList;

typedef struct
{
    String name;
    int start;
    int end;
} Local;

typedef struct
{
    int size;
    Local* value;
} LocalList;

typedef struct
{
    int size;
    String* value;
} UpvalueList;

typedef struct
{
    struct list_head node;
    int id;
    int entry;
    int exit;
    int num_succ;
    int num_pred;
    void* successors;
    void* predecessors;
    char reachable;
} CodeBlock;

typedef struct
{
    int from;
    int to;
} Jump;

typedef struct {
    int* slots;
    int max_local;
} StackFrame;

typedef struct
{
    String source_name;
    int first_line;
    int last_line;
    unsigned char num_upvalue;
    unsigned char num_parameter;
    unsigned char is_vararg;
    unsigned char max_stack_size;
    InstructionList instruction_list;
    ConstantList constant_list;
    int num_func;
    void* funcs;
    LocalList local_list;
    UpvalueList upvalue_list;
    int level;
    int num_code_block;
    CodeBlock** code_block;
    StackFrame* stack_frames;
} FunctionBlock;

typedef struct
{
    int total_instruction_num;
    int instruction_num[VARARG+1];
} Summary;

//--------------------------------------------------
// functions
//--------------------------------------------------

int is_same_string( String* s1, String* s2 );

void read_function( FILE* f, FunctionBlock* fb, int lv );

void get_summary( FunctionBlock* fb, Summary* smr );

void reset_stack_frames( FunctionBlock* fb );

void get_instruction_detail( Instruction* in, InstructionDetail* ind );
void make_instruction( Instruction* in, InstructionDetail* ind );

#define FORMAT_LEVEL \
    for( tmp_lv = 0; tmp_lv < fb->level; tmp_lv++ ) \
        printf( "\t" ); \
    printf

void format_luaheader( LuaHeader* lh );
void format_function( FunctionBlock* fb, OptArg* fo, int recursive, int verbose );

void write_function( FILE* f, FunctionBlock* fb );

#endif
