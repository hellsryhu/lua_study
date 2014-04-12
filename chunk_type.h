#ifndef __CHUNK_TYPE_H__
#define __CHUNK_TYPE_H__

#include "mylist.h"

//--------------------------------------------------
// structs
//--------------------------------------------------

typedef struct
{
    char flow;
    char header;
    char quiet;
    char summary;
    char verbose;
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
} Instruction;

typedef struct
{
    int size;
    Instruction* value;
} InstructionList;

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
    int entry;
    int exit;
    int size_succ;
    int size_pred;
    void* successors;
    void* predecessors;
} CodeBlock;

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
    struct list_head code_block;
} FunctionBlock;

typedef struct
{
    int total_instruction_num;
    int instruction_num[VARARG+1];
} Summary;

//--------------------------------------------------
// functions
//--------------------------------------------------

#define INIT_FUNCTION_BLOCK( pfb ) \
    { \
        memset( ( pfb ), 0, sizeof( FunctionBlock ) ); \
        INIT_LIST_HEAD( &( pfb )->code_block ); \
    }

void read_string( FILE* f, String* str );
void read_instruction( FILE* f, InstructionList* il );
void read_constant( FILE* f, ConstantList* cl );
void read_linepos( FILE* f, InstructionList* il );
void read_local( FILE* f, LocalList *ll );
void read_upvalue( FILE* f, UpvalueList* ul );
void read_function( FILE* f, FunctionBlock* fb, int lv, Summary* smr );

void format_luaheader( LuaHeader* lh );
void format_instruction( FunctionBlock* fb, Instruction* in, int order, OptArg* fo );
void format_constant( Constant* c, int global );
void format_function( FunctionBlock* fb, OptArg* fo );

void flow_analysis( FunctionBlock* fb, OptArg* oa );

#endif
