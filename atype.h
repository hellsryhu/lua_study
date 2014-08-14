#ifndef __ATYPE_H__
#define __ATYPE_H__

#include "mylist.h"
#include "const.h"

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
    const char* name;
    unsigned char type;
    unsigned char param_num;
    const char* desc;
} InstructionDesc;

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

#define GET_OP( O ) ( O & 0x3F )
#define GET_A( O ) ( ( O >> 6 ) & 0xFF )
#define GET_B( O ) ( O >> 23 )
#define GET_C( O ) ( ( O >> 14 ) & 0x1FF )
#define GET_BX( O ) GET_C( O )
#define GET_SBX( O ) ( ( O >> 14 )-0x1FFFF )

#define IS_CONST( RK ) ( RK >= CONST_BASE )

typedef struct {
    int type;
    int val[2];
} ValueInfo;

typedef struct {
    int num_depend;
    int num_dependent;
    int* depends;
    int* dependents;
    ValueInfo lval;
    ValueInfo rval;
} InstructionContext;

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
    char pred_sequential;
    char succ_sequential;
    InstructionContext* instruction_context;
    int* exe_levels;
    int* exe_order;
} CodeBlock;

#define CODE_BLOCK_LEN( cb ) ( cb->exit-cb->entry+1 )

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
    int total_instruction_num;
    int instruction_num[VARARG+1];
} Summary;

#endif
