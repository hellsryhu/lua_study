#ifndef __LTYPE_H__
#define __LTYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "atype.h"

//--------------------------------------------------
// structs
//--------------------------------------------------

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
    int hint;
} Instruction;

typedef struct
{
    int size;
    Instruction* value;
} InstructionList;

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
    int level;
    int num_code_block;
    CodeBlock** code_block;
    StackFrame* stack_frames;
} FunctionBlock;

#endif
