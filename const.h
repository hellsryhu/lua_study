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

enum VALUE_TYPE
{
    VT_NONE = 0,
    VT_REGISTER = 1,
    VT_2_REGISTER = 2,
    VT_RANGE_REGISTER = 3,
    VT_GLOBAL = 4,
    VT_UPVALUE = 5,
    VT_TABLE = 6,
    VT_TABLE_RANGE = 7,
    VT_VARARG = 8,
};

#define FPF 20

#endif
