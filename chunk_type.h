#ifndef __CHUNK_TYPE_H__
#define __CHUNK_TYPE_H__

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
} FunctionBlock;

char* get_op_name( unsigned int opcode );

void read_string( FILE* f, String* str );
void read_instruction( FILE* f, InstructionList* il );
void read_constant( FILE* f, ConstantList* cl );
void read_linepos( FILE* f, InstructionList* il );
void read_local( FILE* f, LocalList *ll );
void read_upvalue( FILE* f, UpvalueList* ul );
void read_function( FILE* f, FunctionBlock* fb );

void format_luaheader( LuaHeader* lh );
void format_function( FunctionBlock* fb );

#endif
