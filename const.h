#ifndef __CONST_H__
#define __CONST_H__

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

enum CONSTANT_TYPE
{
    LUA_TNIL = 0,
    LUA_TBOOLEAN = 1,
    LUA_TNUMBER = 3,
    LUA_TSTRING = 4,
};

#define CONST_BASE 0x100

enum AFFECT_TYPE
{
    AT_REGISTER = 1,
    AT_GLOBAL = 2,
    AT_UPVALUE = 3,
    AT_TABLE = 4,
    AT_RANGE_REGISTER = 5,
    AT_2_REGISTER = 6,
    AT_TABLE_RANGE = 7,
};

#define FPF 20

#endif
