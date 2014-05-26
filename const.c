#include "ltype.h"

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
    { "FORLOOP  ", iAsBx, 2, "R(A) += R(A+2);if R(A) <?= R(A+1) then { PC += sBx; R(A+3) = R(A) }" },
    { "FORPREP  ", iAsBx, 2, "R(A) -= R(A+2); PC += sBx" },
    { "TFORLOOP ", iABC, 3, "R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then { R(A+2) = R(A+3); } else { PC++; }" },
    { "SETLIST  ", iABC, 3, "R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B" },
    { "CLOSE    ", iABC, 1, "close all variables in the stack up to (>=) R(A)" },
    { "CLOSURE  ", iABx, 2, "R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))" },
    { "VARARG   ", iABC, 2, "R(A), R(A+1), ..., R(A+B-1) = vararg" },
};

const char* OPTIMIZATION_HINT[] = {
    "constant folding",
    "constant propagation",
};

