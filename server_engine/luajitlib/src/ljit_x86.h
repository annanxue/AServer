/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.1.4, DynASM x86 version 1.1.4
** DO NOT EDIT! The original file is in "ljit_x86.dasc".
*/

#if DASM_VERSION != 10104
#error "Version mismatch between DynASM and included encoding engine"
#endif

# 1 "ljit_x86.dasc"
/*
** Bytecode to machine code translation for x86 CPUs.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

//|// Include common definitions and macros.
//|.include ljit_x86.dash
# 1 "ljit_x86.dash"
//|//
//|// Common DynASM definitions and macros for x86 CPUs.
//|// Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
//|//
//|
//|// Standard DynASM declarations.
//|.arch x86
//|.section code, deopt, tail, mfmap
#define DASM_SECTION_CODE	0
#define DASM_SECTION_DEOPT	1
#define DASM_SECTION_TAIL	2
#define DASM_SECTION_MFMAP	3
#define DASM_MAXSECTION		4
# 9 "ljit_x86.dash"
//|
//|// Type definitions with (almost) global validity.
//|.type L,		lua_State,	esi	// L.
#define Dt1(_V) (int)&(((lua_State *)0)_V)
# 12 "ljit_x86.dash"
//|.type BASE,		TValue,		ebx	// L->base.
#define Dt2(_V) (int)&(((TValue *)0)_V)
# 13 "ljit_x86.dash"
//|.type TOP,		TValue,		edi	// L->top (calls/open ops).
#define Dt3(_V) (int)&(((TValue *)0)_V)
# 14 "ljit_x86.dash"
//|.type CI,		CallInfo,	ecx	// L->ci (calls, locally).
#define Dt4(_V) (int)&(((CallInfo *)0)_V)
# 15 "ljit_x86.dash"
//|.type LCL,		LClosure,	eax	// func->value (calls).
#define Dt5(_V) (int)&(((LClosure *)0)_V)
# 16 "ljit_x86.dash"
//|
//|// Type definitions with local validity.
//|.type GL,		global_State
#define Dt6(_V) (int)&(((global_State *)0)_V)
# 19 "ljit_x86.dash"
//|.type TVALUE,		TValue
#define Dt7(_V) (int)&(((TValue *)0)_V)
# 20 "ljit_x86.dash"
//|.type VALUE,		Value
#define Dt8(_V) (int)&(((Value *)0)_V)
# 21 "ljit_x86.dash"
//|.type CINFO,		CallInfo
#define Dt9(_V) (int)&(((CallInfo *)0)_V)
# 22 "ljit_x86.dash"
//|.type GCOBJECT,	GCObject
#define DtA(_V) (int)&(((GCObject *)0)_V)
# 23 "ljit_x86.dash"
//|.type TSTRING,		TString
#define DtB(_V) (int)&(((TString *)0)_V)
# 24 "ljit_x86.dash"
//|.type TABLE,		Table
#define DtC(_V) (int)&(((Table *)0)_V)
# 25 "ljit_x86.dash"
//|.type CCLOSURE,	CClosure
#define DtD(_V) (int)&(((CClosure *)0)_V)
# 26 "ljit_x86.dash"
//|.type PROTO,		Proto
#define DtE(_V) (int)&(((Proto *)0)_V)
# 27 "ljit_x86.dash"
//|.type UPVAL,		UpVal
#define DtF(_V) (int)&(((UpVal *)0)_V)
# 28 "ljit_x86.dash"
//|.type NODE,		Node
#define Dt10(_V) (int)&(((Node *)0)_V)
# 29 "ljit_x86.dash"
//|
//|// Definitions copied to DynASM domain to avoid unnecessary constant args.
//|// CHECK: must match with the definitions in lua.h!
//|.define LUA_TNIL,		0
//|.define LUA_TBOOLEAN,		1
//|.define LUA_TLIGHTUSERDATA,	2
//|.define LUA_TNUMBER,		3
//|.define LUA_TSTRING,		4
//|.define LUA_TTABLE,		5
//|.define LUA_TFUNCTION,		6
//|.define LUA_TUSERDATA,		7
//|.define LUA_TTHREAD,		8
//|
//|.define LUA_TNUM_NUM,		0x33
//|.define LUA_TNUM_NUM_NUM,	0x333
//|.define LUA_TSTR_STR,		0x44
//|.define LUA_TSTR_NUM,		0x43
//|.define LUA_TSTR_NUM_NUM,	0x433
//|.define LUA_TTABLE_NUM,	0x53
//|.define LUA_TTABLE_STR,	0x54
//|
//|// Macros to test, set and copy stack slots.
//|.macro istt, idx, tp;  cmp dword BASE[idx].tt, tp; .endmacro
//|.macro isnil, idx;  istt idx, LUA_TNIL; .endmacro
//|.macro isnumber, idx;  istt idx, LUA_TNUMBER; .endmacro
//|.macro isstring, idx;  istt idx, LUA_TSTRING; .endmacro
//|.macro istable, idx;  istt idx, LUA_TTABLE; .endmacro
//|.macro isfunction, idx;  istt idx, LUA_TFUNCTION; .endmacro
//|
//|.macro isnumber2, idx1, idx2, reg
//|  mov reg, BASE[idx1].tt;  shl reg, 4;  or reg, BASE[idx2].tt
//|  cmp reg, LUA_TNUM_NUM
//|.endmacro
//|.macro isnumber2, idx1, idx2; isnumber2, idx1, idx2, eax; .endmacro
//|
//|.macro isnumber3, idx1, idx2, idx3, reg
//|  mov reg, BASE[idx1].tt;  shl reg, 4;  or reg, BASE[idx2].tt
//|  shl reg, 4;  or reg, BASE[idx3].tt;  cmp reg, LUA_TNUM_NUM_NUM
//|.endmacro
//|.macro isnumber3, idx1, idx2, idx3; isnumber3, idx1, idx2, idx3, eax; .endmacro
//|
//|.macro tvisnil, tv;  cmp dword tv.tt, LUA_TNIL; .endmacro
//|
//|.macro settt, tv, tp;  mov dword tv.tt, tp; .endmacro
//|.macro setnilvalue, tv;  settt tv, LUA_TNIL; .endmacro
//|
//|.macro setbvalue, tv, val		// May use edx.
//||if (val) {  /* true */
//|   mov edx, LUA_TBOOLEAN
//|   mov dword tv.value, edx		// Assumes: LUA_TBOOLEAN == 1
//|   settt tv, edx
//||} else {  /* false */
//|   mov dword tv.value, 0
//|   settt tv, LUA_TBOOLEAN
//||}
//|.endmacro
//|
//|.macro loadnvaluek, vptr
//||if ((vptr)->n == (lua_Number)0) {
//|  fldz
//||} else if ((vptr)->n == (lua_Number)1) {
//|  fld1
//||} else {
//|  fld qword [vptr]
//||}
//|.endmacro
//|
//|.macro setnvaluek, tv, vptr		// Pass a Value *! With permanent addr.
//|  // SSE2 does not pay off here (I tried).
//|  loadnvaluek vptr
//|  fstp qword tv.value
//|  settt tv, LUA_TNUMBER
//|.endmacro
//|
//|.macro setnvalue, tv, vptr		// Pass a Value *! Temporary ok.
//|  mov dword tv.value, (vptr)->na[0]
//|  mov dword tv.value.na[1], (vptr)->na[1]
//|  settt tv, LUA_TNUMBER
//|.endmacro
//|
//|.macro setsvalue, tv, vptr
//|  mov aword tv.value, vptr
//|  settt tv, LUA_TSTRING
//|.endmacro
//|
//|.macro setclvalue, tv, vptr
//|  mov aword tv.value, vptr
//|  settt tv, LUA_TFUNCTION
//|.endmacro
//|
//|.macro sethvalue, tv, vptr
//|  mov aword tv.value, vptr
//|  settt tv, LUA_TTABLE
//|.endmacro
//|
//|.macro copyslotSSE, D, S, R1		// May use xmm0.
//|  mov R1, S.tt;  movq xmm0, qword S.value
//|  mov D.tt, R1;  movq qword D.value, xmm0
//|.endmacro
//|
//|.macro copyslot, D, S, R1, R2, R3
//||if (J->flags & JIT_F_CPU_SSE2) {
//|  copyslotSSE D, S, R1
//||} else {
//|  mov R1, S.value;  mov R2, S.value.na[1];  mov R3, S.tt
//|  mov D.value, R1;  mov D.value.na[1], R2;  mov D.tt, R3
//||}
//|.endmacro
//|
//|.macro copyslot, D, S, R1, R2
//||if (J->flags & JIT_F_CPU_SSE2) {
//|  copyslotSSE D, S, R1
//||} else {
//|  mov R1, S.value;  mov R2, S.value.na[1];  mov D.value, R1
//|  mov R1, S.tt;  mov D.value.na[1], R2;  mov D.tt, R1
//||}
//|.endmacro
//|
//|.macro copyslot, D, S
//|  copyslot D, S, ecx, edx, eax
//|.endmacro
//|
//|.macro copyconst, tv, tvk		// May use edx.
//||switch (ttype(tvk)) {
//||case LUA_TNIL:
//|   setnilvalue tv
//||  break;
//||case LUA_TBOOLEAN:
//|   setbvalue tv, bvalue(tvk)		// May use edx.
//||  break;
//||case LUA_TNUMBER: {
//|   setnvaluek tv, &(tvk)->value
//||  break;
//||}
//||case LUA_TSTRING:
//|   setsvalue tv, &gcvalue(tvk)
//||  break;
//||case LUA_TFUNCTION:
//|   setclvalue tv, &gcvalue(tvk)
//||  break;
//||default: lua_assert(0); break;
//||}
//|.endmacro
//|
//|// Macros to get Lua structures.
//|.macro getLCL, reg			// May use CI and TOP (edi).
//||if (!J->pt->is_vararg) {
//|  mov LCL:reg, BASE[-1].value
//||} else {
//|  mov CI, L->ci
//|  mov TOP, CI->func
//|  mov LCL:reg, TOP->value
//||}
//|.endmacro
//|.macro getLCL;  getLCL eax; .endmacro
//|
//|// Macros to handle variants.
//|.macro addidx, type, idx
//||if (idx) {
//|  add type, idx*#type
//||}
//|.endmacro
//|
//|.macro subidx, type, idx
//||if (idx) {
//|  sub type, idx*#type
//||}
//|.endmacro
//|
//|// Annoying x87 stuff: support for two compare variants.
//|.macro fcomparepp			// Compare and pop st0 >< st1.
//||if (J->flags & JIT_F_CPU_CMOV) {
//|  fucomip st1
//|  fpop
//||} else {
//|  fucompp
//|  fnstsw ax				// eax modified!
//|  sahf
//|  // Sometimes test ah, imm8 would be faster.
//|  // But all following compares need to be changed then.
//|  // Don't bother since this is only compatibility stuff for old CPUs.
//||}
//|.endmacro
//|
//|// If you change LUA_TVALUE_ALIGN, be sure to change the Makefile, too:
//|//   DASMFLAGS= -D TVALUE_SIZE=...
//|// Then rerun make. Or change the default below:
//|.if not TVALUE_SIZE; .define TVALUE_SIZE, 16; .endif
//|
//|.if TVALUE_SIZE == 16
//|  .macro TValuemul, reg;  sal reg, 4; .endmacro  // *16
//|  .macro TValuediv, reg;  sar reg, 4; .endmacro  // /16
//|  .macro Nodemul, reg;  sal reg, 5; .endmacro    // *32
//|.elif TVALUE_SIZE == 12
//|  .macro TValuemul, reg;  sal reg, 2;  lea reg, [reg+reg*2]; .endmacro  // *12
//|  .macro TValuediv, reg;  sal reg, 2;  imul reg, 0xaaaaaaab; .endmacro  // /12
//|  .macro Nodemul, reg;  imul reg, 28; .endmacro  // *28
//|.else
//|  .fatal Unsupported TValue size `TVALUE_SIZE'.
//|.endif
//|
//|//
//|// x86 C calling conventions and stack frame layout during a JIT call:
//|//
//|// ebp+aword*4  CARG2     nresults
//|// ebp+aword*3  CARG2     func      (also used as SAVER3 for L)
//|// ebp+aword*2  CARG1     L
//|// -------------------------------  call to GATE_LJ
//|// ebp+aword*1  retaddr
//|// ebp+aword*0  frameptr       ebp
//|// ebp-aword*1  SAVER1    TOP  edi
//|// ebp-aword*2  SAVER2    BASE ebx
//|// -------------------------------
//|//              GATE_LJ retaddr
//|// esp+aword*2  ARG3
//|// esp+aword*1  ARG2
//|// esp+aword*0  ARG1                <-- esp for first JIT frame
//|// -------------------------------
//|//              1st JIT frame retaddr
//|// esp+aword*2  ARG3
//|// esp+aword*1  ARG2
//|// esp+aword*0  ARG1                <-- esp for second JIT frame
//|// -------------------------------
//|//              2nd JIT frame retaddr
//|//
//|// We could omit the fixed frame pointer (ebp) and have one more register
//|// available. But there is no pressing need (could use it for CI).
//|// And it's easier for debugging (gdb is still confused -- why?).
//|//
//|// The stack is aligned to 4 awords (16 bytes). Calls to C functions
//|// with up to 3 arguments do not need any stack pointer adjustment.
//|//
//|
//|.define CARG3, [ebp+aword*4]
//|.define CARG2, [ebp+aword*3]
//|.define CARG1, [ebp+aword*2]
//|.define SAVER1, [ebp-aword*1]
//|.define SAVER2, [ebp-aword*2]
//|.define ARG7, aword [esp+aword*6]	// Requires large frame.
//|.define ARG6, aword [esp+aword*5]	// Requires large frame.
//|.define ARG5, aword [esp+aword*4]	// Requires large frame.
//|.define ARG4, aword [esp+aword*3]	// Requires large frame.
//|.define ARG3, aword [esp+aword*2]
//|.define ARG2, aword [esp+aword*1]
//|.define ARG1, aword [esp]
//|.define FRAME_RETADDR, aword [esp+aword*3]
//|.define TMP3, [esp+aword*2]
//|.define TMP2, [esp+aword*1]
//|.define TMP1, [esp]
//|.define FPARG2, qword [esp+qword*1]	// Requires large frame.
//|.define FPARG1, qword [esp]
//|.define LJFRAME_OFFSET, aword*2	// 16 byte aligned with retaddr + ebp.
//|.define FRAME_OFFSET, aword*3		// 16 byte aligned with retaddr.
//|.define LFRAME_OFFSET, aword*7		// 16 byte aligned with retaddr.
//|.define S2LFRAME_OFFSET, aword*4	// Delta to large frame.
//|
//|.macro call, target, a1
//|  mov ARG1, a1;  call target; .endmacro
//|.macro call, target, a1, a2
//|  mov ARG1, a1;  mov ARG2, a2;  call target; .endmacro
//|.macro call, target, a1, a2, a3
//|  mov ARG1, a1;  mov ARG2, a2;  mov ARG3, a3;  call target; .endmacro
//|.macro call, target, a1, a2, a3, a4
//|  push a4;  push a3;  push a2;  push a1
//|  call target;  add esp, S2LFRAME_OFFSET;  .endmacro
//|.macro call, target, a1, a2, a3, a4, a5
//|  mov ARG1, a5; push a4; push a3;  push a2;  push a1
//|  call target;  add esp, S2LFRAME_OFFSET;  .endmacro
//|
//|// The following macros require a large frame.
//|.macro call_LFRAME, target, a1, a2, a3, a4
//|  mov ARG1, a1;  mov ARG2, a2;  mov ARG3, a3;  mov ARG4, a4
//|  call target; .endmacro
//|.macro call_LFRAME, target, a1, a2, a3, a4, a5
//|  mov ARG1, a1;  mov ARG2, a2;  mov ARG3, a3;  mov ARG4, a4;  mov ARG5, a5
//|  call target; .endmacro
//|
# 8 "ljit_x86.dasc"
//|
//|// Place actionlist and globals here at the top of the file.
//|.actionlist jit_actionlist
static const unsigned char jit_actionlist[5083] = {
  156,90,137,209,129,252,242,0,0,32,0,82,157,156,90,49,192,57,209,15,132,245,
  247,64,83,15,162,91,137,208,249,1,195,255,254,0,251,15,249,10,141,68,36,4,
  195,251,15,249,11,85,137,229,131,252,236,8,137,93,252,252,139,93,12,137,117,
  12,139,117,8,137,125,252,248,139,190,235,139,131,235,139,142,235,102,252,
  255,134,235,252,255,144,235,139,142,235,137,190,235,139,145,235,139,69,16,
  137,150,235,139,145,235,133,192,137,150,235,15,136,245,248,193,224,4,1,195,
  49,201,137,158,235,249,1,137,143,235,129,199,241,57,223,15,130,245,1,249,
  2,255,102,252,255,142,235,184,239,139,125,252,248,139,93,252,252,139,117,
  12,137,252,236,93,195,251,15,249,12,139,144,235,129,186,235,241,15,133,245,
  247,139,146,235,137,144,235,252,255,226,249,1,131,252,236,12,139,129,235,
  137,142,235,137,190,235,199,68,36,8,252,255,252,255,252,255,252,255,137,134,
  235,137,92,36,4,43,158,235,137,52,36,232,244,133,192,15,133,245,248,137,52,
  36,199,68,36,4,1,0,0,0,232,244,249,2,131,196,12,3,158,235,255,139,190,235,
  195,251,15,249,13,141,135,235,131,252,236,12,59,134,235,15,131,245,14,59,
  142,235,141,137,235,15,132,245,15,137,142,235,137,153,235,137,129,235,139,
  147,235,129,195,241,137,190,235,137,158,235,137,153,235,249,16,137,52,36,
  252,255,146,235,249,2,131,196,12,139,142,235,255,193,224,4,139,185,235,15,
  132,245,250,139,158,235,137,218,41,195,249,3,139,3,131,195,4,137,7,131,199,
  4,57,211,15,130,245,3,249,4,139,153,235,129,233,241,137,142,235,195,144,144,
  144,144,144,144,251,15,249,17,252,246,134,235,237,15,133,245,253,249,6,137,
  52,36,252,255,146,235,252,246,134,235,237,15,132,245,2,255,137,195,137,52,
  36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,255,252,255,232,244,137,
  216,233,245,2,249,7,137,211,137,52,36,199,68,36,4,239,199,68,36,8,252,255,
  252,255,252,255,252,255,232,244,137,218,233,245,6,251,15,249,14,41,252,248,
  193,252,248,4,137,190,235,43,158,235,137,76,36,8,137,52,36,137,68,36,4,232,
  244,139,76,36,8,3,158,235,139,190,235,139,131,235,131,196,12,252,255,160,
  235,251,15,249,15,137,190,235,137,52,36,232,244,141,136,235,255,139,131,235,
  137,142,235,131,196,12,252,255,160,235,255,249,18,90,233,245,19,249,20,137,
  190,235,249,19,137,150,235,137,52,36,232,244,139,158,235,139,190,235,252,
  255,224,251,15,255,137,190,235,255,232,245,21,255,251,15,249,21,252,246,134,
  235,237,15,132,245,248,252,255,142,235,15,132,245,247,252,246,134,235,237,
  15,132,245,248,249,1,139,4,36,131,252,236,12,137,52,36,137,68,36,4,232,244,
  131,196,12,139,158,235,139,190,235,249,2,195,255,250,255,233,246,255,250,
  243,255,254,1,233,245,19,254,0,250,254,2,250,251,1,252,255,252,255,254,3,
  242,0,0,0,0,0,0,0,0,0,254,0,141,249,9,186,239,254,0,249,9,186,239,233,245,
  20,254,0,139,142,235,139,129,235,191,247,253,59,129,235,15,131,245,22,249,
  7,255,251,15,249,22,137,52,36,232,244,139,158,235,252,255,231,255,131,187,
  235,5,15,133,245,9,49,192,137,131,235,137,131,235,254,3,238,238,254,0,131,
  190,235,0,15,132,245,9,199,134,235,239,129,195,241,255,141,187,235,255,137,
  158,235,137,190,235,137,52,36,232,244,139,158,235,139,190,235,255,199,135,
  235,0,0,0,0,255,139,139,235,252,243,15,126,131,235,137,139,235,102,15,214,
  131,235,255,139,139,235,139,147,235,139,131,235,137,139,235,137,147,235,137,
  131,235,255,57,223,15,130,245,9,255,131,187,235,8,15,133,245,9,139,131,235,
  131,184,235,0,15,132,245,9,199,134,235,239,137,190,235,137,52,36,137,92,36,
  4,199,68,36,8,239,232,244,139,158,235,255,137,199,255,131,187,235,4,15,133,
  245,9,139,139,235,219,129,235,199,131,235,3,0,0,0,221,155,235,255,141,187,
  235,232,245,23,137,131,235,199,131,235,4,0,0,0,255,141,187,235,232,245,24,
  137,131,235,199,131,235,4,0,0,0,255,131,187,235,3,15,133,245,9,141,134,235,
  221,131,235,219,24,129,56,252,255,0,0,0,15,135,245,9,137,52,36,137,68,36,
  4,199,68,36,8,1,0,0,0,232,244,137,131,235,199,131,235,4,0,0,0,255,251,15,
  249,23,139,135,235,193,224,4,11,135,235,193,224,4,11,135,235,45,51,4,0,0,
  15,133,245,18,221,135,235,221,135,235,219,92,36,8,219,92,36,4,139,143,235,
  139,185,235,139,84,36,8,57,215,15,130,245,250,249,1,11,68,36,4,15,142,245,
  252,249,2,41,194,15,140,245,253,141,140,253,1,235,66,249,3,137,116,36,4,137,
  76,36,8,137,84,36,12,139,190,235,139,135,235,255,59,135,235,15,131,245,254,
  233,244,249,4,15,140,245,251,141,84,58,1,233,245,1,249,5,137,252,250,233,
  245,1,249,6,15,132,245,251,1,252,248,64,15,143,245,2,249,5,184,1,0,0,0,233,
  245,2,249,7,49,210,233,245,3,255,251,15,249,24,139,135,235,193,224,4,11,135,
  235,131,232,67,15,133,245,18,221,135,235,219,92,36,4,139,143,235,139,185,
  235,137,252,250,233,245,1,249,8,131,252,236,12,137,52,36,232,244,131,196,
  12,139,158,235,233,244,255,131,187,235,5,15,133,245,9,255,141,131,235,137,
  52,36,137,68,36,4,232,244,255,141,131,235,141,139,235,137,52,36,137,68,36,
  4,137,76,36,8,232,244,255,139,131,235,137,4,36,232,244,137,4,36,219,4,36,
  221,155,235,199,131,235,3,0,0,0,255,131,187,235,3,15,133,245,9,221,131,235,
  255,139,131,235,193,224,4,11,131,235,131,252,248,51,15,133,245,9,255,217,
  252,254,255,217,252,255,255,217,252,242,221,216,255,217,60,36,217,45,239,
  217,252,252,217,44,36,255,217,225,255,217,252,250,255,221,131,235,221,131,
  235,249,1,217,252,248,223,224,158,15,138,245,1,221,217,255,221,131,235,221,
  131,235,217,252,243,255,221,28,36,232,244,255,131,187,235,6,15,133,245,9,
  129,187,235,239,15,133,245,9,255,141,131,235,57,199,15,133,245,9,255,141,
  187,235,137,190,235,255,131,196,12,129,252,235,241,129,174,235,241,195,255,
  141,187,235,137,52,36,137,124,36,4,232,244,133,192,15,133,246,255,139,131,
  235,64,139,147,235,137,131,235,137,20,36,137,68,36,4,232,244,139,136,235,
  133,201,15,132,245,255,219,131,235,199,131,235,3,0,0,0,221,155,235,139,144,
  235,139,128,235,137,139,235,137,147,235,137,131,235,233,246,249,9,255,141,
  135,235,131,252,236,12,59,134,235,15,131,245,14,59,142,235,141,137,235,15,
  132,245,15,49,192,137,153,235,129,195,241,137,142,235,255,141,147,235,57,
  215,255,137,223,255,15,71,252,250,255,15,134,245,247,137,215,249,1,255,141,
  147,235,137,129,235,137,145,235,137,150,235,137,158,235,137,153,235,255,15,
  130,245,251,249,4,254,2,249,5,137,135,235,129,199,241,57,215,15,130,245,5,
  233,245,4,254,0,137,190,235,137,185,235,137,129,235,255,139,139,235,252,243,
  15,126,131,235,137,143,235,102,15,214,135,235,255,139,139,235,139,147,235,
  137,143,235,139,139,235,137,151,235,137,143,235,255,137,252,251,141,147,235,
  141,187,235,137,145,235,137,150,235,255,137,135,235,255,249,2,137,135,235,
  137,135,235,129,199,241,57,215,15,130,245,2,255,137,52,36,232,244,255,252,
  246,134,235,237,15,132,245,255,232,245,25,249,9,255,251,15,249,25,139,142,
  235,139,185,235,139,135,235,139,184,235,139,135,235,131,192,4,137,134,235,
  131,252,236,12,137,52,36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,
  255,252,255,232,244,131,196,12,139,135,235,137,134,235,139,158,235,195,255,
  137,52,36,137,92,36,4,232,244,255,129,174,235,241,137,223,129,252,235,241,
  131,196,12,255,139,142,235,139,153,235,129,233,241,137,142,235,141,187,235,
  131,196,12,255,252,246,134,235,237,15,132,245,253,232,245,26,249,7,255,139,
  68,36,12,137,134,235,255,251,15,249,26,139,4,36,137,134,235,131,252,236,12,
  137,52,36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,255,252,255,232,
  244,131,196,12,139,158,235,139,190,235,195,255,139,145,235,57,252,251,15,
  131,245,248,249,1,139,3,131,195,4,137,2,131,194,4,57,252,251,15,130,245,1,
  249,2,131,196,12,139,153,235,129,233,241,137,215,137,142,235,195,255,129,
  174,235,241,129,252,235,241,255,131,196,12,141,187,235,195,255,139,142,235,
  139,185,235,129,233,241,137,142,235,255,139,139,235,139,147,235,139,131,235,
  137,143,235,137,151,235,137,135,235,255,131,196,12,137,252,251,255,129,199,
  241,255,139,142,235,131,187,235,6,255,139,131,235,186,239,137,145,235,255,
  15,133,245,20,255,15,133,245,19,255,15,132,245,247,232,245,27,249,1,255,251,
  15,249,27,131,252,236,12,137,150,235,137,190,235,137,52,36,137,92,36,4,232,
  244,131,196,12,137,195,139,190,235,139,131,235,139,142,235,195,255,252,255,
  144,235,255,137,158,235,255,49,192,255,141,147,235,249,1,137,135,235,137,
  135,235,129,199,241,57,215,15,130,245,1,255,131,187,235,6,15,133,245,9,255,
  131,187,235,6,15,133,245,251,254,2,249,5,255,186,239,233,245,28,254,0,251,
  15,249,28,137,150,235,137,190,235,137,52,36,137,92,36,4,232,244,139,142,235,
  139,150,235,139,185,235,249,1,139,24,131,192,4,137,31,131,199,4,57,208,15,
  130,245,1,139,153,235,139,131,235,129,233,241,131,196,12,252,255,160,235,
  255,139,131,235,255,139,139,235,139,147,235,137,139,235,139,139,235,137,147,
  235,137,139,235,255,141,187,235,129,252,235,241,139,142,235,137,131,235,255,
  139,142,235,141,187,235,139,153,235,139,135,235,137,131,235,255,139,135,235,
  252,243,15,126,135,235,137,131,235,102,15,214,131,235,255,139,135,235,139,
  151,235,137,131,235,139,135,235,137,147,235,137,131,235,255,141,187,235,139,
  131,235,255,139,145,235,249,1,139,3,131,195,4,137,2,131,194,4,57,252,251,
  15,130,245,1,139,153,235,137,215,139,131,235,255,199,131,235,0,0,0,0,255,
  186,1,0,0,0,137,147,235,137,147,235,255,199,131,235,0,0,0,0,199,131,235,1,
  0,0,0,255,217,252,238,255,217,232,255,221,5,239,255,199,131,235,239,199,131,
  235,4,0,0,0,255,199,131,235,239,199,131,235,6,0,0,0,255,137,131,235,195,255,
  141,139,235,141,147,235,249,1,137,1,57,209,141,137,235,15,134,245,1,255,139,
  142,235,139,185,235,139,135,235,255,139,136,235,139,185,235,255,139,143,235,
  252,243,15,126,135,235,137,139,235,102,15,214,131,235,255,139,143,235,139,
  151,235,139,135,235,137,139,235,137,147,235,137,131,235,255,139,136,235,139,
  185,235,139,131,235,139,147,235,137,135,235,131,252,248,4,139,131,235,137,
  151,235,137,135,235,15,131,245,251,249,4,254,2,249,5,252,246,130,235,237,
  15,132,245,4,252,246,129,235,237,15,132,245,4,232,245,29,233,245,4,254,0,
  251,15,249,29,137,84,36,12,137,76,36,8,137,116,36,4,233,244,255,251,15,249,
  30,139,142,235,139,185,235,139,135,235,139,184,235,233,245,255,255,251,15,
  249,31,131,191,235,5,139,191,235,15,133,245,18,249,9,15,182,143,235,184,1,
  0,0,0,211,224,72,35,130,235,193,224,5,3,135,235,249,1,131,184,235,4,15,133,
  245,248,57,144,235,15,133,245,248,139,136,235,133,201,15,132,245,249,255,
  252,243,15,126,128,235,102,15,214,131,235,255,139,144,235,139,184,235,137,
  147,235,137,187,235,255,137,139,235,139,158,235,195,249,2,139,128,235,133,
  192,15,133,245,1,49,201,249,3,139,135,235,133,192,15,132,245,250,252,246,
  128,235,237,15,132,245,251,249,4,137,139,235,139,158,235,195,249,5,137,150,
  235,199,134,235,4,0,0,0,139,12,36,131,252,236,12,137,142,235,137,52,36,137,
  124,36,4,137,92,36,8,232,244,131,196,12,139,158,235,255,251,15,249,32,139,
  135,235,193,224,4,11,129,235,131,252,248,84,139,191,235,139,145,235,15,132,
  245,9,233,245,18,255,251,15,249,33,139,142,235,128,167,235,237,139,145,235,
  137,185,235,137,151,235,195,255,251,15,249,34,139,142,235,139,185,235,139,
  135,235,139,184,235,233,245,255,255,251,15,249,35,131,191,235,5,139,191,235,
  15,133,245,18,249,9,15,182,143,235,184,1,0,0,0,211,224,72,35,130,235,193,
  224,5,3,135,235,249,1,131,184,235,4,15,133,245,250,57,144,235,15,133,245,
  250,131,184,235,0,15,132,245,252,249,2,198,135,235,0,249,3,255,252,246,135,
  235,237,15,133,245,254,249,7,255,139,139,235,252,243,15,126,131,235,137,136,
  235,102,15,214,128,235,255,139,139,235,139,147,235,139,187,235,137,136,235,
  137,144,235,137,184,235,255,139,158,235,195,249,8,232,245,33,233,245,7,249,
  4,139,128,235,133,192,15,133,245,1,139,143,235,133,201,15,132,245,251,252,
  246,129,235,237,15,132,245,253,249,5,141,134,235,137,144,235,199,128,235,
  4,0,0,0,131,252,236,12,137,52,36,137,124,36,4,137,68,36,8,232,244,131,196,
  12,233,245,2,249,6,255,139,143,235,133,201,15,132,245,2,252,246,129,235,237,
  15,133,245,2,249,7,137,150,235,199,134,235,4,0,0,0,139,12,36,131,252,236,
  12,137,142,235,137,52,36,137,124,36,4,137,92,36,8,232,244,131,196,12,139,
  158,235,195,255,251,15,249,36,139,135,235,193,224,4,11,129,235,131,252,248,
  84,139,191,235,139,145,235,15,132,245,9,233,245,18,255,137,52,36,199,68,36,
  4,239,199,68,36,8,239,232,244,137,131,235,199,131,235,5,0,0,0,255,186,239,
  255,232,245,30,255,232,245,34,255,141,187,235,186,239,255,141,187,235,141,
  139,235,255,131,187,235,5,139,187,235,15,133,245,255,185,239,139,135,235,
  59,143,235,15,135,245,251,255,139,131,235,193,224,4,11,131,235,131,252,248,
  83,15,133,245,255,255,252,242,15,16,131,235,252,242,15,44,192,252,242,15,
  42,200,72,102,15,46,200,139,187,235,15,133,245,255,15,138,245,255,255,221,
  131,235,219,20,36,219,4,36,255,223,233,221,216,255,218,233,223,224,158,255,
  15,133,245,255,15,138,245,255,139,4,36,139,187,235,72,255,59,135,235,15,131,
  245,251,193,224,4,3,135,235,255,232,245,31,255,232,245,32,255,185,239,255,
  141,147,235,255,199,134,235,239,83,81,82,86,232,244,131,196,16,139,158,235,
  255,249,1,139,144,235,133,210,15,132,245,252,255,139,136,235,139,128,235,
  137,139,235,137,131,235,255,249,2,137,147,235,254,2,232,245,37,255,232,245,
  38,255,233,245,1,249,6,139,143,235,133,201,15,132,245,2,252,246,129,235,237,
  15,133,245,2,249,9,186,239,233,245,19,254,0,251,15,249,37,137,76,36,4,131,
  252,236,12,137,60,36,137,76,36,4,232,244,131,196,12,139,76,36,4,193,225,4,
  41,200,129,192,241,195,255,251,15,249,38,64,137,124,36,4,137,68,36,8,233,
  244,255,187,239,255,232,245,35,255,232,245,36,255,199,134,235,239,82,81,83,
  86,232,244,131,196,16,139,158,235,255,249,1,131,184,235,0,15,132,245,252,
  249,2,254,2,232,245,39,255,232,245,40,255,252,246,135,235,237,15,133,245,
  253,249,3,254,2,249,7,232,245,33,233,245,3,254,0,199,128,235,0,0,0,0,255,
  186,1,0,0,0,137,144,235,137,144,235,255,199,128,235,0,0,0,0,199,128,235,1,
  0,0,0,255,221,152,235,199,128,235,3,0,0,0,255,199,128,235,239,199,128,235,
  4,0,0,0,255,199,128,235,239,199,128,235,6,0,0,0,255,251,15,249,39,137,76,
  36,4,131,252,236,12,137,52,36,137,124,36,4,137,76,36,8,232,244,131,196,12,
  139,76,36,4,193,225,4,41,200,129,192,241,195,255,251,15,249,40,64,137,116,
  36,4,137,124,36,8,137,68,36,12,233,244,255,137,190,235,141,131,235,41,252,
  248,252,247,216,193,252,248,4,139,187,235,15,132,245,250,255,129,192,241,
  255,57,135,235,15,131,245,247,137,52,36,137,124,36,4,137,68,36,8,232,244,
  249,1,252,246,135,235,237,139,151,235,15,133,245,252,139,190,235,254,2,249,
  6,232,245,33,233,245,1,254,0,139,187,235,129,191,235,241,15,130,245,251,249,
  1,252,246,135,235,237,139,151,235,15,133,245,252,141,187,235,254,2,249,5,
  137,52,36,137,124,36,4,199,68,36,8,239,232,244,233,245,1,249,6,232,245,33,
  233,245,1,254,0,129,194,241,255,141,139,235,249,3,139,1,131,193,4,137,2,131,
  194,4,57,252,249,15,130,245,3,249,4,255,131,187,235,3,139,131,235,15,133,
  245,255,133,192,15,136,245,255,255,221,131,235,221,5,239,255,221,5,239,221,
  131,235,255,139,131,235,193,224,4,11,131,235,131,252,248,51,139,131,235,15,
  133,245,255,11,131,235,15,136,245,255,221,131,235,221,131,235,255,131,187,
  235,3,15,133,245,255,221,131,235,255,216,200,255,217,192,216,200,255,220,
  201,255,222,201,255,199,4,36,239,199,68,36,4,239,199,68,36,8,239,131,187,
  235,3,15,133,245,255,219,44,36,220,139,235,217,192,217,252,252,220,233,217,
  201,217,252,240,217,232,222,193,217,252,253,221,217,255,251,15,249,41,217,
  232,221,68,36,8,217,252,241,139,68,36,4,219,56,195,255,131,187,235,3,15,133,
  245,255,255,131,187,235,3,255,139,131,235,193,224,4,11,131,235,131,252,248,
  51,255,216,192,255,220,131,235,255,220,163,235,255,220,171,235,255,220,139,
  235,255,220,179,235,255,220,187,235,255,131,252,236,16,221,28,36,221,131,
  235,221,92,36,8,232,244,131,196,16,255,131,252,236,16,221,92,36,8,221,131,
  235,221,28,36,232,244,131,196,16,255,217,224,255,15,138,246,255,15,130,246,
  255,15,134,246,255,15,135,246,255,15,131,246,255,199,134,235,239,137,52,36,
  137,76,36,4,137,84,36,8,232,244,133,192,139,158,235,255,15,132,246,255,199,
  134,235,239,199,4,36,239,82,81,83,86,232,244,131,196,16,139,158,235,255,131,
  187,235,5,139,139,235,15,133,245,9,137,12,36,232,244,137,4,36,219,4,36,221,
  155,235,199,131,235,3,0,0,0,255,131,187,235,4,139,139,235,15,133,245,9,219,
  129,235,221,155,235,199,131,235,3,0,0,0,255,199,134,235,239,137,52,36,137,
  92,36,4,137,76,36,8,232,244,139,158,235,255,139,131,235,139,139,235,186,1,
  0,0,0,33,193,209,232,9,193,49,192,57,209,17,192,137,147,235,137,131,235,255,
  232,245,42,137,131,235,199,131,235,4,0,0,0,255,199,134,235,239,137,52,36,
  199,68,36,4,239,199,68,36,8,239,232,244,139,158,235,255,251,15,249,42,137,
  116,36,4,139,131,235,193,224,4,11,131,235,131,232,68,15,133,245,18,249,1,
  139,190,235,139,179,235,139,147,235,139,142,235,133,201,15,132,245,248,11,
  130,235,15,132,245,250,1,200,15,130,245,255,59,135,235,15,135,245,251,139,
  191,235,129,198,241,255,252,243,164,139,138,235,141,178,235,252,243,164,41,
  199,139,116,36,4,137,124,36,8,137,68,36,12,139,158,235,233,244,249,2,137,
  208,249,3,139,116,36,4,139,158,235,195,249,4,137,252,240,233,245,3,249,5,
  139,116,36,4,141,143,235,131,252,236,12,137,52,36,137,76,36,4,137,68,36,8,
  232,244,131,196,12,49,192,233,245,1,249,9,139,116,36,4,233,245,18,255,131,
  187,235,0,255,139,131,235,139,139,235,72,73,9,200,255,139,131,235,72,11,131,
  235,255,131,187,235,3,15,133,246,221,131,235,221,5,239,255,131,187,235,4,
  15,133,246,129,187,235,239,255,139,131,235,59,131,235,15,133,246,255,131,
  252,248,3,15,133,245,9,221,131,235,221,131,235,255,131,252,248,4,15,133,245,
  9,139,139,235,59,139,235,255,141,147,235,141,139,235,199,134,235,239,137,
  52,36,137,76,36,4,137,84,36,8,232,244,72,139,158,235,255,139,131,235,139,
  139,235,137,194,33,202,141,20,80,209,234,255,15,132,245,247,255,15,133,245,
  247,255,139,147,235,137,131,235,137,139,235,137,147,235,233,246,249,1,255,
  139,131,235,193,224,4,11,131,235,131,252,248,51,15,133,245,255,249,4,221,
  131,235,221,131,235,221,147,235,255,249,4,139,131,235,193,224,4,11,131,235,
  193,224,4,11,131,235,61,51,3,0,0,139,131,235,15,133,245,255,221,131,235,221,
  131,235,133,192,221,147,235,15,136,245,247,217,201,249,1,255,199,131,235,
  3,0,0,0,15,130,246,255,249,9,141,131,235,199,134,235,239,137,52,36,137,68,
  36,4,232,244,233,245,4,254,0,221,131,235,221,131,235,220,131,235,221,147,
  235,221,147,235,199,131,235,3,0,0,0,255,139,131,235,221,131,235,221,131,235,
  221,131,235,222,193,221,147,235,221,147,235,199,131,235,3,0,0,0,133,192,15,
  136,245,247,217,201,249,1,255,131,187,235,0,15,132,245,247,255,141,131,235,
  137,68,36,4,255,137,92,36,4,255,139,187,235,255,139,142,235,139,185,235,139,
  191,235,255,139,151,235,137,52,36,199,68,36,4,239,137,84,36,8,232,244,199,
  128,235,239,137,131,235,199,131,235,6,0,0,0,255,139,151,235,137,144,235,255,
  137,52,36,232,244,137,135,235,255,249,1,139,142,235,139,145,235,129,194,241,
  141,132,253,27,235,41,208,59,134,235,15,131,245,251,141,187,235,57,218,15,
  131,245,249,249,2,139,2,131,194,4,137,7,131,199,4,57,218,15,130,245,2,249,
  3,254,2,249,5,43,134,235,193,252,248,4,137,52,36,137,68,36,4,232,244,139,
  158,235,233,245,1,254,0,139,142,235,139,145,235,129,194,241,141,187,235,141,
  139,235,57,218,15,131,245,248,249,1,139,2,131,194,4,137,7,131,199,4,57,207,
  15,131,245,250,57,218,15,130,245,1,249,2,49,192,249,3,137,135,235,129,199,
  241,57,207,15,130,245,3,249,4,255
};

# 11 "ljit_x86.dasc"
//|.globals JSUB_
enum {
  JSUB_STACKPTR,
  JSUB_GATE_LJ,
  JSUB_GATE_JL,
  JSUB_GATE_JC,
  JSUB_GROW_STACK,
  JSUB_GROW_CI,
  JSUB_GATE_JC_PATCH,
  JSUB_GATE_JC_DEBUG,
  JSUB_DEOPTIMIZE_CALLER,
  JSUB_DEOPTIMIZE,
  JSUB_DEOPTIMIZE_OPEN,
  JSUB_HOOKINS,
  JSUB_GCSTEP,
  JSUB_STRING_SUB3,
  JSUB_STRING_SUB2,
  JSUB_HOOKCALL,
  JSUB_HOOKRET,
  JSUB_METACALL,
  JSUB_METATAILCALL,
  JSUB_BARRIERF,
  JSUB_GETGLOBAL,
  JSUB_GETTABLE_KSTR,
  JSUB_GETTABLE_STR,
  JSUB_BARRIERBACK,
  JSUB_SETGLOBAL,
  JSUB_SETTABLE_KSTR,
  JSUB_SETTABLE_STR,
  JSUB_GETTABLE_KNUM,
  JSUB_GETTABLE_NUM,
  JSUB_SETTABLE_KNUM,
  JSUB_SETTABLE_NUM,
  JSUB_LOG2_TWORD,
  JSUB_CONCAT_STR2,
  JSUB__MAX
};
# 12 "ljit_x86.dasc"

/* ------------------------------------------------------------------------ */

/* Arch string. */
const char luaJIT_arch[] = "x86";

/* Forward declarations for C functions called from jsubs. */
static void jit_hookins(lua_State *L, const Instruction *newpc);
static void jit_gettable_fb(lua_State *L, Table *t, StkId dest);
static void jit_settable_fb(lua_State *L, Table *t, StkId val);

/* ------------------------------------------------------------------------ */

/* Detect CPU features and set JIT flags. */
static int jit_cpudetect(jit_State *J)
{
  void *mcode;
  size_t sz;
  int status;
  /* Some of the jsubs need the flags. So compile this separately. */
  unsigned int feature;
  dasm_setup(Dst, jit_actionlist);
  //|  // Check for CPUID support first.
  //|  pushfd
  //|  pop edx
  //|  mov ecx, edx
  //|  xor edx, 0x00200000		// Toggle ID bit in flags.
  //|  push edx
  //|  popfd
  //|  pushfd
  //|  pop edx
  //|  xor eax, eax			// Zero means no features supported.
  //|  cmp ecx, edx
  //|  jz >1				// No ID toggle means no CPUID support.
  //|
  //|  inc eax				// CPUID function 1.
  //|  push ebx				// Callee-save ebx modified by CPUID.
  //|  cpuid
  //|  pop ebx
  //|  mov eax, edx			// Return feature support bits.
  //|1:
  //|  ret
  dasm_put(Dst, 0);
# 54 "ljit_x86.dasc"
  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &mcode, &sz);
  if (status != JIT_S_OK)
    return status;
  /* Check feature bits. See the Intel/AMD manuals for the bit definitions. */
  feature = ((unsigned int (*)(void))mcode)();
  if (feature & (1<<15)) J->flags |= JIT_F_CPU_CMOV;
  if (feature & (1<<26)) J->flags |= JIT_F_CPU_SSE2;
  luaJIT_freemcode(J, mcode, sz);  /* We don't need this code anymore. */
  return JIT_S_OK;
}

/* Check some assumptions. Should compile to nop. */
static int jit_consistency_check(jit_State *J)
{
  do {
    /* Force a compiler error for inconsistent structure sizes. */
    /* Check LUA_TVALUE_ALIGN in luaconf.h, too. */
    int check_TVALUE_SIZE_in_ljit_x86_dash[1+16-sizeof(TValue)];
    int check_TVALUE_SIZE_in_ljit_x86_dash_[1+sizeof(TValue)-16];
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash[0]);
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash_[0]);
    if (LUA_TNIL != 0 || LUA_TBOOLEAN != 1 || PCRLUA != 0) break;
    if ((int)&(((Node *)0)->i_val) != (int)&(((StkId)0)->value)) break;
    return JIT_S_OK;
  } while (0);
  J->dasmstatus = 999999999;  /* Recognizable error. */
  return JIT_S_COMPILER_ERROR;
}

/* Compile JIT subroutines (once). */
static int jit_compile_jsub(jit_State *J)
{
  int status = jit_consistency_check(J);
  if (status != JIT_S_OK) return status;
  status = jit_cpudetect(J);
  if (status != JIT_S_OK) return status;
  dasm_setup(Dst, jit_actionlist);
  //|// Macros to reorder and combine JIT subroutine definitions.
  //|.macro .jsub, name
  //|.capture JSUB			// Add the entry point.
  //||//-----------------------------------------------------------------------
  //||//->name:
  //|  .align 16
  //|->name:
  //|.endmacro
  //|.macro .endjsub;  .endcapture; .endmacro
  //|.macro .dumpjsub;  .dumpcapture JSUB; .endmacro
  //|
  //|.code
  dasm_put(Dst, 34);
# 104 "ljit_x86.dasc"
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|  // Must be the first JSUB defined or used.
  //|->STACKPTR:				// Get stack pointer (for jit.util.*).
  //|  lea eax, [esp+aword*1]		// But adjust for the return address.
  //|  ret
  //|
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|->GATE_LJ:				// Lua -> JIT gate. (L, func, nresults)
  //|  push ebp
  //|  mov ebp, esp
  //|  sub esp, LJFRAME_OFFSET
  //|  mov SAVER1, BASE
  //|   mov BASE, CARG2	// func
  //|  mov CARG2, L	// Arg used as savereg. Avoids aword*8 stack frame.
  //|   mov L, CARG1	// L
  //|  mov SAVER2, TOP
  //|   mov TOP, L->top
  //|  mov LCL, BASE->value
  //|   mov CI, L->ci
  //|  // Prevent stackless yields. No limit check -- this is not a real C call.
  //|  inc word L->nCcalls  // short
  //|
  //|  call aword LCL->jit_gate		// Call the compiled code.
  //|
  //|   mov CI, L->ci
  //|  mov L->top, TOP			// Only correct for LUA_MULTRET.
  //|   mov edx, CI->savedpc
  //|  mov eax, CARG3	// nresults
  //|   mov L->savedpc, edx		// L->savedpc = CI->savedpc
  //|   mov edx, CI->base
  //|  test eax, eax
  //|   mov L->base, edx			// L->base = CI->base
  //|  js >2				// Skip for nresults == LUA_MULTRET.
  //|
  //|  TValuemul eax
  //|  add BASE, eax
  //|  xor ecx, ecx
  //|  mov L->top, BASE			// L->top = &func[nresults]
  //|1:  // No initial check. May use EXTRA_STACK (once).
  //|  mov TOP->tt, ecx			// Clear unset stack slots.
  //|  add TOP, #TOP
  //|  cmp TOP, BASE
  //|  jb <1
  //|
  //|2:
  //|  dec word L->nCcalls  // short
  dasm_put(Dst, 36, Dt1(->top), Dt2(->value), Dt1(->ci), Dt1(->nCcalls), Dt5(->jit_gate), Dt1(->ci), Dt1(->top), Dt4(->savedpc), Dt1(->savedpc), Dt4(->base), Dt1(->base), Dt1(->top), Dt3(->tt), sizeof(TValue));
# 152 "ljit_x86.dasc"
  //|  mov eax, PCRC
  //|  mov TOP, SAVER2
  //|  mov BASE, SAVER1
  //|  mov L, CARG2
  //|  mov esp, ebp
  //|  pop ebp
  //|  ret
  //|
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|->GATE_JL:				// JIT -> Lua callgate.
  //|  mov PROTO:edx, LCL->p
  //|  cmp dword PROTO:edx->jit_status, JIT_S_OK
  //|  jne >1				// Already compiled?
  //|
  //|  // Yes, copy callgate to closure (so GATE_JL is not called again).
  //|  mov edx, PROTO:edx->jit_mcode
  //|  mov LCL->jit_gate, edx
  //|  jmp edx				// Chain to compiled code.
  //|
  //|1:  // Let luaD_precall do the hard work: compile & run or fallback.
  //|  sub esp, FRAME_OFFSET
  //|   mov eax, CI->savedpc
  //|  mov L->ci, CI			// May not be in sync for tailcalls.
  //|   mov L->top, TOP
  //|  mov ARG3, -1			// LUA_MULTRET
  //|   mov L->savedpc, eax		// luaD_precall expects it there.
  //|  mov ARG2, BASE
  //|   sub BASE, L->stack		// Preserve old BASE (= func).
  //|  mov ARG1, L
  //|  call &luaD_precall			// luaD_precall(L, func, nresults)
  //|  test eax,eax			// Assumes: PCRLUA == 0
  //|  jnz >2				// PCRC? PCRYIELD cannot happen.
  //|
  //|  // Returned PCRLUA: need to call the bytecode interpreter.
  //|  call &luaV_execute, L, 1
  //|  // Indirect yield (L->status == LUA_YIELD) cannot happen.
  //|
  //|2:  // Returned PCRC: compile & run done. Frame is already unwound.
  //|  add esp, FRAME_OFFSET
  //|  add BASE, L->stack  // Restore stack-relative pointers BASE and TOP.
  //|  mov TOP, L->top
  dasm_put(Dst, 145, Dt1(->nCcalls), PCRC, Dt5(->p), DtE(->jit_status), JIT_S_OK, DtE(->jit_mcode), Dt5(->jit_gate), Dt4(->savedpc), Dt1(->ci), Dt1(->top), Dt1(->savedpc), Dt1(->stack), (ptrdiff_t)(luaD_precall), (ptrdiff_t)(luaV_execute), Dt1(->stack));
# 194 "ljit_x86.dasc"
  //|  ret
  //|
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|->GATE_JC:				// JIT -> C callgate.
  //|  lea eax, TOP[LUA_MINSTACK]
  //|   sub esp, FRAME_OFFSET
  //|  cmp eax, L->stack_last
  //|  jae ->GROW_STACK			// Stack overflow?
  //|  cmp CI, L->end_ci
  //|   lea CI, CI[1]
  //|  je ->GROW_CI			// CI overflow?
  //|  mov L->ci, CI
  //|  mov CI->func, BASE
  //|  mov CI->top, eax
  //|  mov CCLOSURE:edx, BASE->value
  //|  add BASE, #BASE
  //|  mov L->top, TOP
  //|  mov L->base, BASE
  //|  mov CI->base, BASE
  //|  // ci->nresults is not set because we don't use luaD_poscall().
  //|
  //|->GATE_JC_PATCH:			// Patch mark for jmp to GATE_JC_DEBUG.
  //|
  //|  call aword CCLOSURE:edx->f, L	// Call the C function.
  //|
  //|2:					// Label used below!
  //|  add esp, FRAME_OFFSET
  //|   mov CI, L->ci
  //|  TValuemul eax			// eax = nresults*sizeof(TValue)
  dasm_put(Dst, 262, Dt1(->top), Dt3([LUA_MINSTACK]), Dt1(->stack_last), Dt1(->end_ci), Dt4([1]), Dt1(->ci), Dt4(->func), Dt4(->top), Dt2(->value), sizeof(TValue), Dt1(->top), Dt1(->base), Dt4(->base), DtD(->f), Dt1(->ci));
# 224 "ljit_x86.dasc"
  //|   mov TOP, CI->func
  //|  jz >4				// Skip loop if nresults == 0.
  //|					// Yield (-1) cannot happen.
  //|  mov BASE, L->top
  //|  mov edx, BASE
  //|  sub BASE, eax			// BASE = &L->top[-nresults]
  //|3:  // Relocate [L->top-nresults, L->top) -> [ci->func, ci->func+nresults)
  //|  mov eax, [BASE]
  //|  add BASE, aword*1
  //|  mov [TOP], eax
  //|  add TOP, aword*1
  //|  cmp BASE, edx
  //|  jb <3
  //|
  //|4:
  //|  mov BASE, CI->func
  //|  sub CI, #CI
  //|  mov L->ci, CI
  //|  ret
  //|
  //|//-----------------------------------------------------------------------
  //|  nop; nop; nop; nop; nop; nop	// Save area. See DEBUGPATCH_SIZE.
  //|  .align 16
  //|->GATE_JC_DEBUG:			// JIT -> C callgate for debugging.
  //|  test byte L->hookmask, LUA_MASKCALL // Need to call hook?
  //|  jnz >7
  //|6:
  //|  call aword CCLOSURE:edx->f, L	// Call the C function.
  //|
  //|  test byte L->hookmask, LUA_MASKRET	// Need to call hook?
  //|  jz <2
  //|
  //|  // Return hook. TODO: LUA_HOOKTAILRET is not called since tailcalls == 0.
  //|  mov BASE, eax  // BASE (ebx) is callee-save.
  dasm_put(Dst, 336, Dt4(->func), Dt1(->top), Dt4(->func), sizeof(CallInfo), Dt1(->ci), Dt1(->hookmask), LUA_MASKCALL, DtD(->f), Dt1(->hookmask), LUA_MASKRET);
# 258 "ljit_x86.dasc"
  //|  call &luaD_callhook, L, LUA_HOOKRET, -1
  //|  mov eax, BASE
  //|  jmp <2
  //|
  //|7:  // Call hook.
  //|  mov BASE, CCLOSURE:edx  // BASE (ebx) is callee-save.
  //|  call &luaD_callhook, L, LUA_HOOKCALL, -1
  //|  mov CCLOSURE:edx, BASE
  //|  jmp <6
  //|
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|->GROW_STACK:			// Grow stack. Jump from/to prologue.
  //|  sub eax, TOP
  //|  TValuediv eax			// eax = (eax-TOP)/sizeof(TValue).
  //|  mov L->top, TOP
  //|  sub BASE, L->stack
  //|  mov ARG3, CI
  //|  call &luaD_growstack, L, eax
  //|  mov CI, ARG3			// CI may not be in sync with L->ci.
  //|  add BASE, L->stack			// Restore stack-relative pointers.
  //|  mov TOP, L->top
  //|  mov LCL, BASE->value
  //|  add esp, FRAME_OFFSET		// Undo esp adjust of prologue/GATE_JC.
  //|  jmp aword LCL->jit_gate		// Retry prologue.
  //|
  //|//-----------------------------------------------------------------------
  //|  .align 16
  //|->GROW_CI:				// Grow CI. Jump from/to prologue.
  //|  mov L->top, TOP			// May throw LUA_ERRMEM, so save TOP.
  //|  call &luaD_growCI, L
  //|  lea CI, CINFO:eax[-1]		// Undo ci++ (L->ci reset in prologue).
  //|  mov LCL, BASE->value
  dasm_put(Dst, 421, LUA_HOOKRET, (ptrdiff_t)(luaD_callhook), LUA_HOOKCALL, (ptrdiff_t)(luaD_callhook), Dt1(->top), Dt1(->stack), (ptrdiff_t)(luaD_growstack), Dt1(->stack), Dt1(->top), Dt2(->value), Dt5(->jit_gate), Dt1(->top), (ptrdiff_t)(luaD_growCI), Dt9([-1]));
# 291 "ljit_x86.dasc"
  //|  mov L->ci, CI
  //|  add esp, FRAME_OFFSET		// Undo esp adjust of prologue/GATE_JC.
  //|  jmp aword LCL->jit_gate		// Retry prologue.
  //|
  //|//-----------------------------------------------------------------------
  //|.dumpjsub				// Dump all captured .jsub's.
  dasm_put(Dst, 547, Dt2(->value), Dt1(->ci), Dt5(->jit_gate));
  //|//-----------------------------------------------------------------------
  //|//->HOOKINS:
# 395 "ljit_x86.dasc"
  //|  test byte L->hookmask, LUA_MASKLINE|LUA_MASKCOUNT
  //|  jz >2
  //|  dec dword L->hookcount
  //|  jz >1
  //|  test byte L->hookmask, LUA_MASKLINE
  //|  jz >2
  //|1:
  //|  mov eax, [esp]			// Current machine code address.
  //|  sub esp, FRAME_OFFSET
  //|  call &jit_hookins, L, eax
  //|  add esp, FRAME_OFFSET
  //|  mov BASE, L->base			// Restore stack-relative pointers.
  //|  mov TOP, L->top
  //|2:
  //|  ret
  //|.endjsub
  dasm_put(Dst, 602, Dt1(->hookmask), LUA_MASKLINE|LUA_MASKCOUNT, Dt1(->hookcount), Dt1(->hookmask), LUA_MASKLINE, (ptrdiff_t)(jit_hookins), Dt1(->base), Dt1(->top));
  //|//-----------------------------------------------------------------------
  //|//->GCSTEP:
# 494 "ljit_x86.dasc"
  //|  call &luaC_step, L
  //|  mov BASE, L->base
  //|  jmp TOP
  //|.endjsub
  dasm_put(Dst, 737, (ptrdiff_t)(luaC_step), Dt1(->base));
  //|//-----------------------------------------------------------------------
  //|//->STRING_SUB3:
# 207 "ljit_x86_inline.dash"
  //|  mov eax, TOP[0].tt; shl eax, 4; or eax, TOP[1].tt; shl eax, 4
  //|  or eax, TOP[2].tt; sub eax, LUA_TSTR_NUM_NUM
  //|  jne ->DEOPTIMIZE_CALLER		// Wrong types? Deoptimize.
  //|  // eax must be zero here!
  //|   fld qword TOP[1].value
  //|  fld qword TOP[2].value
  //|  fistp aword TMP3			// size_t
  //|   fistp aword TMP2			// size_t
  //|   mov TSTRING:ecx, TOP[0].value
  //|   mov TOP, aword TSTRING:ecx->tsv.len  // size_t
  //|  mov edx, TMP3
  //|   cmp TOP, edx
  //|  jb >4
  //|1:
  //|  or eax, TMP2			// eax is known to be zero.
  //|  jle >6				// start <= 0?
  //|2:
  //|  sub edx, eax			// newlen = end-start
  //|  jl >7				// start > end?
  //|  lea ecx, [TSTRING:ecx+eax+#TSTRING-1]  // svalue()-1+start
  //|  inc edx
  //|3:
  //|  mov ARG2, L			// First arg for tailcall is ARG2.
  //|  mov ARG3, ecx			// Pointer to start.
  //|  mov ARG4, edx			// Length.
  //|   mov GL:edi, L->l_G
  //|   mov eax, GL:edi->totalbytes	// size_t
  //|   cmp eax, GL:edi->GCthreshold	// size_t
  dasm_put(Dst, 1026, Dt3([0].tt), Dt3([1].tt), Dt3([2].tt), Dt3([1].value), Dt3([2].value), Dt3([0].value), DtB(->tsv.len), sizeof(TString)-1, Dt1(->l_G), Dt6(->totalbytes));
# 235 "ljit_x86_inline.dash"
  //|   jae >8				// G->totalbytes >= G->GCthreshold?
  //|  jmp &luaS_newlstr			// Tailcall to C function.
  //|
  //|4:  // Negative end or overflow.
  //|  jl >5
  //|  lea edx, [edx+TOP+1]		// end = end+(len+1)
  //|  jmp <1
  //|5:  // Overflow
  //|  mov edx, TOP			// end = len
  //|  jmp <1
  //|
  //|6:  // Negative start or underflow.
  //|  je >5
  //|  add eax, TOP			// start = start+(len+1)
  //|  inc eax
  //|  jg <2				// start > 0?
  //|5:  // Underflow.
  //|  mov eax, 1				// start = 1
  //|  jmp <2
  //|
  //|7:  // Range underflow.
  //|  xor edx, edx			// Zero length.
  //|  jmp <3				// Any pointer in ecx is ok.
  //|.endjsub
  dasm_put(Dst, 1129, Dt6(->GCthreshold), (ptrdiff_t)(luaS_newlstr));
  //|//-----------------------------------------------------------------------
  //|//->STRING_SUB2:
# 262 "ljit_x86_inline.dash"
  //|  mov eax, TOP[0].tt; shl eax, 4; or eax, TOP[1].tt; sub eax, LUA_TSTR_NUM
  //|  jne ->DEOPTIMIZE_CALLER		// Wrong types? Deoptimize.
  //|  // eax must be zero here!
  //|  fld qword TOP[1].value
  //|  fistp aword TMP2			// size_t
  //|  mov TSTRING:ecx, TOP[0].value
  //|  mov TOP, aword TSTRING:ecx->tsv.len // size_t
  //|  mov edx, TOP
  //|  jmp <1				// See STRING_SUB3.
  //|
  //|8:  // GC threshold reached.
  //|  sub esp, FRAME_OFFSET
  //|  call &luaC_step, L
  //|  add esp, FRAME_OFFSET
  //|  mov BASE, L->base
  //|  jmp &luaS_newlstr			// Tailcall to C function.
  //|.endjsub
  dasm_put(Dst, 1191, Dt3([0].tt), Dt3([1].tt), Dt3([1].value), Dt3([0].value), DtB(->tsv.len), (ptrdiff_t)(luaC_step), Dt1(->base), (ptrdiff_t)(luaS_newlstr));
    //|//-----------------------------------------------------------------------
    //|//->HOOKCALL:
# 657 "ljit_x86.dasc"
    //|  mov CI, L->ci
    //|  mov TOP, CI->func
    //|  mov LCL, TOP->value
    //|  mov PROTO:edi, LCL->p		// clvalue(L->ci->func)->l.p
    //|  mov eax, PROTO:edi->code
    //|  add eax, 4			// Hooks expect incremented PC.
    //|  mov L->savedpc, eax
    //|  sub esp, FRAME_OFFSET
    //|  call &luaD_callhook, L, LUA_HOOKCALL, -1
    //|  add esp, FRAME_OFFSET
    //|  mov eax, PROTO:edi->code		// PROTO:edi is callee-save.
    //|  mov L->savedpc, eax		// jit_hookins needs previous PC.
    //|  mov BASE, L->base
    //|  ret
    //|.endjsub
    dasm_put(Dst, 1755, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->p), DtE(->code), Dt1(->savedpc), LUA_HOOKCALL, (ptrdiff_t)(luaD_callhook), DtE(->code), Dt1(->savedpc), Dt1(->base));
    //|//-----------------------------------------------------------------------
    //|//->HOOKRET:
# 718 "ljit_x86.dasc"
    //|  mov eax, [esp]			// Current machine code address.
    //|  mov L->savedpc, eax
    //|  sub esp, FRAME_OFFSET
    //|  call &luaD_callhook, L, LUA_HOOKRET, -1
    //|  add esp, FRAME_OFFSET
    //|  mov BASE, L->base		// Restore stack-relative pointers.
    //|  mov TOP, L->top
    //|  ret
    //|.endjsub
    dasm_put(Dst, 1886, Dt1(->savedpc), LUA_HOOKRET, (ptrdiff_t)(luaD_callhook), Dt1(->base), Dt1(->top));
    //|//-----------------------------------------------------------------------
    //|//->METACALL:
# 815 "ljit_x86.dasc"
    //|  sub esp, FRAME_OFFSET
    //|  mov L->savedpc, edx		// May throw errors. Save PC and TOP.
    //|  mov L->top, TOP
    //|  call &luaD_tryfuncTM, L, BASE	// Resolve __call metamethod.
    //|  add esp, FRAME_OFFSET
    //|  mov BASE, eax			// Restore stack-relative pointers.
    //|  mov TOP, L->top
    //|  mov LCL, BASE->value
    //|  mov CI, L->ci
    //|  ret
    //|.endjsub
    dasm_put(Dst, 2077, Dt1(->savedpc), Dt1(->top), (ptrdiff_t)(luaD_tryfuncTM), Dt1(->top), Dt2(->value), Dt1(->ci));
    //|//-----------------------------------------------------------------------
    //|//->METATAILCALL:
# 884 "ljit_x86.dasc"
    //|  mov L->savedpc, edx
    //|  mov L->top, TOP
    //|  call &luaD_tryfuncTM, L, BASE	// Resolve __call metamethod.
    //|
    //|// Relocate [eax, L->top) -> [L->ci->func, *).
    //|  mov CI, L->ci
    //|  mov edx, L->top
    //|  mov TOP, CI->func
    //|1:
    //|  mov BASE, [eax]
    //|  add eax, aword*1
    //|  mov [TOP], BASE
    //|  add TOP, aword*1
    //|  cmp eax, edx
    //|  jb <1
    //|
    //|  mov BASE, CI->func
    //|  mov LCL, BASE->value
    //|  sub CI, #CI
    //|  add esp, FRAME_OFFSET
    //|  jmp aword LCL->jit_gate		// Chain to callgate.
    //|.endjsub
    dasm_put(Dst, 2178, Dt1(->savedpc), Dt1(->top), (ptrdiff_t)(luaD_tryfuncTM), Dt1(->ci), Dt1(->top), Dt4(->func), Dt4(->func), Dt2(->value), sizeof(CallInfo), Dt5(->jit_gate));
  //|//-----------------------------------------------------------------------
  //|//->BARRIERF:
# 1049 "ljit_x86.dasc"
  //|  mov ARG4, GCOBJECT:edx
  //|  mov ARG3, UPVAL:ecx
  //|  mov ARG2, L
  //|  jmp &luaC_barrierf			// Chain to C code.
  //|.endjsub
  dasm_put(Dst, 2582, (ptrdiff_t)(luaC_barrierf));
  //|//-----------------------------------------------------------------------
  //|//->GETGLOBAL:
# 1085 "ljit_x86.dasc"
  //|// Call with: TSTRING:edx (key), BASE (dest)
  //|  mov CI, L->ci
  //|  mov TOP, CI->func
  //|  mov LCL, TOP->value
  //|  mov TABLE:edi, LCL->env
  //|  jmp >9
  //|.endjsub
  dasm_put(Dst, 2601, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->env));
  //|//-----------------------------------------------------------------------
  //|//->GETTABLE_KSTR:
# 1095 "ljit_x86.dasc"
  //|// Call with: TOP (tab), TSTRING:edx (key), BASE (dest)
  //|  cmp dword TOP->tt, LUA_TTABLE
  //|   mov TABLE:edi, TOP->value
  //|  jne ->DEOPTIMIZE_CALLER		// Not a table? Deoptimize.
  //|
  //|// Common entry: TABLE:edi (tab), TSTRING:edx (key), BASE (dest)
  //|// Restores BASE, destroys eax, ecx, edx, edi (TOP).
  //|9:
  //|  movzx ecx, byte TABLE:edi->lsizenode	// hashstr(t, key).
  //|  mov eax, 1
  //|  shl eax, cl
  //|  dec eax
  //|  and eax, TSTRING:edx->tsv.hash
  //|  Nodemul NODE:eax
  //|  add NODE:eax, TABLE:edi->node
  //|
  //|1:  // Start of inner loop. Check node key.
  //|  cmp dword NODE:eax->i_key.nk.tt, LUA_TSTRING
  //|  jne >2
  //|  cmp aword NODE:eax->i_key.nk.value, TSTRING:edx
  //|  jne >2
  //|  // Note: swapping the two checks is faster, but valgrind complains.
  //|// Assumes: (int)&(((Node *)0)->i_val) == (int)&(((StkId)0)->value)
  //|
  //|// Ok, key found. Copy node value to destination (stack) slot.
  //|  mov ecx, NODE:eax->i_val.tt
  //|  test ecx, ecx; je >3			// Node has nil value?
  dasm_put(Dst, 2621, Dt3(->tt), Dt3(->value), DtC(->lsizenode), DtB(->tsv.hash), DtC(->node), Dt10(->i_key.nk.tt), Dt10(->i_key.nk.value), Dt10(->i_val.tt));
  if (J->flags & JIT_F_CPU_SSE2) {
# 1123 "ljit_x86.dasc"
  //|  movq xmm0, qword NODE:eax->i_val.value
  //|  movq qword BASE->value, xmm0
  dasm_put(Dst, 2686, Dt10(->i_val.value), Dt2(->value));
  } else {
# 1126 "ljit_x86.dasc"
  //|  mov edx, NODE:eax->i_val.value
  //|  mov edi, NODE:eax->i_val.value.na[1]
  //|  mov BASE->value, edx
  //|  mov BASE->value.na[1], edi
  dasm_put(Dst, 2698, Dt10(->i_val.value), Dt10(->i_val.value.na[1]), Dt2(->value), Dt2(->value.na[1]));
  }
# 1131 "ljit_x86.dasc"
  //|  mov BASE->tt, ecx
  //|  mov BASE, L->base
  //|  ret
  //|2:
  //|  mov NODE:eax, NODE:eax->i_key.nk.next	// Get next key in chain.
  //|  test NODE:eax, NODE:eax
  //|  jnz <1					// Loop if non-NULL.
  //|
  //|  xor ecx, ecx
  //|3:
  //|  mov TABLE:eax, TABLE:edi->metatable
  //|  test TABLE:eax, TABLE:eax
  //|  jz >4					// No metatable?
  //|  test byte TABLE:eax->flags, 1<<TM_INDEX
  //|  jz >5					// Or 'no __index' flag set?
  //|4:
  //|  settt BASE[0], ecx				// Yes, set to nil.
  //|  mov BASE, L->base
  //|  ret
  //|
  //|5:  // Otherwise chain to C code which eventually calls luaV_gettable.
  //|  setsvalue L->env, TSTRING:edx		// Use L->env as temp key.
  //|  mov ecx, [esp]
  //|  sub esp, FRAME_OFFSET
  //|  mov L->savedpc, ecx
  //|  call &jit_gettable_fb, L, TABLE:edi, BASE
  //|  add esp, FRAME_OFFSET
  //|  mov BASE, L->base
  //|  ret
  dasm_put(Dst, 2711, Dt2(->tt), Dt1(->base), Dt10(->i_key.nk.next), DtC(->metatable), DtC(->flags), 1<<TM_INDEX, Dt2([0].tt), Dt1(->base), Dt1(->env.value), Dt1(->env.tt), Dt1(->savedpc), (ptrdiff_t)(jit_gettable_fb), Dt1(->base));
# 1160 "ljit_x86.dasc"
  //|.endjsub
  dasm_put(Dst, 32);
  //|//-----------------------------------------------------------------------
  //|//->GETTABLE_STR:
# 1164 "ljit_x86.dasc"
  //|// Call with: TOP (tab), TVALUE:ecx (key), BASE (dest)
  //|  mov eax, TOP->tt; shl eax, 4; or eax, TVALUE:ecx->tt
  //|  cmp eax, LUA_TTABLE_STR
  //|   mov TABLE:edi, TOP->value
  //|   mov TSTRING:edx, TVALUE:ecx->value
  //|  je <9					// Types ok? Continue above.
  //|  jmp ->DEOPTIMIZE_CALLER		// Otherwise deoptimize.
  //|.endjsub
  dasm_put(Dst, 2802, Dt3(->tt), Dt7(->tt), Dt3(->value), Dt7(->value));
  //|//-----------------------------------------------------------------------
  //|//->BARRIERBACK:
# 1198 "ljit_x86.dasc"
  //|// Call with: TABLE:edi (table). Destroys ecx, edx.
  //|  mov GL:ecx, L->l_G
  //|   and byte TABLE:edi->marked, (~bitmask(BLACKBIT))&0xff
  //|  mov edx, GL:ecx->grayagain
  //|   mov GL:ecx->grayagain, TABLE:edi
  //|  mov TABLE:edi->gclist, edx
  //|  ret
  //|.endjsub
  dasm_put(Dst, 2833, Dt1(->l_G), DtC(->marked), (~bitmask(BLACKBIT))&0xff, Dt6(->grayagain), Dt6(->grayagain), DtC(->gclist));
  //|//-----------------------------------------------------------------------
  //|//->SETGLOBAL:
# 1209 "ljit_x86.dasc"
  //|// Call with: TSTRING:edx (key), BASE (val)
  //|  mov CI, L->ci
  //|  mov TOP, CI->func
  //|  mov LCL, TOP->value
  //|  mov TABLE:edi, LCL->env
  //|  jmp >9
  //|.endjsub
  dasm_put(Dst, 2855, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->env));
  //|//-----------------------------------------------------------------------
  //|//->SETTABLE_KSTR:
# 1219 "ljit_x86.dasc"
  //|// Call with: TOP (tab), TSTRING:edx (key), BASE (val)
  //|  cmp dword TOP->tt, LUA_TTABLE
  //|   mov TABLE:edi, TOP->value
  //|  jne ->DEOPTIMIZE_CALLER		// Not a table? Deoptimize.
  //|
  //|// Common entry: TABLE:edi (tab), TSTRING:edx (key), BASE (val)
  //|// Restores BASE, destroys eax, ecx, edx, edi (TOP).
  //|9:
  //|  movzx ecx, byte TABLE:edi->lsizenode	// hashstr(t, key).
  //|  mov eax, 1
  //|  shl eax, cl
  //|  dec eax
  //|  and eax, TSTRING:edx->tsv.hash
  //|  Nodemul NODE:eax
  //|  add NODE:eax, TABLE:edi->node
  //|
  //|1:  // Start of inner loop. Check node key.
  //|  cmp dword NODE:eax->i_key.nk.tt, LUA_TSTRING
  //|  jne >4
  //|  cmp aword NODE:eax->i_key.nk.value, TSTRING:edx
  //|  jne >4
  //|  // Note: swapping the two checks is faster, but valgrind complains.
  //|
  //|// Ok, key found. Copy new value to node value.
  //|  cmp dword NODE:eax->i_val.tt, LUA_TNIL	// Previous value is nil?
  //|  je >6
  //|  // Assumes: (int)&(((Node *)0)->i_val) == (int)&(((StkId)0)->value)
  //|2:
  //|  mov byte TABLE:edi->flags, 0		// Clear metamethod cache.
  //|3:  // Target for SETTABLE_NUM below.
  //|  test byte TABLE:edi->marked, bitmask(BLACKBIT)  // isblack(table)
  dasm_put(Dst, 2875, Dt3(->tt), Dt3(->value), DtC(->lsizenode), DtB(->tsv.hash), DtC(->node), Dt10(->i_key.nk.tt), Dt10(->i_key.nk.value), Dt10(->i_val.tt), DtC(->flags));
# 1250 "ljit_x86.dasc"
  //|  jnz >8				// Unlikely, but set barrier back.
  //|7:  // Caveat: recycled label.
  //|  copyslot TVALUE:eax[0], BASE[0], ecx, edx, TOP
  dasm_put(Dst, 2947, DtC(->marked), bitmask(BLACKBIT));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 2959, Dt2([0].tt), Dt2([0].value), Dt7([0].tt), Dt7([0].value));
  } else {
  dasm_put(Dst, 2977, Dt2([0].value), Dt2([0].value.na[1]), Dt2([0].tt), Dt7([0].value), Dt7([0].value.na[1]), Dt7([0].tt));
  }
# 1253 "ljit_x86.dasc"
  //|  mov BASE, L->base
  //|  ret
  //|
  //|8:  // Avoid valiswhite() check -- black2gray(table) is ok.
  //|  call ->BARRIERBACK
  //|  jmp <7
  //|
  //|4:
  //|  mov NODE:eax, NODE:eax->i_key.nk.next	// Get next key in chain.
  //|  test NODE:eax, NODE:eax
  //|  jnz <1					// Loop if non-NULL.
  //|
  //|// Key not found. Add a new one, but check metatable first.
  //|  mov TABLE:ecx, TABLE:edi->metatable
  //|  test TABLE:ecx, TABLE:ecx
  //|  jz >5					// No metatable?
  //|  test byte TABLE:ecx->flags, 1<<TM_NEWINDEX
  //|  jz >7					// Or 'no __newindex' flag set?
  //|
  //|5:  // Add new key.
  //|  // No need for setting L->savedpc since only LUA_ERRMEM may be thrown.
  //|  lea TVALUE:eax, L->env
  //|  setsvalue TVALUE:eax[0], TSTRING:edx
  //|  sub esp, FRAME_OFFSET
  //|  call &luaH_newkey, L, TABLE:edi, TVALUE:eax
  //|  add esp, FRAME_OFFSET
  //|  jmp <2  // Copy to the returned value. See Node/TValue assumption above.
  //|
  //|6:  // Key found, but previous value is nil.
  //|  mov TABLE:ecx, TABLE:edi->metatable
  dasm_put(Dst, 2996, Dt1(->base), Dt10(->i_key.nk.next), DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, Dt1(->env), Dt7([0].value), Dt7([0].tt), (ptrdiff_t)(luaH_newkey));
# 1283 "ljit_x86.dasc"
  //|  test TABLE:ecx, TABLE:ecx
  //|  jz <2					// No metatable?
  //|  test byte TABLE:ecx->flags, 1<<TM_NEWINDEX
  //|  jnz <2					// Or 'no __newindex' flag set?
  //|
  //|7:  // Otherwise chain to C code which eventually calls luaV_settable.
  //|  setsvalue L->env, TSTRING:edx		// Use L->env as temp key.
  //|  mov ecx, [esp]
  //|  sub esp, FRAME_OFFSET
  //|  mov L->savedpc, ecx
  //|  call &jit_settable_fb, L, TABLE:edi, BASE
  //|  add esp, FRAME_OFFSET
  //|  mov BASE, L->base
  //|  ret
  //|.endjsub
  dasm_put(Dst, 3078, DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, Dt1(->env.value), Dt1(->env.tt), Dt1(->savedpc), (ptrdiff_t)(jit_settable_fb), Dt1(->base));
  //|//-----------------------------------------------------------------------
  //|//->SETTABLE_STR:
# 1301 "ljit_x86.dasc"
  //|// Call with: TOP (tab), TVALUE:ecx (key), BASE (val)
  //|  mov eax, TOP->tt; shl eax, 4; or eax, TVALUE:ecx->tt
  //|  cmp eax, LUA_TTABLE_STR
  //|   mov TABLE:edi, TOP->value
  //|   mov TSTRING:edx, TVALUE:ecx->value
  //|  je <9					// Types ok? Continue above.
  //|  jmp ->DEOPTIMIZE_CALLER		// Otherwise deoptimize.
  //|.endjsub
  dasm_put(Dst, 3139, Dt3(->tt), Dt7(->tt), Dt3(->value), Dt7(->value));
  //|//-----------------------------------------------------------------------
  //|//->GETTABLE_KNUM:
# 1475 "ljit_x86.dasc"
  //|  mov TMP2, ecx				// Save k.
  //|  sub esp, FRAME_OFFSET
  //|  call &luaH_getnum, TABLE:edi, ecx
  //|  add esp, FRAME_OFFSET
  //|  mov ecx, TMP2				// Restore k.
  //|  TValuemul ecx
  //|  sub TVALUE:eax, ecx		// Compensate for TVALUE:eax[k-1].
  //|  add TVALUE:eax, #TVALUE
  //|  ret
  //|.endjsub
  dasm_put(Dst, 3450, (ptrdiff_t)(luaH_getnum), sizeof(TValue));
  //|//-----------------------------------------------------------------------
  //|//->GETTABLE_NUM:
# 1487 "ljit_x86.dasc"
  //|  inc eax
  //|  mov ARG2, TABLE:edi			// Really ARG1 and ARG2.
  //|  mov ARG3, eax
  //|  jmp &luaH_getnum				// Chain to C code.
  //|.endjsub
  dasm_put(Dst, 3488, (ptrdiff_t)(luaH_getnum));
  //|//-----------------------------------------------------------------------
  //|//->SETTABLE_KNUM:
# 1571 "ljit_x86.dasc"
  //|  mov TMP2, ecx				// Save k.
  //|  sub esp, FRAME_OFFSET
  //|  call &luaH_setnum, L, TABLE:edi, ecx
  //|  add esp, FRAME_OFFSET
  //|  mov ecx, TMP2				// Restore k.
  //|  TValuemul ecx
  //|  sub TVALUE:eax, ecx		// Compensate for TVALUE:eax[k-1].
  //|  add TVALUE:eax, #TVALUE
  //|  ret
  //|.endjsub
  dasm_put(Dst, 3647, (ptrdiff_t)(luaH_setnum), sizeof(TValue));
  //|//-----------------------------------------------------------------------
  //|//->SETTABLE_NUM:
# 1583 "ljit_x86.dasc"
  //|  inc eax
  //|  mov ARG2, L				// Really ARG1, ARG2 and ARG3.
  //|  mov ARG3, TABLE:edi
  //|  mov ARG4, eax
  //|  jmp &luaH_setnum				// Chain to C code.
  //|.endjsub
  dasm_put(Dst, 3689, (ptrdiff_t)(luaH_setnum));
      //|//-----------------------------------------------------------------------
      //|//->LOG2_TWORD:
# 1777 "ljit_x86.dasc"
      //|// Called with (int *ptr, double k).
      //|  fld1; fld FPARG2			// Offset ok due to retaddr.
      //|  fyl2x
      //|  mov eax, ARG2				// Really ARG1.
      //|  fstp tword [eax]
      //|  ret
      //|.endjsub
      dasm_put(Dst, 4016);
  //|//-----------------------------------------------------------------------
  //|//->CONCAT_STR2:
# 2025 "ljit_x86.dasc"
  //|// Call with: BASE (first). Destroys all regs. L and BASE restored.
  //|  mov ARG2, L			// Save L (esi).
  //|  mov eax, BASE[0].tt; shl eax, 4; or eax, BASE[1].tt
  //|  sub eax, LUA_TSTR_STR		// eax = 0 on success.
  //|  jne ->DEOPTIMIZE_CALLER	// Wrong types? Deoptimize.
  //|
  //|1:
  //|   mov GL:edi, L->l_G
  //|  mov TSTRING:esi, BASE[0].value	// Caveat: L (esi) is gone now!
  //|  mov TSTRING:edx, BASE[1].value
  //|  mov ecx, TSTRING:esi->tsv.len	// size_t
  //|  test ecx, ecx
  //|  jz >2				// 1st string is empty?
  //|  or eax, TSTRING:edx->tsv.len	// eax is known to be zero.
  //|  jz >4				// 2nd string is empty?
  //|  add eax, ecx
  //|  jc >9				// Length overflow?
  //|  cmp eax, GL:edi->buff.buffsize	// size_t
  //|  ja >5				// Temp buffer overflow?
  //|  mov edi, GL:edi->buff.buffer
  //|  add esi, #TSTRING
  //|  rep; movsb				// Copy first string.
  dasm_put(Dst, 4349, Dt2([0].tt), Dt2([1].tt), Dt1(->l_G), Dt2([0].value), Dt2([1].value), DtB(->tsv.len), DtB(->tsv.len), Dt6(->buff.buffsize), Dt6(->buff.buffer), sizeof(TString));
# 2047 "ljit_x86.dasc"
  //|  mov ecx, TSTRING:edx->tsv.len
  //|  lea esi, TSTRING:edx[1]
  //|  rep; movsb				// Copy second string.
  //|
  //|  sub edi, eax			// start = end - total.
  //|   mov L, ARG2			// Restore L (esi). Reuse as 1st arg.
  //|  mov ARG3, edi
  //|  mov ARG4, eax
  //|   mov BASE, L->base			// Restore BASE.
  //|  jmp &luaS_newlstr
  //|
  //|2:  // 1st string is empty.
  //|  mov eax, TSTRING:edx		// Return 2nd string.
  //|3:
  //|  mov L, ARG2			// Restore L (esi) and BASE.
  //|  mov BASE, L->base
  //|  ret
  //|
  //|4:  // 2nd string is empty.
  //|  mov eax, TSTRING:esi		// Return 1st string.
  //|  jmp <3
  //|
  //|5:  // Resize temp buffer.
  //|  // No need for setting L->savedpc since only LUA_ERRMEM may be thrown.
  //|  mov L, ARG2			// Restore L.
  //|  lea ecx, GL:edi->buff
  //|  sub esp, FRAME_OFFSET
  //|  call &luaZ_openspace, L, ecx, eax
  //|  add esp, FRAME_OFFSET
  //|  xor eax, eax			// BASE (first) and L saved. eax = 0.
  //|  jmp <1				// Just restart.
  //|
  //|9:  // Length overflow errors are rare (> 2 GB string required).
  //|  mov L, ARG2			// Need L for deoptimization.
  //|  jmp ->DEOPTIMIZE_CALLER
  //|.endjsub
  dasm_put(Dst, 4420, DtB(->tsv.len), DtB([1]), Dt1(->base), (ptrdiff_t)(luaS_newlstr), Dt1(->base), Dt6(->buff), (ptrdiff_t)(luaZ_openspace));
# 297 "ljit_x86.dasc"
  //|
  //|// Uncritical jsubs follow. No need to align them.
  //|//-----------------------------------------------------------------------
  //|->DEOPTIMIZE_CALLER:			// Deoptimize calling instruction.
  //|  pop edx
  //|  jmp ->DEOPTIMIZE
  //|
  //|->DEOPTIMIZE_OPEN:			// Deoptimize open instruction.
  //|  mov L->top, TOP			// Save TOP.
  //|
  //|->DEOPTIMIZE:			// Deoptimize instruction.
  //|  mov L->savedpc, edx		// &J->nextins expected in edx.
  //|  call &luaJIT_deoptimize, L
  //|  mov BASE, L->base
  //|  mov TOP, L->top			// Restore TOP for open ins.
  //|  jmp eax				// Continue with new mcode addr.
  //|
  //|  .align 16
  //|//-----------------------------------------------------------------------
  dasm_put(Dst, 561, Dt1(->top), Dt1(->savedpc), (ptrdiff_t)(luaJIT_deoptimize), Dt1(->base), Dt1(->top));
# 316 "ljit_x86.dasc"

  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &J->jsubmcode, &J->szjsubmcode);
  if (status != JIT_S_OK)
    return status;

  /* Copy the callgates from the globals to the global state. */
  G(J->L)->jit_gateLJ = (luaJIT_GateLJ)J->jsub[JSUB_GATE_LJ];
  G(J->L)->jit_gateJL = (lua_CFunction)J->jsub[JSUB_GATE_JL];
  G(J->L)->jit_gateJC = (lua_CFunction)J->jsub[JSUB_GATE_JC];
  return JIT_S_OK;
}

/* Match with number of nops above. Avoid confusing the instruction decoder. */
#define DEBUGPATCH_SIZE		6

/* Notify backend that the debug mode may have changed. */
void luaJIT_debugnotify(jit_State *J)
{
  unsigned char *patch = (unsigned char *)J->jsub[JSUB_GATE_JC_PATCH];
  unsigned char *target = (unsigned char *)J->jsub[JSUB_GATE_JC_DEBUG];
  /* Yep, this is self-modifying code -- don't tell anyone. */
  if (patch[0] == 0xe9) {  /* Debug patch is active. */
    if (!(J->flags & JIT_F_DEBUG_CALL))  /* Deactivate it. */
      memcpy(patch, target-DEBUGPATCH_SIZE, DEBUGPATCH_SIZE);
  } else {  /* Debug patch is inactive. */
    if (J->flags & JIT_F_DEBUG_CALL) {  /* Activate it. */
      int rel = target-(patch+5);
      memcpy(target-DEBUGPATCH_SIZE, patch, DEBUGPATCH_SIZE);
      patch[0] = 0xe9;  /* jmp */
      memcpy(patch+1, &rel, 4);  /* Relative address. */
      memset(patch+5, 0x90, DEBUGPATCH_SIZE-5);  /* nop */
    }
  }
}

/* Patch a jmp into existing mcode. */
static void jit_patch_jmp(jit_State *J, void *mcode, void *to)
{
  unsigned char *patch = (unsigned char *)mcode;
  int rel = ((unsigned char *)to)-(patch+5);
  patch[0] = 0xe9;  /* jmp */
  memcpy((void *)(patch+1), &rel, 4);  /* Relative addr. */
}

/* ------------------------------------------------------------------------ */

/* Call line/count hook. */
static void jit_hookins(lua_State *L, const Instruction *newpc)
{
  Proto *pt = ci_func(L->ci)->l.p;
  int pc = luaJIT_findpc(pt, newpc);  /* Sloooow with mcode addrs. */
  const Instruction *savedpc = L->savedpc;
  L->savedpc = pt->code + pc + 1;
  if (L->hookmask > LUA_MASKLINE && L->hookcount == 0) {
    resethookcount(L);
    luaD_callhook(L, LUA_HOOKCOUNT, -1);
  }
  if (L->hookmask & LUA_MASKLINE) {
    int newline = getline(pt, pc);
    if (pc != 0) {
      int oldpc = luaJIT_findpc(pt, savedpc);
      if (oldpc < 0) return;
      if (!(pc <= oldpc || newline != getline(pt, oldpc))) return;
    }
    luaD_callhook(L, LUA_HOOKLINE, newline);
  }
}

/* Insert hook check for each instruction in full debug mode. */
static void jit_ins_debug(jit_State *J, int openop)
{
  if (openop) {
    //|  mov L->top, TOP
    dasm_put(Dst, 594, Dt1(->top));
# 390 "ljit_x86.dasc"
  }
  //|// TODO: Passing bytecode addrs would speed this up (but use more space).
  //|  call ->HOOKINS
  dasm_put(Dst, 598);
# 393 "ljit_x86.dasc"

  //|.jsub HOOKINS
# 411 "ljit_x86.dasc"
}

/* Called before every instruction. */
static void jit_ins_start(jit_State *J)
{
  //|// Always emit PC labels, even for dead code (but not for combined JMP).
  //|=>J->nextpc:
  dasm_put(Dst, 663, J->nextpc);
# 418 "ljit_x86.dasc"
}

/* Chain to another instruction. */
static void jit_ins_chainto(jit_State *J, int pc)
{
  //|  jmp =>pc
  dasm_put(Dst, 665, pc);
# 424 "ljit_x86.dasc"
}

/* Set PC label. */
static void jit_ins_setpc(jit_State *J, int pc, void *target)
{
  //|.label =>pc, &target
  dasm_put(Dst, 668, pc, (ptrdiff_t)(target));
# 430 "ljit_x86.dasc"
}

/* Called after the last instruction has been encoded. */
static void jit_ins_last(jit_State *J, int lastpc, int sizemfm)
{
  if (J->tflags & JIT_TF_USED_DEOPT) {  /* Deopt section has been used? */
    //|.deopt
    dasm_put(Dst, 671);
# 437 "ljit_x86.dasc"
    //|  jmp ->DEOPTIMIZE			// Yes, need to add final jmp.
    //|.code
    dasm_put(Dst, 673);
# 439 "ljit_x86.dasc"
  }
  //|=>lastpc+1:				// Extra label at the end of .code.
  //|.tail
  dasm_put(Dst, 678, lastpc+1);
# 442 "ljit_x86.dasc"
  //|=>lastpc+2:				// And at the end of .deopt/.tail.
  //|  .align word			// Keep next section word aligned.
  //|  .word 0xffff			// Terminate mfm with JIT_MFM_STOP.
  //|.mfmap
  dasm_put(Dst, 681, lastpc+2);
# 446 "ljit_x86.dasc"
  //|  // <-- Deoptimization hints are inserted here.
  //|  .space sizemfm			// To be filled in with inverse mfm.
  //|  .aword 0, 0			// Next mcode block pointer and size.
  //|  // The previous two awords are only word, but not aword aligned.
  //|  // Copying them is easier than aligning them and adjusting mfm handling.
  //|.code
  dasm_put(Dst, 690, sizemfm);
# 452 "ljit_x86.dasc"
}

/* Add a deoptimize target for the current instruction. */
static void jit_deopt_target(jit_State *J, int nargs)
{
  //|.define L_DEOPTLABEL, 9		// Local deopt label.
  //|.define L_DEOPTIMIZE, <9		// Local deopt target. Use after call.
  //|.define L_DEOPTIMIZEF, >9		// Local deopt target. Use before call.
  if (nargs != -1) {
    //|// Alas, x86 doesn't have conditional calls. So branch to the .deopt
    //|// section to load J->nextins and jump to JSUB_DEOPTIMIZE.
    //|// Only a single jump is added at the end (if needed) and any
    //|// intervening code sequences are shadowed (lea trick).
    //|.deopt				// Occupies 6 bytes in .deopt section.
    dasm_put(Dst, 671);
# 466 "ljit_x86.dasc"
    //|  .byte 0x8d			// Shadow mov with lea edi, [edx+ofs].
    //|L_DEOPTLABEL:
    //|  mov edx, &J->nextins		// Current instruction + 1.
    //|.code
    dasm_put(Dst, 702, (ptrdiff_t)(J->nextins));
# 470 "ljit_x86.dasc"
    J->tflags |= JIT_TF_USED_DEOPT;
  } else {
    //|.tail				// Occupies 10 bytes in .tail section.
    dasm_put(Dst, 679);
# 473 "ljit_x86.dasc"
    //|L_DEOPTLABEL:
    //|  mov edx, &J->nextins
    //|  jmp ->DEOPTIMIZE_OPEN		// Open ins need to save TOP, too.
    //|  // And TOP (edi) would be overwritten by the lea trick.
    //|  // So checking for open ops later on wouldn't suffice. Sigh.
    //|.code
    dasm_put(Dst, 709, (ptrdiff_t)(J->nextins));
# 479 "ljit_x86.dasc"
  }
}

/* luaC_checkGC() inlined. Destroys caller-saves + TOP (edi). Uses label 7:. */
/* Use this only at the _end_ of an instruction. */
static void jit_checkGC(jit_State *J)
{
  //|  mov GL:ecx, L->l_G
  //|  mov eax, GL:ecx->totalbytes	// size_t
  //|  mov TOP, >7
  //|  cmp eax, GL:ecx->GCthreshold	// size_t
  //|  jae ->GCSTEP
  //|7:
  dasm_put(Dst, 718, Dt1(->l_G), Dt6(->totalbytes), Dt6(->GCthreshold));
# 492 "ljit_x86.dasc"

  //|.jsub GCSTEP
# 498 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

//|// JIT->JIT calling conventions:
//|//
//|//  Register/Type | Call Setup   | Prologue     | Epilogue     | Call Finish
//|// ===========================================================================
//|//  eax | LCL     | = BASE->value|              | *            | *
//|//  ecx | CI      | = L->ci      | L->ci = ++CI | *            | *
//|//  edx | *       | *            | *            | *            | *
//|// ---------------------------------------------------------------------------
//|//  esi | L       |              |              |              |
//|//  ebx | BASE    | += f         | ++           | --           | -= f
//|//  edi | TOP     | += f+1+nargs | = BASE+maxst | = f+nresults | = BASE+maxst
//|// ---------------------------------------------------------------------------
//|//  L->base       |              | = BASE       |              | = BASE
//|//  L->top        |              | = TOP        |              | = TOP
//|//  L->ci         |              | ++, -> = ... | --           |
//|//  L->ci->savedpc| = &code[pc]  | [ L-> = ]    |              |
//|// ---------------------------------------------------------------------------
//|//  args + vars   |              | setnil       |              |
//|//  results       |              |              | move         | setnil
//|// ---------------------------------------------------------------------------


//|// Include support for function inlining.
//|.include ljit_x86_inline.dash
# 1 "ljit_x86_inline.dash"
/*
** Function inlining support for x86 CPUs.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

/* ------------------------------------------------------------------------ */

/* Private structure holding function inlining info. */
typedef struct jit_InlineInfo {
  int func;			/* Function slot. 1st arg slot = func+1. */
  int res;			/* 1st result slot. Overlaps func/ci->func. */
  int nargs;			/* Number of args. */
  int nresults;			/* Number of results. */
  int xnargs;			/* Expected number of args. */
  int xnresults;		/* Returned number of results. */
  int hidx;			/* Library/function index numbers. */
} jit_InlineInfo;

/* ------------------------------------------------------------------------ */

enum { TFOR_FUNC, TFOR_TAB, TFOR_CTL, TFOR_KEY, TFOR_VAL };

static void jit_inline_base(jit_State *J, jit_InlineInfo *ii)
{
  int func = ii->func;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_BASE_PAIRS:
  case JIT_IH_BASE_IPAIRS:
    //|// Easy for regular calls: res == func. Not inlined for tailcalls.
    //|// Guaranteed to be inlined only if used in conjunction with TFORLOOP.
    //|// So we omit setting the iterator function and fake the control var.
    //|  istable func+TFOR_TAB; jne L_DEOPTIMIZE	// Caveat: deopt TFORLOOP, too!
    //|  xor eax, eax				// Assumes: LUA_TNIL == 0.
    //|  mov BASE[func+TFOR_CTL].tt, eax		// Fake nil type.
    //|  mov BASE[func+TFOR_CTL].value, eax	// Hidden control var = 0.
    //|//  mov BASE[func+TFOR_FUNC].tt, eax	// Kill function (not needed).
    //|.mfmap
    dasm_put(Dst, 753, Dt2([func+TFOR_TAB].tt), Dt2([func+TFOR_CTL].tt), Dt2([func+TFOR_CTL].value));
# 38 "ljit_x86_inline.dash"
    //|  .word JIT_MFM_DEOPT_PAIRS, J->nextpc-1	// Deoptimize TFORLOOP, too.
    //|.code
    dasm_put(Dst, 771, JIT_MFM_DEOPT_PAIRS, J->nextpc-1);
# 40 "ljit_x86_inline.dash"
    break;
  default:
    jit_assert(0);
    break;
  }
}

/* ------------------------------------------------------------------------ */

#ifndef COCO_DISABLE

/* Helper function for inlined coroutine.resume(). */
static StkId jit_coroutine_resume(lua_State *L, StkId base, int nresults)
{
  lua_State *co = thvalue(base-1);
  /* Check for proper usage. Merge of lua_resume() and auxresume() checks. */
  if (co->status != LUA_YIELD) {
    if (co->status > LUA_YIELD) {
errdead:
      setsvalue(L, base-1, luaS_newliteral(L, "cannot resume dead coroutine"));
      goto err;
    } else if (co->ci != co->base_ci) {
      setsvalue(L, base-1,
	luaS_newliteral(L, "cannot resume non-suspended coroutine"));
      goto err;
    } else if (co->base == co->top) {
      goto errdead;
    }
  }
  {
    unsigned int ndelta = (char *)L->top - (char *)base;
    int nargs = ndelta/sizeof(TValue);  /* Compute nargs. */
    int status;
    if ((char *)co->stack_last-(char *)co->top <= ndelta) {
      co->ci->top = (StkId)(((char *)co->top) + ndelta);  /* Ok before grow. */
      luaD_growstack(co, nargs);  /* Grow thread stack. */
    }
    /* Copy args. */
    co->top = (StkId)(((char *)co->top) + ndelta);
    { StkId t = co->top, f = L->top; while (f > base) setobj2s(co, --t, --f); }
    L->top = base;
    status = luaCOCO_resume(co, nargs);  /* Resume Coco thread. */
    if (status == 0 || status == LUA_YIELD) {  /* Ok. */
      StkId f;
      if (nresults == 0) return NULL;
      if (nresults == -1) {
	luaD_checkstack(L, co->top - co->base);  /* Grow own stack. */
      }
      base = L->top - 2;
      setbvalue(base++, 1);  /* true */
      /* Copy results. Fill unused result slots with nil. */
      f = co->base;
      while (--nresults != 0 && f < co->top) setobj2s(L, base++, f++);
      while (nresults-- > 0) setnilvalue(base++);
      co->top = co->base;
      return base;
    } else {  /* Error. */
      base = L->top;
      setobj2s(L, base-1, co->top-1);  /* Copy error object. */
err:
      setbvalue(base-2, 0);  /* false */
      nresults -= 2;
      while (--nresults >= 0) setnilvalue(base+nresults);  /* Fill results. */
      return base;
    }
  }
}

static void jit_inline_coroutine(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  int i;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_COROUTINE_YIELD:
    //|  cmp aword [L+((int)&LHASCOCO((lua_State *)0))], 0  // Got a C stack?
    //|  je L_DEOPTIMIZE
    //|  mov L->savedpc, &J->nextins		// Debugger-friendly.
    //|  add BASE, arg*#TVALUE
    dasm_put(Dst, 775, ((int)&LHASCOCO((lua_State *)0)), Dt1(->savedpc), (ptrdiff_t)(J->nextins), arg*sizeof(TValue));
# 119 "ljit_x86_inline.dash"
    if (ii->nargs >= 0) {  /* Previous op was not open and did not set TOP. */
      //|  lea TOP, BASE[ii->nargs]
      dasm_put(Dst, 791, Dt2([ii->nargs]));
# 121 "ljit_x86_inline.dash"
    }
    //|  mov L->base, BASE
    //|  mov L->top, TOP
    //|  call &luaCOCO_yield, L
    //|  mov BASE, L->base
    //|  mov TOP, L->top
    dasm_put(Dst, 795, Dt1(->base), Dt1(->top), (ptrdiff_t)(luaCOCO_yield), Dt1(->base), Dt1(->top));
# 127 "ljit_x86_inline.dash"
    jit_assert(ii->nresults >= 0 && ii->nresults <= EXTRA_STACK);
    for (i = 0; i < ii->nresults; i++) {
      //|  setnilvalue TOP[i]			// Clear undefined result.
      //|  copyslot BASE[res+i], BASE[arg+i]	// Move result down.
      dasm_put(Dst, 813, Dt3([i].tt));
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([arg+i].tt), Dt2([arg+i].value), Dt2([res+i].tt), Dt2([res+i].value));
      } else {
      dasm_put(Dst, 839, Dt2([arg+i].value), Dt2([arg+i].value.na[1]), Dt2([arg+i].tt), Dt2([res+i].value), Dt2([res+i].value.na[1]), Dt2([res+i].tt));
      }
# 131 "ljit_x86_inline.dash"
    }
    ii->nargs = -1;  /* Force restore of L->top. */
    break;
  case JIT_IH_COROUTINE_RESUME:
    jit_assert(ii->nargs != 0 && ii->res == ii->func);
    //|  add BASE, (arg+1)*#TVALUE
    dasm_put(Dst, 787, (arg+1)*sizeof(TValue));
# 137 "ljit_x86_inline.dash"
    if (ii->nargs >= 0) {  /* Previous op was not open and did not set TOP. */
      //|  lea TOP, BASE[ii->nargs-1]
      dasm_put(Dst, 791, Dt2([ii->nargs-1]));
# 139 "ljit_x86_inline.dash"
    } else {
      //|  cmp TOP, BASE; jb L_DEOPTIMIZE		// No thread arg? Deoptimize.
      dasm_put(Dst, 858);
# 141 "ljit_x86_inline.dash"
    }
    //|  istt -1, LUA_TTHREAD; jne L_DEOPTIMIZE	// Wrong type? Deoptimize.
    //|  mov L:eax, BASE[-1].value
    //|  cmp aword [L:eax+((int)&LHASCOCO((lua_State *)0))], 0
    //|  je L_DEOPTIMIZE				// No C stack? Deoptimize.
    //|  mov L->savedpc, &J->nextins		// Debugger-friendly.
    //|  mov L->top, TOP
    //|  call &jit_coroutine_resume, L, BASE, ii->nresults
    //|  mov BASE, L->base
    dasm_put(Dst, 865, Dt2([-1].tt), Dt2([-1].value), ((int)&LHASCOCO((lua_State *)0)), Dt1(->savedpc), (ptrdiff_t)(J->nextins), Dt1(->top), ii->nresults, (ptrdiff_t)(jit_coroutine_resume), Dt1(->base));
# 150 "ljit_x86_inline.dash"
    if (ii->nresults == -1) {
      //|  mov TOP, eax
      dasm_put(Dst, 909);
# 152 "ljit_x86_inline.dash"
    }
    ii->nargs = -1;  /* Force restore of L->top. */
    break;
  default:
    jit_assert(0);
    break;
  }
}

#endif /* COCO_DISABLE */

/* ------------------------------------------------------------------------ */

static void jit_inline_string(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_STRING_LEN:
    //|  isstring arg; jne L_DEOPTIMIZE
    //|  mov TSTRING:ecx, BASE[arg].value
    //|  fild aword TSTRING:ecx->tsv.len	// size_t
    //|  settt BASE[res], LUA_TNUMBER
    //|  fstp qword BASE[res].value
    dasm_put(Dst, 912, Dt2([arg].tt), Dt2([arg].value), DtB(->tsv.len), Dt2([res].tt), Dt2([res].value));
# 176 "ljit_x86_inline.dash"
    break;
  case JIT_IH_STRING_SUB:
    /* TODO: inline numeric constants with help from the optimizer. */
    /*       But this would save only another 15-20% in a trivial loop. */
    jit_assert(ii->nargs >= 2);  /* Open op caveat is ok, too. */
    if (ii->nargs > 2) {
      //|  lea TOP, BASE[arg]
      //|  call ->STRING_SUB3
      //|  setsvalue BASE[res], eax
      dasm_put(Dst, 937, Dt2([arg]), Dt2([res].value), Dt2([res].tt));
# 185 "ljit_x86_inline.dash"
    } else {
      //|  lea TOP, BASE[arg]
      //|  call ->STRING_SUB2
      //|  setsvalue BASE[res], eax
      dasm_put(Dst, 954, Dt2([arg]), Dt2([res].value), Dt2([res].tt));
# 189 "ljit_x86_inline.dash"
    }
    break;
  case JIT_IH_STRING_CHAR:
    //|  isnumber arg; jne L_DEOPTIMIZE
    //|  lea eax, L->env			// Abuse L->env to hold temp string.
    //|  fld qword BASE[arg].value
    //|  fistp dword [eax]		// LSB is at start (little-endian).
    //|  cmp dword [eax], 255; ja L_DEOPTIMIZE
    //|  call &luaS_newlstr, L, eax, 1
    //|  setsvalue BASE[res], eax
    dasm_put(Dst, 971, Dt2([arg].tt), Dt1(->env), Dt2([arg].value), (ptrdiff_t)(luaS_newlstr), Dt2([res].value), Dt2([res].tt));
# 199 "ljit_x86_inline.dash"
    break;
  default:
    jit_assert(0);
    break;
  }

  //|//-----------------------------------------------------------------------
  //|.jsub STRING_SUB3			// string.sub(str, start, end)
# 259 "ljit_x86_inline.dash"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub STRING_SUB2			// string.sub(str, start)
# 279 "ljit_x86_inline.dash"
}

/* ------------------------------------------------------------------------ */

/* Helper functions for inlined calls to table.*. */
static void jit_table_insert(lua_State *L, TValue *arg)
{
  setobj2t(L, luaH_setnum(L, hvalue(arg), luaH_getn(hvalue(arg))+1), arg+1);
  luaC_barriert(L, hvalue(arg), arg+1);
}

static TValue *jit_table_remove(lua_State *L, TValue *arg, TValue *res)
{
  int n = luaH_getn(hvalue(arg));
  if (n == 0) {
    setnilvalue(res);  /* For the nresults == 1 case. Harmless otherwise. */
    return res;  /* For the nresults == -1 case. */
  } else {
    TValue *val = luaH_setnum(L, hvalue(arg), n);
    setobj2s(L, res, val);
    setnilvalue(val);
    return res+1;  /* For the nresults == -1 case. */
  }
}

static void jit_inline_table(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  //|  istable arg; jne L_DEOPTIMIZE
  dasm_put(Dst, 1250, Dt2([arg].tt));
# 309 "ljit_x86_inline.dash"
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_TABLE_INSERT:
    jit_assert(ii->nargs == 2);
    //|  lea TVALUE:eax, BASE[arg]
    //|  call &jit_table_insert, L, TVALUE:eax
    dasm_put(Dst, 1259, Dt2([arg]), (ptrdiff_t)(jit_table_insert));
# 314 "ljit_x86_inline.dash"
    break;
  case JIT_IH_TABLE_REMOVE:
    jit_assert(ii->nargs == 1);
    //|  lea TVALUE:eax, BASE[arg]
    //|  lea TVALUE:ecx, BASE[res]
    //|  call &jit_table_remove, L, TVALUE:eax, TVALUE:ecx
    dasm_put(Dst, 1272, Dt2([arg]), Dt2([res]), (ptrdiff_t)(jit_table_remove));
# 320 "ljit_x86_inline.dash"
    if (ii->nresults == -1) {
      ii->xnresults = -1;
      //|  mov TOP, TVALUE:eax
      dasm_put(Dst, 909);
# 323 "ljit_x86_inline.dash"
    }
    break;
  case JIT_IH_TABLE_GETN:
    //|  mov TABLE:eax, BASE[arg].value
    //|  call &luaH_getn, TABLE:eax
    //|  mov TMP1, eax
    //|  fild dword TMP1
    //|  fstp qword BASE[res].value
    //|  settt BASE[res], LUA_TNUMBER
    dasm_put(Dst, 1292, Dt2([arg].value), (ptrdiff_t)(luaH_getn), Dt2([res].value), Dt2([res].tt));
# 332 "ljit_x86_inline.dash"
    break;
  default:
    jit_assert(0);
    break;
  }
}

/* ------------------------------------------------------------------------ */

/* This typedef must match the libm function signature. */
/* Serves as a check against wrong lua_Number or wrong calling conventions. */
typedef lua_Number (*mathfunc_11)(lua_Number);

/* Partially inlined math functions. */
/* CHECK: must match with jit_hints.h and jit.opt_lib. */
static const mathfunc_11 jit_mathfuncs_11[JIT_IH_MATH_SIN] = {
  log, log10, exp,	sinh, cosh, tanh,	asin, acos, atan
};

/* FPU control words for ceil and floor (exceptions masked, full precision). */
static const unsigned short jit_fpucw[2] = { 0x0b7f, 0x077f };

static void jit_inline_math(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  int idx = JIT_IH_IDX(ii->hidx);

  if (idx < JIT_IH_MATH__21) {
    //|  isnumber arg; jne L_DEOPTIMIZE
    //|  fld qword BASE[arg].value
    dasm_put(Dst, 1317, Dt2([arg].tt), Dt2([arg].value));
# 363 "ljit_x86_inline.dash"
  } else {
    jit_assert(idx < JIT_IH_MATH__LAST);
    //|  isnumber2 arg, arg+1; jne L_DEOPTIMIZE
    dasm_put(Dst, 1329, Dt2([arg].tt), Dt2([arg+1].tt));
# 366 "ljit_x86_inline.dash"
  }
  switch (idx) {
  /* We ignore sin/cos/tan range overflows (2^63 rad) just like -ffast-math. */
  case JIT_IH_MATH_SIN:
    //|  fsin
    dasm_put(Dst, 1347);
# 371 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_COS:
    //|  fcos
    dasm_put(Dst, 1351);
# 374 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_TAN:
    //|  fptan; fpop
    dasm_put(Dst, 1355);
# 377 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_CEIL:
  case JIT_IH_MATH_FLOOR:
    //|  fnstcw word TMP1
    //|  fldcw word [(ptrdiff_t)&jit_fpucw[idx-JIT_IH_MATH_CEIL]]
    //|  frndint
    //|  fldcw word TMP1
    dasm_put(Dst, 1361, (ptrdiff_t)&jit_fpucw[idx-JIT_IH_MATH_CEIL]);
# 384 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_ABS:
    //|  fabs
    dasm_put(Dst, 1374);
# 387 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_SQRT:
    //|  fsqrt
    dasm_put(Dst, 1377);
# 390 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_FMOD:
    //|  fld qword BASE[arg+1].value
    //|  fld qword BASE[arg].value
    //|1: ; fprem; fnstsw ax; sahf; jp <1
    //|  fstp st1
    dasm_put(Dst, 1381, Dt2([arg+1].value), Dt2([arg].value));
# 396 "ljit_x86_inline.dash"
    break;
  case JIT_IH_MATH_ATAN2:
    //|// Inlining is easier than calling atan2().
    //|  fld qword BASE[arg].value
    //|  fld qword BASE[arg+1].value
    //|  fpatan
    dasm_put(Dst, 1402, Dt2([arg].value), Dt2([arg+1].value));
# 402 "ljit_x86_inline.dash"
    break;
  default:
    //|// Partially inlined. Just call the libm function (__cdecl!).
    //|  fstp FPARG1
    //|  call &jit_mathfuncs_11[idx]
    dasm_put(Dst, 1412, (ptrdiff_t)(jit_mathfuncs_11[idx]));
# 407 "ljit_x86_inline.dash"
    break;
  }
  //|  settt BASE[res], LUA_TNUMBER
  //|  fstp qword BASE[res].value
  dasm_put(Dst, 926, Dt2([res].tt), Dt2([res].value));
# 411 "ljit_x86_inline.dash"
}

/* ------------------------------------------------------------------------ */

/* Try to inline a CALL or TAILCALL instruction. */
static int jit_inline_call(jit_State *J, int func, int nargs, int nresults)
{
  const TValue *callable = hint_get(J, TYPE);  /* TYPE hint = callable. */
  int cltype = ttype(callable);
  const TValue *oidx;
  jit_InlineInfo ii;
  int idx;

  if (cltype != LUA_TFUNCTION) goto fail;
  if (J->flags & JIT_F_DEBUG) goto fail;  /* DWIM. */

  oidx = hint_get(J, INLINE);  /* INLINE hint = library/function index. */
  if (!ttisnumber(oidx)) goto fail;

  ii.hidx = (int)nvalue(oidx);
  idx = JIT_IH_IDX(ii.hidx);

  if (nresults == -2) {  /* Tailcall. */
    /* Tailcalls from vararg functions don't work with BASE[-1]. */
    if (J->pt->is_vararg) goto fail;  /* So forget about this rare case. */
    ii.res = -1;  /* Careful: 2nd result overlaps 1st stack slot. */
    ii.nresults = -1;
  } else {
    ii.res = func;
    ii.nresults = nresults;
  }
  ii.func = func;
  ii.nargs = nargs;
  ii.xnargs = ii.xnresults = 1;  /* Default: 1 arg, 1 result. */

  /* Check for the currently supported cases. */
  switch (JIT_IH_LIB(ii.hidx)) {
  case JIT_IHLIB_BASE:
    switch (idx) {
    case JIT_IH_BASE_PAIRS:
    case JIT_IH_BASE_IPAIRS:
      if (nresults == -2) goto fail;  /* Not useful for tailcalls. */
      ii.xnresults = 3;
      goto check;
    }
    break;
#ifndef COCO_DISABLE
  case JIT_IHLIB_COROUTINE:
    switch (idx) {
    case JIT_IH_COROUTINE_YIELD:
      /* Only support common cases: no tailcalls, low number of results. */
      if (nresults < 0 || nresults > EXTRA_STACK) goto fail;
      ii.xnargs = ii.xnresults = -1;
      goto ok;  /* Anything else is ok. */
    case JIT_IH_COROUTINE_RESUME:
      /* Only support common cases: no tailcalls, not with 0 args (error). */
      if (nresults == -2 || nargs == 0) goto fail;
      ii.xnargs = ii.xnresults = -1;
      goto ok;  /* Anything else is ok. */
    }
    break;
#endif
  case JIT_IHLIB_STRING:
    switch (idx) {
    case JIT_IH_STRING_LEN:
      goto check;
    case JIT_IH_STRING_SUB:
      if (nargs < 2) goto fail;  /* No support for open calls, too. */
      goto ok;  /* 2 or more args are ok. */
    case JIT_IH_STRING_CHAR:
      goto check;  /* Only single arg supported. */
    }
    break;
  case JIT_IHLIB_TABLE:
    switch (idx) {
    case JIT_IH_TABLE_INSERT:
      ii.xnargs = 2;
      goto check;  /* Only push (append) supported. */
    case JIT_IH_TABLE_REMOVE:
      goto check;  /* Only pop supported. */
    case JIT_IH_TABLE_GETN:
      goto check;
    }
    break;
  case JIT_IHLIB_MATH:
    if (idx >= JIT_IH_MATH__LAST) goto fail;
    if (idx >= JIT_IH_MATH__21) ii.xnargs = 2;
    goto check;
  }
fail:
  return cltype;  /* Call could not be inlined. Return type of callable. */

check:
  if (nargs != ii.xnargs && nargs != -1) goto fail;
  /* The optimizer already checks the number of results (avoid setnil). */

ok:  /* Whew, all checks done. Go for it! */

  /* Start with the common leadin for inlined calls. */
  jit_deopt_target(J, nargs);
  //|// Caveat: Must save TOP for open ops if jsub uses DEOPTIMIZE_CALLER.
  //|  isfunction func
  //|  jne L_DEOPTIMIZE			// Not a function? Deoptimize.
  //|  cmp aword BASE[func].value, &clvalue(callable)
  //|  jne L_DEOPTIMIZE			// Wrong closure? Deoptimize.
  dasm_put(Dst, 1418, Dt2([func].tt), Dt2([func].value), (ptrdiff_t)(clvalue(callable)));
# 516 "ljit_x86_inline.dash"
  if (nargs == -1 && ii.xnargs >= 0) {
    //|  lea eax, BASE[func+1+ii.xnargs]
    //|  cmp TOP, eax
    //|  jne L_DEOPTIMIZE			// Wrong #args? Deoptimize.
    dasm_put(Dst, 1435, Dt2([func+1+ii.xnargs]));
# 520 "ljit_x86_inline.dash"
  }

  /* Now inline the function itself. */
  switch (JIT_IH_LIB(ii.hidx)) {
  case JIT_IHLIB_BASE: jit_inline_base(J, &ii); break;
#ifndef COCO_DISABLE
  case JIT_IHLIB_COROUTINE: jit_inline_coroutine(J, &ii); break;
#endif
  case JIT_IHLIB_STRING: jit_inline_string(J, &ii); break;
  case JIT_IHLIB_TABLE:  jit_inline_table(J, &ii); break;
  case JIT_IHLIB_MATH:   jit_inline_math(J, &ii); break;
  default: jit_assert(0); break;
  }

  /* And add the common leadout for inlined calls. */
  if (ii.nresults == -1) {
    if (ii.xnresults >= 0) {
      //|  lea TOP, BASE[ii.res+ii.xnresults]
      dasm_put(Dst, 791, Dt2([ii.res+ii.xnresults]));
# 538 "ljit_x86_inline.dash"
    }
  } else if (ii.nargs == -1) {  /* Restore L->top only if needed. */
    //|  lea TOP, BASE[J->pt->maxstacksize]
    //|  mov L->top, TOP
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
# 542 "ljit_x86_inline.dash"
  }

  if (nresults == -2) {  /* Results are in place. Add return for tailcalls. */
    //|  add esp, FRAME_OFFSET
    //|  sub BASE, #BASE
    //|  sub aword L->ci, #CI
    //|  ret
    dasm_put(Dst, 1452, sizeof(TValue), Dt1(->ci), sizeof(CallInfo));
# 549 "ljit_x86_inline.dash"
  }

  return -1;  /* Success, call has been inlined. */
}

/* ------------------------------------------------------------------------ */

/* Helper function for inlined iterator code. Paraphrased from luaH_next. */
/* TODO: GCC has trouble optimizing this. */
static int jit_table_next(lua_State *L, TValue *ra)
{
  Table *t = hvalue(&ra[TFOR_TAB]);
  int i = ra[TFOR_CTL].value.b;  /* Hidden control variable. */
  for (; i < t->sizearray; i++) {  /* First the array part. */
    if (!ttisnil(&t->array[i])) {
      setnvalue(&ra[TFOR_KEY], cast_num(i+1));
      setobj2s(L, &ra[TFOR_VAL], &t->array[i]);
      ra[TFOR_CTL].value.b = i+1;
      return 1;
    }
  }
  for (i -= t->sizearray; i < sizenode(t); i++) {  /* Then the hash part. */
    if (!ttisnil(gval(gnode(t, i)))) {
      setobj2s(L, &ra[TFOR_KEY], key2tval(gnode(t, i)));
      setobj2s(L, &ra[TFOR_VAL], gval(gnode(t, i)));
      ra[TFOR_CTL].value.b = i+1+t->sizearray;
      return 1;
    }
  }
  return 0;  /* End of iteration. */
}

/* Try to inline a TFORLOOP instruction. */
static int jit_inline_tforloop(jit_State *J, int ra, int nresults, int target)
{
  const TValue *oidx = hint_get(J, INLINE);  /* INLINE hint = lib/func idx. */
  int idx;

  if (!ttisnumber(oidx)) return 0;  /* No hint: don't inline anything. */
  idx = (int)nvalue(oidx);
  if (J->flags & JIT_F_DEBUG) return 0;  /* DWIM. */

  switch (idx) {
  case JIT_IH_MKIDX(JIT_IHLIB_BASE, JIT_IH_BASE_PAIRS):
    //|// The type checks can be omitted -- see the iterator constructor.
    //|  lea TOP, BASE[ra]
    //|  call &jit_table_next, L, TOP
    //|  test eax, eax
    //|  jnz =>target
    dasm_put(Dst, 1465, Dt2([ra]), (ptrdiff_t)(jit_table_next), target);
# 598 "ljit_x86_inline.dash"
    return 1;  /* Success, iterator has been inlined. */
  case JIT_IH_MKIDX(JIT_IHLIB_BASE, JIT_IH_BASE_IPAIRS):
    //|// The type checks can be omitted -- see the iterator constructor.
    //|  mov eax, BASE[ra+TFOR_CTL].value		// Hidden control variable.
    //|  inc eax
    //|   mov TABLE:edx, BASE[ra+TFOR_TAB].value	// Table object.
    //|  mov BASE[ra+TFOR_CTL].value, eax
    //|  call &luaH_getnum, TABLE:edx, eax
    //|  // This is really copyslot BASE[ra+TFOR_VAL], TVALUE:eax[0] plus compare.
    //|  mov ecx, TVALUE:eax->tt
    //|  test ecx, ecx				// Assumes: LUA_TNIL == 0.
    //|  jz >9					// nil value stops iteration.
    //|   fild dword BASE[ra+TFOR_CTL].value	// Set numeric key.
    //|   settt BASE[ra+TFOR_KEY], LUA_TNUMBER
    //|   fstp qword BASE[ra+TFOR_KEY].value
    //|  mov edx, TVALUE:eax->value
    //|  mov eax, TVALUE:eax->value.na[1]	// Overwrites eax.
    //|  mov BASE[ra+TFOR_VAL].tt, ecx		// Copy value from table slot.
    //|  mov BASE[ra+TFOR_VAL].value, edx
    //|  mov BASE[ra+TFOR_VAL].value.na[1], eax
    //|  jmp =>target
    //|9:
    dasm_put(Dst, 1483, Dt2([ra+TFOR_CTL].value), Dt2([ra+TFOR_TAB].value), Dt2([ra+TFOR_CTL].value), (ptrdiff_t)(luaH_getnum), Dt7(->tt), Dt2([ra+TFOR_CTL].value), Dt2([ra+TFOR_KEY].tt), Dt2([ra+TFOR_KEY].value), Dt7(->value), Dt7(->value.na[1]), Dt2([ra+TFOR_VAL].tt), Dt2([ra+TFOR_VAL].value), Dt2([ra+TFOR_VAL].value.na[1]), target);
# 620 "ljit_x86_inline.dash"
    return 1;  /* Success, iterator has been inlined. */
  }

  return 0;  /* No support for inlining any other iterators. */
}

/* ------------------------------------------------------------------------ */

# 526 "ljit_x86.dasc"


#ifdef LUA_COMPAT_VARARG
static void jit_vararg_table(lua_State *L)
{
  Table *tab;
  StkId base, func;
  int i, num, numparams;
  luaC_checkGC(L);
  base = L->base;
  func = L->ci->func;
  numparams = clvalue(func)->l.p->numparams;
  num = base - func - numparams - 1;
  tab = luaH_new(L, num, 1);
  for (i = 0; i < num; i++)
    setobj2n(L, luaH_setnum(L, tab, i+1), base - num + i);
  setnvalue(luaH_setstr(L, tab, luaS_newliteral(L, "n")), (lua_Number)num);
  sethvalue(L, base + numparams, tab);
}
#endif

/* Encode JIT function prologue. */
static void jit_prologue(jit_State *J)
{
  Proto *pt = J->pt;
  int numparams = pt->numparams;
  int stacksize = pt->maxstacksize;

  //|// Note: the order of the following instructions has been carefully tuned.
  //|  lea eax, TOP[stacksize]
  //|   sub esp, FRAME_OFFSET
  //|  cmp eax, L->stack_last
  //|  jae ->GROW_STACK			// Stack overflow?
  //|  // This is a slight overallocation (BASE[1+stacksize] would be enough).
  //|  // We duplicate luaD_precall() behaviour so we can use luaD_growstack().
  //|  cmp CI, L->end_ci
  //|   lea CI, CI[1]
  //|  je ->GROW_CI			// CI overflow?
  //|  xor eax, eax			// Assumes: LUA_TNIL == 0
  //|   mov CI->func, BASE
  //|  add BASE, #BASE
  //|   mov L->ci, CI
  dasm_put(Dst, 1544, Dt3([stacksize]), Dt1(->stack_last), Dt1(->end_ci), Dt4([1]), Dt4(->func), sizeof(TValue), Dt1(->ci));
# 568 "ljit_x86.dasc"

  if (numparams > 0) {
    //|  lea edx, BASE[numparams]
    //|  cmp TOP, edx			// L->top >< L->base+numparams ?
    dasm_put(Dst, 1580, Dt2([numparams]));
# 572 "ljit_x86.dasc"
  }

  if (!pt->is_vararg) {  /* Fixarg function. */
    /* Must cap L->top at L->base+numparams because 1st LOADNIL is omitted. */
    if (numparams == 0) {
      //|  mov TOP, BASE
      dasm_put(Dst, 1586);
# 578 "ljit_x86.dasc"
    } else if (J->flags & JIT_F_CPU_CMOV) {
      //|  cmova TOP, edx
      dasm_put(Dst, 1589);
# 580 "ljit_x86.dasc"
    } else {
      //|  jna >1
      //|  mov TOP, edx
      //|1:
      dasm_put(Dst, 1594);
# 584 "ljit_x86.dasc"
    }
    //|   lea edx, BASE[stacksize]	// New ci->top.
    //|  mov CI->tailcalls, eax		// 0
    //|   mov CI->top, edx
    //|   mov L->top, edx
    //|  mov L->base, BASE
    //|  mov CI->base, BASE
    dasm_put(Dst, 1603, Dt2([stacksize]), Dt4(->tailcalls), Dt4(->top), Dt1(->top), Dt1(->base), Dt4(->base));
# 591 "ljit_x86.dasc"
  } else {  /* Vararg function. */
    int i;
    if (numparams > 0) {
      //|// If some fixargs are missing we need to clear them and
      //|// bump TOP to get a consistent frame layout for OP_VARARG.
      //|  jb >5
      //|4:
      //|.tail
      dasm_put(Dst, 1622);
# 599 "ljit_x86.dasc"
      //|5:  // This is uncommon. So move it to .tail and use a loop.
      //|  mov TOP->tt, eax
      //|  add TOP, #TOP
      //|  cmp TOP, edx
      //|  jb <5
      //|  jmp <4
      //|.code
      dasm_put(Dst, 1630, Dt3(->tt), sizeof(TValue));
# 606 "ljit_x86.dasc"
    }
    //|  mov L->base, TOP			// New base is after last arg.
    //|  mov CI->base, TOP
    //|   mov CI->tailcalls, eax		// 0
    dasm_put(Dst, 1649, Dt1(->base), Dt4(->base), Dt4(->tailcalls));
# 610 "ljit_x86.dasc"
    for (i = 0; i < numparams; i++) {  /* Move/clear fixargs. */
      //|// Inline this. Vararg funcs usually have very few fixargs.
      //|  copyslot TOP[i], BASE[i], ecx, edx
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 1659, Dt2([i].tt), Dt2([i].value), Dt3([i].tt), Dt3([i].value));
      } else {
      dasm_put(Dst, 1677, Dt2([i].value), Dt2([i].value.na[1]), Dt3([i].value), Dt2([i].tt), Dt3([i].value.na[1]), Dt3([i].tt));
      }
# 613 "ljit_x86.dasc"
      //|  mov BASE[i].tt, eax		// Clear old fixarg slot (help the GC).
      dasm_put(Dst, 854, Dt2([i].tt));
# 614 "ljit_x86.dasc"
    }
    if (numparams > 0) {
      //|  mov CI, L->ci			// Reload CI = ecx (used by move).
      dasm_put(Dst, 332, Dt1(->ci));
# 617 "ljit_x86.dasc"
    }
    //|  mov BASE, TOP
    //|   lea edx, BASE[stacksize]	// New ci->top.
    //|  lea TOP, BASE[numparams]		// Start of vars to clear.
    //|   mov CI->top, edx
    //|   mov L->top, edx
    dasm_put(Dst, 1696, Dt2([stacksize]), Dt2([numparams]), Dt4(->top), Dt1(->top));
# 623 "ljit_x86.dasc"
    stacksize -= numparams;		/* Fixargs are already cleared. */
  }

  /* Clear undefined args and all vars. Still assumes eax = LUA_TNIL = 0. */
  /* Note: cannot clear only args because L->top has grown. */
  if (stacksize <= EXTRA_STACK) {  /* Loopless clear. May use EXTRA_STACK. */
    int i;
    for (i = 0; i < stacksize; i++) {
      //|  mov TOP[i].tt, eax
      dasm_put(Dst, 1712, Dt3([i].tt));
# 632 "ljit_x86.dasc"
    }
  } else {  /* Standard loop. */
    //|2:  // Unrolled for 2 stack slots. No initial check. May use EXTRA_STACK.
    //|  mov TOP[0].tt, eax
    //|  mov TOP[1].tt, eax
    //|  add TOP, 2*#TOP
    //|  cmp TOP, edx
    //|  jb <2
    //|// Note: TOP is undefined now. TOP is only valid across calls/open ins.
    dasm_put(Dst, 1716, Dt3([0].tt), Dt3([1].tt), 2*sizeof(TValue));
# 641 "ljit_x86.dasc"
  }

#ifdef LUA_COMPAT_VARARG
  if (pt->is_vararg & VARARG_NEEDSARG) {
    //|  call &jit_vararg_table, L
    dasm_put(Dst, 1734, (ptrdiff_t)(jit_vararg_table));
# 646 "ljit_x86.dasc"
  }
#endif

  /* Call hook check. */
  if (J->flags & JIT_F_DEBUG_CALL) {
    //|  test byte L->hookmask, LUA_MASKCALL
    //|  jz >9
    //|  call ->HOOKCALL
    //|9:
    dasm_put(Dst, 1740, Dt1(->hookmask), LUA_MASKCALL);
# 655 "ljit_x86.dasc"

    //|.jsub HOOKCALL
# 672 "ljit_x86.dasc"
  }
}

/* Check if we can combine 'return const'. */
static int jit_return_k(jit_State *J)
{
  if (!J->combine) return 0;  /* COMBINE hint set? */
  /* May need to close open upvalues. */
  if (!fhint_isset(J, NOCLOSE)) {
    //|  call &luaF_close, L, BASE
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
# 682 "ljit_x86.dasc"
  }
  if (!J->pt->is_vararg) {  /* Fixarg function. */
    //|  sub aword L->ci, #CI
    //|  mov TOP, BASE
    //|  sub BASE, #BASE
    //|  add esp, FRAME_OFFSET
    dasm_put(Dst, 1830, Dt1(->ci), sizeof(CallInfo), sizeof(TValue));
# 688 "ljit_x86.dasc"
  } else {  /* Vararg function. */
    //|  mov CI, L->ci
    //|  mov BASE, CI->func
    //|  sub CI, #CI
    //|  mov L->ci, CI
    //|  lea TOP, BASE[1]
    //|  add esp, FRAME_OFFSET
    dasm_put(Dst, 1844, Dt1(->ci), Dt4(->func), sizeof(CallInfo), Dt1(->ci), Dt2([1]));
# 695 "ljit_x86.dasc"
  }
  jit_assert(J->combine == 1);  /* Required to skip next RETURN instruction. */
  return 1;
}

static void jit_op_return(jit_State *J, int rbase, int nresults)
{
  /* Return hook check. */
  if (J->flags & JIT_F_DEBUG_CALL) {
    if (nresults < 0 && !(J->flags & JIT_F_DEBUG_INS)) {
      //| mov L->top, TOP
      dasm_put(Dst, 594, Dt1(->top));
# 706 "ljit_x86.dasc"
    }
    //|// TODO: LUA_HOOKTAILRET (+ ci->tailcalls counting) or changed debug API.
    //|  test byte L->hookmask, LUA_MASKRET
    //|  jz >7
    //|  call ->HOOKRET
    //|7:
    dasm_put(Dst, 1863, Dt1(->hookmask), LUA_MASKRET);
# 712 "ljit_x86.dasc"
    if (J->flags & JIT_F_DEBUG_INS) {
      //|  mov eax, FRAME_RETADDR
      //|  mov L->savedpc, eax
      dasm_put(Dst, 1878, Dt1(->savedpc));
# 715 "ljit_x86.dasc"
    }

    //|.jsub HOOKRET
# 727 "ljit_x86.dasc"
  }

  /* May need to close open upvalues. */
  if (!fhint_isset(J, NOCLOSE)) {
    //|  call &luaF_close, L, BASE
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
# 732 "ljit_x86.dasc"
  }

  /* Previous op was open: 'return f()' or 'return ...' */
  if (nresults < 0) {
    //|// Relocate [BASE+rbase, TOP) -> [ci->func, *).
    //|  mov CI, L->ci
    //|  addidx BASE, rbase
    dasm_put(Dst, 332, Dt1(->ci));
    if (rbase) {
    dasm_put(Dst, 787, rbase*sizeof(TValue));
    }
# 739 "ljit_x86.dasc"
    //|  mov edx, CI->func
    //|  cmp BASE, TOP
    //|  jnb >2
    //|1:
    //|  mov eax, [BASE]
    //|  add BASE, aword*1
    //|  mov [edx], eax
    //|  add edx, aword*1
    //|  cmp BASE, TOP
    //|  jb <1
    //|2:
    //|  add esp, FRAME_OFFSET
    //|  mov BASE, CI->func
    //|  sub CI, #CI
    //|  mov TOP, edx			// Relocated TOP.
    //|  mov L->ci, CI
    //|  ret
    dasm_put(Dst, 1933, Dt4(->func), Dt4(->func), sizeof(CallInfo), Dt1(->ci));
# 756 "ljit_x86.dasc"
    return;
  }

  if (!J->pt->is_vararg) {  /* Fixarg function, nresults >= 0. */
    int i;
    //|  sub aword L->ci, #CI
    //|// Relocate [BASE+rbase,BASE+rbase+nresults) -> [BASE-1, *).
    //|// TODO: loop for large nresults?
    //|  sub BASE, #BASE
    dasm_put(Dst, 1980, Dt1(->ci), sizeof(CallInfo), sizeof(TValue));
# 765 "ljit_x86.dasc"
    for (i = 0; i < nresults; i++) {
      //|  copyslot BASE[i], BASE[rbase+i+1]
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([rbase+i+1].tt), Dt2([rbase+i+1].value), Dt2([i].tt), Dt2([i].value));
      } else {
      dasm_put(Dst, 839, Dt2([rbase+i+1].value), Dt2([rbase+i+1].value.na[1]), Dt2([rbase+i+1].tt), Dt2([i].value), Dt2([i].value.na[1]), Dt2([i].tt));
      }
# 767 "ljit_x86.dasc"
    }
    //|  add esp, FRAME_OFFSET
    //|  lea TOP, BASE[nresults]
    //|  ret
    dasm_put(Dst, 1989, Dt2([nresults]));
# 771 "ljit_x86.dasc"
  } else {  /* Vararg function, nresults >= 0. */
    int i;
    //|// Relocate [BASE+rbase,BASE+rbase+nresults) -> [ci->func, *).
    //|  mov CI, L->ci
    //|  mov TOP, CI->func
    //|  sub CI, #CI
    //|  mov L->ci, CI			// CI = ecx is used by copyslot.
    dasm_put(Dst, 1997, Dt1(->ci), Dt4(->func), sizeof(CallInfo), Dt1(->ci));
# 778 "ljit_x86.dasc"
    for (i = 0; i < nresults; i++) {
      //|  copyslot TOP[i], BASE[rbase+i]
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 1659, Dt2([rbase+i].tt), Dt2([rbase+i].value), Dt3([i].tt), Dt3([i].value));
      } else {
      dasm_put(Dst, 2010, Dt2([rbase+i].value), Dt2([rbase+i].value.na[1]), Dt2([rbase+i].tt), Dt3([i].value), Dt3([i].value.na[1]), Dt3([i].tt));
      }
# 780 "ljit_x86.dasc"
    }
    //|  add esp, FRAME_OFFSET
    //|  mov BASE, TOP
    //|  addidx TOP, nresults
    dasm_put(Dst, 2029);
    if (nresults) {
    dasm_put(Dst, 2036, nresults*sizeof(TValue));
    }
# 784 "ljit_x86.dasc"
    //|  ret
    dasm_put(Dst, 32);
# 785 "ljit_x86.dasc"
  }
}

static void jit_op_call(jit_State *J, int func, int nargs, int nresults)
{
  int cltype = jit_inline_call(J, func, nargs, nresults);
  if (cltype < 0) return;  /* Inlined? */

  //|// Note: the order of the following instructions has been carefully tuned.
  //|  addidx BASE, func
  if (func) {
  dasm_put(Dst, 787, func*sizeof(TValue));
  }
# 795 "ljit_x86.dasc"
  //|  mov CI, L->ci
  //|   isfunction 0			// BASE[0] is L->base[func].
  dasm_put(Dst, 2040, Dt1(->ci), Dt2([0].tt));
# 797 "ljit_x86.dasc"
  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    //|  lea TOP, BASE[1+nargs]
    dasm_put(Dst, 791, Dt2([1+nargs]));
# 799 "ljit_x86.dasc"
  }
  //|  mov LCL, BASE->value
  //|  mov edx, &J->nextins
  //|  mov CI->savedpc, edx
  dasm_put(Dst, 2048, Dt2(->value), (ptrdiff_t)(J->nextins), Dt4(->savedpc));
# 803 "ljit_x86.dasc"
  if (cltype == LUA_TFUNCTION) {
    if (nargs == -1) {
      //| jne ->DEOPTIMIZE_OPEN		// TYPE hint was wrong (open op)?
      dasm_put(Dst, 2057);
# 806 "ljit_x86.dasc"
    } else {
      //| jne ->DEOPTIMIZE		// TYPE hint was wrong?
      dasm_put(Dst, 2062);
# 808 "ljit_x86.dasc"
    }
  } else {
    //|   je >1				// Skip __call handling for functions.
    //|  call ->METACALL
    //|1:
    dasm_put(Dst, 2067);
# 813 "ljit_x86.dasc"

    //|.jsub METACALL			// CALL to __call metamethod.
# 826 "ljit_x86.dasc"
  }
  //|  call aword LCL->jit_gate		// Call JIT func or GATE_JL/GATE_JC.
  //|  subidx BASE, func
  dasm_put(Dst, 2116, Dt5(->jit_gate));
  if (func) {
  dasm_put(Dst, 1984, func*sizeof(TValue));
  }
# 829 "ljit_x86.dasc"
  //|  mov L->base, BASE
  dasm_put(Dst, 2121, Dt1(->base));
# 830 "ljit_x86.dasc"

  /* Clear undefined results TOP <= o < func+nresults. */
  if (nresults > 0) {
    //|  xor eax, eax
    dasm_put(Dst, 2125);
# 834 "ljit_x86.dasc"
    if (nresults <= EXTRA_STACK) {  /* Loopless clear. May use EXTRA_STACK. */
      int i;
      for (i = 0; i < nresults; i++) {
	//|  mov TOP[i].tt, eax
	dasm_put(Dst, 1712, Dt3([i].tt));
# 838 "ljit_x86.dasc"
      }
    } else {  /* Standard loop. TODO: move to .tail? */
      //|  lea edx, BASE[func+nresults]
      //|1:  // Unrolled for 2 stack slots. No initial check. May use EXTRA_STACK.
      //|  mov TOP[0].tt, eax			// LUA_TNIL
      //|  mov TOP[1].tt, eax			// LUA_TNIL
      //|  add TOP, 2*#TOP
      //|  cmp TOP, edx
      //|  jb <1
      dasm_put(Dst, 2128, Dt2([func+nresults]), Dt3([0].tt), Dt3([1].tt), 2*sizeof(TValue));
# 847 "ljit_x86.dasc"
    }
  }

  if (nresults >= 0) {  /* Not an open ins. Restore L->top. */
    //|  lea TOP, BASE[J->pt->maxstacksize]  // Faster than getting L->ci->top.
    //|  mov L->top, TOP
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
# 853 "ljit_x86.dasc"
  }  /* Otherwise keep TOP for next instruction. */
}

static void jit_op_tailcall(jit_State *J, int func, int nargs)
{
  int cltype;

  if (!fhint_isset(J, NOCLOSE)) {  /* May need to close open upvalues. */
    //|  call &luaF_close, L, BASE
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
# 862 "ljit_x86.dasc"
  }

  cltype = jit_inline_call(J, func, nargs, -2);
  if (cltype < 0) goto finish;  /* Inlined? */

  if (cltype == LUA_TFUNCTION) {
    jit_deopt_target(J, nargs);
    //|  isfunction func
    //|  jne L_DEOPTIMIZE			// TYPE hint was wrong?
    dasm_put(Dst, 2149, Dt2([func].tt));
# 871 "ljit_x86.dasc"
  } else {
    //|  isfunction func; jne >5		// Handle generic callables first.
    //|.tail
    dasm_put(Dst, 2158, Dt2([func].tt));
# 874 "ljit_x86.dasc"
    //|5:  // Fallback for generic callables.
    //|  addidx BASE, func
    dasm_put(Dst, 2168);
    if (func) {
    dasm_put(Dst, 787, func*sizeof(TValue));
    }
# 876 "ljit_x86.dasc"
    if (nargs >= 0) {
      //|  lea TOP, BASE[1+nargs]
      dasm_put(Dst, 791, Dt2([1+nargs]));
# 878 "ljit_x86.dasc"
    }
    //|  mov edx, &J->nextins
    //|  jmp ->METATAILCALL
    //|.code
    dasm_put(Dst, 2171, (ptrdiff_t)(J->nextins));
# 882 "ljit_x86.dasc"

    //|.jsub METATAILCALL			// TAILCALL to __call metamethod.
# 906 "ljit_x86.dasc"
  }

  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    int i;
    /* Relocate [BASE+func, BASE+func+nargs] -> [ci->func, ci->func+nargs]. */
    /* TODO: loop for large nargs? */
    if (!J->pt->is_vararg) {  /* Fixarg function. */
      //|  mov LCL, BASE[func].value
      dasm_put(Dst, 2241, Dt2([func].value));
# 914 "ljit_x86.dasc"
      for (i = 0; i < nargs; i++) {
	//|  copyslot BASE[i], BASE[func+1+i], ecx, edx
	if (J->flags & JIT_F_CPU_SSE2) {
	dasm_put(Dst, 821, Dt2([func+1+i].tt), Dt2([func+1+i].value), Dt2([i].tt), Dt2([i].value));
	} else {
	dasm_put(Dst, 2245, Dt2([func+1+i].value), Dt2([func+1+i].value.na[1]), Dt2([i].value), Dt2([func+1+i].tt), Dt2([i].value.na[1]), Dt2([i].tt));
	}
# 916 "ljit_x86.dasc"
      }
      //|  lea TOP, BASE[nargs]
      //|   sub BASE, #BASE
      //|  mov CI, L->ci
      //|  mov BASE->value, LCL		// Sufficient to copy func->value.
      dasm_put(Dst, 2264, Dt2([nargs]), sizeof(TValue), Dt1(->ci), Dt2(->value));
# 921 "ljit_x86.dasc"
    } else {  /* Vararg function. */
      //|   mov CI, L->ci
      //|  lea TOP, BASE[func]
      //|   mov BASE, CI->func
      //|  mov LCL, TOP->value
      //|  mov BASE->value, LCL		// Sufficient to copy func->value.
      dasm_put(Dst, 2278, Dt1(->ci), Dt2([func]), Dt4(->func), Dt3(->value), Dt2(->value));
# 927 "ljit_x86.dasc"
      for (i = 0; i < nargs; i++) {
	//|  copyslot BASE[i+1], TOP[i+1], eax, edx
	if (J->flags & JIT_F_CPU_SSE2) {
	dasm_put(Dst, 2294, Dt3([i+1].tt), Dt3([i+1].value), Dt2([i+1].tt), Dt2([i+1].value));
	} else {
	dasm_put(Dst, 2312, Dt3([i+1].value), Dt3([i+1].value.na[1]), Dt2([i+1].value), Dt3([i+1].tt), Dt2([i+1].value.na[1]), Dt2([i+1].tt));
	}
# 929 "ljit_x86.dasc"
      }
      //|  lea TOP, BASE[1+nargs]
      //|  mov LCL, BASE->value		// Need to reload LCL = eax.
      dasm_put(Dst, 2331, Dt2([1+nargs]), Dt2(->value));
# 932 "ljit_x86.dasc"
    }
  } else {  /* Previous op was open and set TOP. */
    //|// Relocate [BASE+func, TOP) -> [ci->func, *).
    //|  mov CI, L->ci
    //|  addidx BASE, func
    dasm_put(Dst, 332, Dt1(->ci));
    if (func) {
    dasm_put(Dst, 787, func*sizeof(TValue));
    }
# 937 "ljit_x86.dasc"
    //|  mov edx, CI->func
    //|1:
    //|  mov eax, [BASE]
    //|  add BASE, aword*1
    //|  mov [edx], eax
    //|  add edx, aword*1
    //|  cmp BASE, TOP
    //|  jb <1
    //|  mov BASE, CI->func
    //|  mov TOP, edx			// Relocated TOP.
    //|  mov LCL, BASE->value
    dasm_put(Dst, 2338, Dt4(->func), Dt4(->func), Dt2(->value));
# 948 "ljit_x86.dasc"
  }
  //|  sub CI, #CI
  //|  add esp, FRAME_OFFSET
  //|  jmp aword LCL->jit_gate		// Chain to JIT function.
  dasm_put(Dst, 2230, sizeof(CallInfo), Dt5(->jit_gate));
# 952 "ljit_x86.dasc"

finish:
  J->combine++;  /* Combine with following return instruction. */
}

/* ------------------------------------------------------------------------ */

static void jit_op_move(jit_State *J, int dest, int src)
{
  //|  copyslot BASE[dest], BASE[src]
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([src].tt), Dt2([src].value), Dt2([dest].tt), Dt2([dest].value));
  } else {
  dasm_put(Dst, 839, Dt2([src].value), Dt2([src].value.na[1]), Dt2([src].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
  }
# 962 "ljit_x86.dasc"
}

static void jit_op_loadk(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  //|  copyconst BASE[dest], kk
  switch (ttype(kk)) {
  case 0:
  dasm_put(Dst, 2369, Dt2([dest].tt));
    break;
  case 1:
  if (bvalue(kk)) {  /* true */
  dasm_put(Dst, 2377, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* false */
  dasm_put(Dst, 2389, Dt2([dest].value), Dt2([dest].tt));
  }
    break;
  case 3: {
  if ((&(kk)->value)->n == (lua_Number)0) {
  dasm_put(Dst, 2404);
  } else if ((&(kk)->value)->n == (lua_Number)1) {
  dasm_put(Dst, 2408);
  } else {
  dasm_put(Dst, 2411, &(kk)->value);
  }
  dasm_put(Dst, 1306, Dt2([dest].value), Dt2([dest].tt));
    break;
  }
  case 4:
  dasm_put(Dst, 2415, Dt2([dest].value), (ptrdiff_t)(gcvalue(kk)), Dt2([dest].tt));
    break;
  case 6:
  dasm_put(Dst, 2427, Dt2([dest].value), (ptrdiff_t)(gcvalue(kk)), Dt2([dest].tt));
    break;
  default: lua_assert(0); break;
  }
# 970 "ljit_x86.dasc"
  if (rk) {
    //|  ret
    dasm_put(Dst, 32);
# 972 "ljit_x86.dasc"
  }
}

static void jit_op_loadnil(jit_State *J, int first, int last)
{
  int idx, num = last - first + 1;
  int rk = jit_return_k(J);
  //|  xor eax, eax  // Assumes: LUA_TNIL == 0
  dasm_put(Dst, 2125);
# 980 "ljit_x86.dasc"
  if (rk) {
    //|  settt BASE[0], eax
    //|  ret
    dasm_put(Dst, 2439, Dt2([0].tt));
# 983 "ljit_x86.dasc"
  } else if (num <= 8) {
    for (idx = first; idx <= last; idx++) {
      //|  settt BASE[idx], eax  // 3/6 bytes
      dasm_put(Dst, 854, Dt2([idx].tt));
# 986 "ljit_x86.dasc"
    }
  } else {
    //|  lea ecx, BASE[first].tt  // 15-21 bytes
    //|  lea edx, BASE[last].tt
    //|1:
    //|  mov [ecx], eax
    //|  cmp ecx, edx
    //|  lea ecx, [ecx+#BASE]  // Preserves CC.
    //|  jbe <1
    dasm_put(Dst, 2444, Dt2([first].tt), Dt2([last].tt), sizeof(TValue));
# 995 "ljit_x86.dasc"
  }
}

static void jit_op_loadbool(jit_State *J, int dest, int b, int dojump)
{
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  //|  setbvalue BASE[dest], b
  if (b) {  /* true */
  dasm_put(Dst, 2377, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* false */
  dasm_put(Dst, 2389, Dt2([dest].value), Dt2([dest].tt));
  }
# 1003 "ljit_x86.dasc"
  if (rk) {
    //|  ret
    dasm_put(Dst, 32);
# 1005 "ljit_x86.dasc"
  } else if (dojump) {
    const TValue *h = hint_getpc(J, COMBINE, J->nextpc);
    if (!(ttisboolean(h) && bvalue(h) == 0)) {  /* Avoid jmp around dead ins. */
      //|  jmp =>J->nextpc+1
      dasm_put(Dst, 665, J->nextpc+1);
# 1009 "ljit_x86.dasc"
    }
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_getupval(jit_State *J, int dest, int uvidx)
{
  //|  getLCL
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 2241, Dt2([-1].value));
  } else {
  dasm_put(Dst, 2464, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
# 1018 "ljit_x86.dasc"
  //|  mov UPVAL:ecx, LCL->upvals[uvidx]
  //|  mov TOP, UPVAL:ecx->v
  //|  copyslot BASE[dest], TOP[0]
  dasm_put(Dst, 2474, Dt5(->upvals[uvidx]), DtF(->v));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 2481, Dt3([0].tt), Dt3([0].value), Dt2([dest].tt), Dt2([dest].value));
  } else {
  dasm_put(Dst, 2499, Dt3([0].value), Dt3([0].value.na[1]), Dt3([0].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
  }
# 1021 "ljit_x86.dasc"
}

static void jit_op_setupval(jit_State *J, int src, int uvidx)
{
  //|  getLCL
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 2241, Dt2([-1].value));
  } else {
  dasm_put(Dst, 2464, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
# 1026 "ljit_x86.dasc"
  //|  mov UPVAL:ecx, LCL->upvals[uvidx]
  //|  mov TOP, UPVAL:ecx->v
  //|  // This is really copyslot TOP[0], BASE[src] with compare mixed in.
  //|   mov eax, BASE[src].tt
  //|   mov GCOBJECT:edx, BASE[src].value
  //|   mov TOP->tt, eax
  //|  cmp eax, LUA_TSTRING				// iscollectable(val)?
  //|   mov eax, BASE[src].value.na[1]
  //|   mov TOP->value, GCOBJECT:edx
  //|   mov TOP->value.na[1], eax
  //|  jae >5
  //|4:
  //|.tail
  dasm_put(Dst, 2518, Dt5(->upvals[uvidx]), DtF(->v), Dt2([src].tt), Dt2([src].value), Dt3(->tt), Dt2([src].value.na[1]), Dt3(->value), Dt3(->value.na[1]));
# 1039 "ljit_x86.dasc"
  //|5:
  //|  test byte GCOBJECT:edx->gch.marked, WHITEBITS	// && iswhite(val)
  //|  jz <4
  //|  test byte UPVAL:ecx->marked, bitmask(BLACKBIT)	// && isblack(uv)
  //|  jz <4
  //|  call ->BARRIERF					// Yes, need barrier.
  //|  jmp <4
  //|.code
  dasm_put(Dst, 2554, DtA(->gch.marked), WHITEBITS, DtF(->marked), bitmask(BLACKBIT));
# 1047 "ljit_x86.dasc"

  //|.jsub BARRIERF			// luaC_barrierf() with regparms.
# 1054 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

/* Optimized table lookup routines. Enter via jsub, fallback to C. */

/* Fallback for GETTABLE_*. Temporary key is in L->env. */
static void jit_gettable_fb(lua_State *L, Table *t, StkId dest)
{
  Table *mt = t->metatable;
  const TValue *tm = luaH_getstr(mt, G(L)->tmname[TM_INDEX]);
  if (ttisnil(tm)) {  /* No __index method? */
    mt->flags |= 1<<TM_INDEX;  /* Cache this fact. */
    setnilvalue(dest);
  } else if (ttisfunction(tm)) {  /* __index function? */
    ptrdiff_t destr = savestack(L, dest);
    setobj2s(L, L->top, tm);
    sethvalue(L, L->top+1, t);
    setobj2s(L, L->top+2, &L->env);
    luaD_checkstack(L, 3);
    L->top += 3;
    luaD_call(L, L->top - 3, 1);
    dest = restorestack(L, destr);
    L->top--;
    setobjs2s(L, dest, L->top);
  } else {  /* Let luaV_gettable() continue with the __index object. */
    luaV_gettable(L, tm, &L->env, dest);
  }

  //|//-----------------------------------------------------------------------
  //|.jsub GETGLOBAL			// Lookup global variable.
# 1092 "ljit_x86.dasc"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub GETTABLE_KSTR			// Lookup constant string in table.
# 1161 "ljit_x86.dasc"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub GETTABLE_STR			// Lookup string in table.
# 1172 "ljit_x86.dasc"
}

/* Fallback for SETTABLE_*STR. Temporary (string) key is in L->env. */
static void jit_settable_fb(lua_State *L, Table *t, StkId val)
{
  Table *mt = t->metatable;
  const TValue *tm = luaH_getstr(mt, G(L)->tmname[TM_NEWINDEX]);
  if (ttisnil(tm)) {  /* No __newindex method? */
    mt->flags |= 1<<TM_NEWINDEX;  /* Cache this fact. */
    t->flags = 0;  /* But need to clear the cache for the table itself. */
    setobj2t(L, luaH_setstr(L, t, rawtsvalue(&L->env)), val);
    luaC_barriert(L, t, val);
  } else if (ttisfunction(tm)) {  /* __newindex function? */
    setobj2s(L, L->top, tm);
    sethvalue(L, L->top+1, t);
    setobj2s(L, L->top+2, &L->env);
    setobj2s(L, L->top+3, val);
    luaD_checkstack(L, 4);
    L->top += 4;
    luaD_call(L, L->top - 4, 0);
  } else {  /* Let luaV_settable() continue with the __newindex object. */
    luaV_settable(L, tm, &L->env, val);
  }

  //|//-----------------------------------------------------------------------
  //|.jsub BARRIERBACK			// luaC_barrierback() with regparms.
# 1206 "ljit_x86.dasc"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub SETGLOBAL			// Set global variable.
# 1216 "ljit_x86.dasc"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub SETTABLE_KSTR			// Set constant string entry in table.
# 1298 "ljit_x86.dasc"
  //|
  //|//-----------------------------------------------------------------------
  //|.jsub SETTABLE_STR			// Set string entry in table.
# 1309 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

static void jit_op_newtable(jit_State *J, int dest, int lnarray, int lnhash)
{
  //|  call &luaH_new, L, luaO_fb2int(lnarray), luaO_fb2int(lnhash)
  //|  sethvalue BASE[dest], eax
  dasm_put(Dst, 3170, luaO_fb2int(lnarray), luaO_fb2int(lnhash), (ptrdiff_t)(luaH_new), Dt2([dest].value), Dt2([dest].tt));
# 1317 "ljit_x86.dasc"
  jit_checkGC(J);
}

static void jit_op_getglobal(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  //|  mov TSTRING:edx, &&kk->value.gc->ts
  //|  addidx BASE, dest
  dasm_put(Dst, 3196, (ptrdiff_t)(&kk->value.gc->ts));
  if (dest) {
  dasm_put(Dst, 787, dest*sizeof(TValue));
  }
# 1326 "ljit_x86.dasc"
  //|  call ->GETGLOBAL
  dasm_put(Dst, 3199);
# 1327 "ljit_x86.dasc"
}

static void jit_op_setglobal(jit_State *J, int rval, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  //|  mov TSTRING:edx, &&kk->value.gc->ts
  //|  addidx BASE, rval
  dasm_put(Dst, 3196, (ptrdiff_t)(&kk->value.gc->ts));
  if (rval) {
  dasm_put(Dst, 787, rval*sizeof(TValue));
  }
# 1335 "ljit_x86.dasc"
  //|  call ->SETGLOBAL
  dasm_put(Dst, 3203);
# 1336 "ljit_x86.dasc"
}

enum { TKEY_KSTR = -2, TKEY_STR = -1, TKEY_ANY = 0 };

/* Optimize key lookup depending on consts or hints type. */
static int jit_keylookup(jit_State *J, int tab, int rkey)
{
  const TValue *tabt = hint_get(J, TYPE);
  const TValue *key;
  if (!ttistable(tabt)) return TKEY_ANY;  /* Not a table? Use fallback. */
  key = ISK(rkey) ? &J->pt->k[INDEXK(rkey)] : hint_get(J, TYPEKEY);
  if (ttisstring(key)) {  /* String key? */
    if (ISK(rkey)) {
      //|  lea TOP, BASE[tab]
      //|  mov TSTRING:edx, &&key->value.gc->ts
      dasm_put(Dst, 3207, Dt2([tab]), (ptrdiff_t)(&key->value.gc->ts));
# 1351 "ljit_x86.dasc"
      return TKEY_KSTR;  /* Const string key. */
    } else {
      //|  lea TOP, BASE[tab]
      //|  lea TVALUE:ecx, BASE[rkey]
      dasm_put(Dst, 3213, Dt2([tab]), Dt2([rkey]));
# 1355 "ljit_x86.dasc"
      return TKEY_STR;  /* Var string key. */
    }
  } else if (ttisnumber(key)) {  /* Number key? */
    lua_Number n = nvalue(key);
    int k;
    lua_number2int(k, n);
    if (!(k >= 1 && k < (1 << 26) && (lua_Number)k == n))
      return TKEY_ANY;  /* Not a proper array key? Use fallback. */
    if (ISK(rkey)) {
      //|  istable tab
      //|   mov TABLE:edi, BASE[tab].value
      //|  jne >9					// TYPE hint was wrong?
      //|  mov ecx, k				// Needed for hash fallback.
      //|   mov TVALUE:eax, TABLE:edi->array
      //|  cmp ecx, TABLE:edi->sizearray; ja >5	// Not in array part?
      dasm_put(Dst, 3220, Dt2([tab].tt), Dt2([tab].value), k, DtC(->array), DtC(->sizearray));
# 1370 "ljit_x86.dasc"
      return k;  /* Const array key (>= 1). */
    } else {
      //|  mov eax, BASE[tab].tt; shl eax, 4; or eax, BASE[rkey].tt
      //|  cmp eax, LUA_TTABLE_NUM; jne >9	// TYPE/TYPEKEY hint was wrong?
      dasm_put(Dst, 3244, Dt2([tab].tt), Dt2([rkey].tt));
# 1374 "ljit_x86.dasc"
      if (J->flags & JIT_F_CPU_SSE2) {
	//|  movsd xmm0, qword BASE[rkey]
	//|  cvttsd2si eax, xmm0
	//|  cvtsi2sd xmm1, eax
	//|  dec eax
	//|  ucomisd xmm1, xmm0
	//|  mov TABLE:edi, BASE[tab].value
	//|  jne >9; jp >9			// Not an integer? Deoptimize.
	dasm_put(Dst, 3262, Dt2([rkey]), Dt2([tab].value));
# 1382 "ljit_x86.dasc"
      } else {
	//|// Annoying x87 stuff: check whether a number is an integer.
	//|// The latency of fist/fild is the real problem here.
	//|  fld qword BASE[rkey].value
	//|  fist dword TMP1
	//|  fild dword TMP1
	//|  fcomparepp                           // eax may be modified.
	dasm_put(Dst, 3295, Dt2([rkey].value));
	if (J->flags & JIT_F_CPU_CMOV) {
	dasm_put(Dst, 3305);
	} else {
	dasm_put(Dst, 3310);
	}
# 1389 "ljit_x86.dasc"
	//|  jne >9; jp >9			// Not an integer? Deoptimize.
	//|  mov eax, TMP1
	//|  mov TABLE:edi, BASE[tab].value
	//|  dec eax
	dasm_put(Dst, 3316, Dt2([tab].value));
# 1393 "ljit_x86.dasc"
      }
      //|  cmp eax, TABLE:edi->sizearray; jae >5	// Not in array part?
      //|  TValuemul eax
      //|  add eax, TABLE:edi->array
      dasm_put(Dst, 3332, DtC(->sizearray), DtC(->array));
# 1397 "ljit_x86.dasc"
      return 1;  /* Variable array key. */
    }
  }
  return TKEY_ANY;  /* Use fallback. */
}

static void jit_op_gettable(jit_State *J, int dest, int tab, int rkey)
{
  TValue* const_keys = (TValue*)hint_get(J, TYPEKEYCONST);
  if( ttype(const_keys) == LUA_TTABLE ) {
    const TValue *key;
    key = ISK(rkey) ? &J->pt->k[INDEXK(rkey)] : hint_get(J, TYPEKEY);
    const TValue* value = luaH_get( hvalue(const_keys), key );
    //|  copyconst BASE[dest], value
    switch (ttype(value)) {
    case 0:
    dasm_put(Dst, 2369, Dt2([dest].tt));
      break;
    case 1:
    if (bvalue(value)) {  /* true */
    dasm_put(Dst, 2377, Dt2([dest].value), Dt2([dest].tt));
    } else {  /* false */
    dasm_put(Dst, 2389, Dt2([dest].value), Dt2([dest].tt));
    }
      break;
    case 3: {
    if ((&(value)->value)->n == (lua_Number)0) {
    dasm_put(Dst, 2404);
    } else if ((&(value)->value)->n == (lua_Number)1) {
    dasm_put(Dst, 2408);
    } else {
    dasm_put(Dst, 2411, &(value)->value);
    }
    dasm_put(Dst, 1306, Dt2([dest].value), Dt2([dest].tt));
      break;
    }
    case 4:
    dasm_put(Dst, 2415, Dt2([dest].value), (ptrdiff_t)(gcvalue(value)), Dt2([dest].tt));
      break;
    case 6:
    dasm_put(Dst, 2427, Dt2([dest].value), (ptrdiff_t)(gcvalue(value)), Dt2([dest].tt));
      break;
    default: lua_assert(0); break;
    }
# 1411 "ljit_x86.dasc"
    return;
  }
  int k = jit_keylookup(J, tab, rkey);
  switch (k) {
  case TKEY_KSTR:  /* Const string key. */
    //|  addidx BASE, dest
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
# 1417 "ljit_x86.dasc"
    //|  call ->GETTABLE_KSTR
    dasm_put(Dst, 3346);
# 1418 "ljit_x86.dasc"
    break;
  case TKEY_STR:  /* Variable string key. */
    //|  addidx BASE, dest
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
# 1421 "ljit_x86.dasc"
    //|  call ->GETTABLE_STR
    dasm_put(Dst, 3350);
# 1422 "ljit_x86.dasc"
    break;
  case TKEY_ANY:  /* Generic gettable fallback. */
    if (ISK(rkey)) {
      //|  mov ecx, &&J->pt->k[INDEXK(rkey)]
      dasm_put(Dst, 3354, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
# 1426 "ljit_x86.dasc"
    } else {
      //|  lea ecx, BASE[rkey]
      dasm_put(Dst, 3216, Dt2([rkey]));
# 1428 "ljit_x86.dasc"
    }
    //|  lea edx, BASE[tab]
    //|  addidx BASE, dest
    dasm_put(Dst, 3357, Dt2([tab]));
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
# 1431 "ljit_x86.dasc"
    //|  mov L->savedpc, &J->nextins
    //|  call &luaV_gettable, L, edx, ecx, BASE
    //|  mov BASE, L->base
    dasm_put(Dst, 3361, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_gettable), Dt1(->base));
# 1434 "ljit_x86.dasc"
    break;
  default:  /* Array key. */
    //|// This is really copyslot BASE[dest], TVALUE:eax[k-1] mixed with compare.
    //|1:
    //|  mov edx, TVALUE:eax[k-1].tt
    //|  test edx, edx; je >6			// Array has nil value?
    dasm_put(Dst, 3378, Dt7([k-1].tt));
# 1440 "ljit_x86.dasc"
    if (J->flags & JIT_F_CPU_SSE2) {
      //|  movq xmm0, qword TVALUE:eax[k-1].value
      //|  movq qword BASE[dest].value, xmm0
      dasm_put(Dst, 2686, Dt7([k-1].value), Dt2([dest].value));
# 1443 "ljit_x86.dasc"
    } else {
      //|  mov ecx, TVALUE:eax[k-1].value
      //|  mov eax, TVALUE:eax[k-1].value.na[1]
      //|  mov BASE[dest].value, ecx
      //|  mov BASE[dest].value.na[1], eax
      dasm_put(Dst, 3390, Dt7([k-1].value), Dt7([k-1].value.na[1]), Dt2([dest].value), Dt2([dest].value.na[1]));
# 1448 "ljit_x86.dasc"
    }
    //|2:
    //|  mov BASE[dest].tt, edx
    //|.tail
    dasm_put(Dst, 3403, Dt2([dest].tt));
# 1452 "ljit_x86.dasc"
    //|5:  // Fallback to hash part. TABLE:edi is callee-saved.
    dasm_put(Dst, 2168);
# 1453 "ljit_x86.dasc"
    if (ISK(rkey)) {
      //|  call ->GETTABLE_KNUM
      dasm_put(Dst, 3410);
# 1455 "ljit_x86.dasc"
    } else {
      //|  call ->GETTABLE_NUM
      dasm_put(Dst, 3414);
# 1457 "ljit_x86.dasc"
    }
    //|  jmp <1					// Slot is at TVALUE:eax[k-1].
    //|
    //|6:  // Shortcut for tables without an __index metamethod.
    //|  mov TABLE:ecx, TABLE:edi->metatable
    //|  test TABLE:ecx, TABLE:ecx
    //|  jz <2					// No metatable?
    //|  test byte TABLE:ecx->flags, 1<<TM_INDEX
    //|  jnz <2					// Or 'no __index' flag set?
    //|
    //|9:  // Otherwise deoptimize.
    //|  mov edx, &J->nextins
    //|  jmp ->DEOPTIMIZE
    //|.code
    dasm_put(Dst, 3418, DtC(->metatable), DtC(->flags), 1<<TM_INDEX, (ptrdiff_t)(J->nextins));
# 1471 "ljit_x86.dasc"
    break;
  }

  //|.jsub GETTABLE_KNUM		// Gettable fallback for const numeric keys.
# 1485 "ljit_x86.dasc"
  //|
  //|.jsub GETTABLE_NUM		// Gettable fallback for variable numeric keys.
# 1492 "ljit_x86.dasc"
}

static void jit_op_settable(jit_State *J, int tab, int rkey, int rval)
{
  const TValue *val = ISK(rval) ? &J->pt->k[INDEXK(rval)] : NULL;
  int k = jit_keylookup(J, tab, rkey);
  switch (k) {
  case TKEY_KSTR:  /* Const string key. */
  case TKEY_STR:  /* Variable string key. */
    if (ISK(rval)) {
      //|  mov BASE, &val
      dasm_put(Dst, 3504, (ptrdiff_t)(val));
# 1503 "ljit_x86.dasc"
    } else {
      //|  addidx BASE, rval
      if (rval) {
      dasm_put(Dst, 787, rval*sizeof(TValue));
      }
# 1505 "ljit_x86.dasc"
    }
    if (k == TKEY_KSTR) {
      //|  call ->SETTABLE_KSTR
      dasm_put(Dst, 3507);
# 1508 "ljit_x86.dasc"
    } else {
      //|  call ->SETTABLE_STR
      dasm_put(Dst, 3511);
# 1510 "ljit_x86.dasc"
    }
    break;
  case TKEY_ANY:  /* Generic settable fallback. */
    if (ISK(rkey)) {
      //|  mov ecx, &&J->pt->k[INDEXK(rkey)]
      dasm_put(Dst, 3354, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
# 1515 "ljit_x86.dasc"
    } else {
      //|  lea ecx, BASE[rkey]
      dasm_put(Dst, 3216, Dt2([rkey]));
# 1517 "ljit_x86.dasc"
    }
    if (ISK(rval)) {
      //|  mov edx, &val
      dasm_put(Dst, 3196, (ptrdiff_t)(val));
# 1520 "ljit_x86.dasc"
    } else {
      //|  lea edx, BASE[rval]
      dasm_put(Dst, 3357, Dt2([rval]));
# 1522 "ljit_x86.dasc"
    }
    //|  addidx BASE, tab
    if (tab) {
    dasm_put(Dst, 787, tab*sizeof(TValue));
    }
# 1524 "ljit_x86.dasc"
    //|  mov L->savedpc, &J->nextins
    //|  call &luaV_settable, L, BASE, ecx, edx
    //|  mov BASE, L->base
    dasm_put(Dst, 3515, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_settable), Dt1(->base));
# 1527 "ljit_x86.dasc"
    break;
  default:  /* Array key. */
    //|1:
    //|  tvisnil TVALUE:eax[k-1]; je >6		// Previous value is nil?
    //|2:
    //|.tail
    dasm_put(Dst, 3532, Dt7([k-1].tt));
# 1533 "ljit_x86.dasc"
    //|5:  // Fallback to hash part. TABLE:edi is callee-saved.
    dasm_put(Dst, 2168);
# 1534 "ljit_x86.dasc"
    if (ISK(rkey)) {
      //|  call ->SETTABLE_KNUM
      dasm_put(Dst, 3546);
# 1536 "ljit_x86.dasc"
    } else {
      //|  call ->SETTABLE_NUM
      dasm_put(Dst, 3550);
# 1538 "ljit_x86.dasc"
    }
    //|  jmp <1					// Slot is at TVALUE:eax[k-1].
    //|
    //|6:  // Shortcut for tables without a __newindex metamethod.
    //|  mov TABLE:ecx, TABLE:edi->metatable
    //|  test TABLE:ecx, TABLE:ecx
    //|  jz <2					// No metatable?
    //|  test byte TABLE:ecx->flags, 1<<TM_NEWINDEX
    //|  jnz <2					// Or 'no __newindex' flag set?
    //|
    //|9:  // Otherwise deoptimize.
    //|  mov edx, &J->nextins
    //|  jmp ->DEOPTIMIZE
    //|.code
    dasm_put(Dst, 3418, DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, (ptrdiff_t)(J->nextins));
# 1552 "ljit_x86.dasc"
    if (!ISK(rval) || iscollectable(val)) {
      //|  test byte TABLE:edi->marked, bitmask(BLACKBIT)  // isblack(table)
      //|  jnz >7				// Unlikely, but set barrier back.
      //|3:
      //|.tail
      dasm_put(Dst, 3554, DtC(->marked), bitmask(BLACKBIT));
# 1557 "ljit_x86.dasc"
      //|7:  // Avoid valiswhite() check -- black2gray(table) is ok.
      //|  call ->BARRIERBACK
      //|  jmp <3
      //|.code
      dasm_put(Dst, 3567);
# 1561 "ljit_x86.dasc"
    }
    if (ISK(rval)) {
      //|  copyconst TVALUE:eax[k-1], val
      switch (ttype(val)) {
      case 0:
      dasm_put(Dst, 3577, Dt7([k-1].tt));
        break;
      case 1:
      if (bvalue(val)) {  /* true */
      dasm_put(Dst, 3585, Dt7([k-1].value), Dt7([k-1].tt));
      } else {  /* false */
      dasm_put(Dst, 3597, Dt7([k-1].value), Dt7([k-1].tt));
      }
        break;
      case 3: {
      if ((&(val)->value)->n == (lua_Number)0) {
      dasm_put(Dst, 2404);
      } else if ((&(val)->value)->n == (lua_Number)1) {
      dasm_put(Dst, 2408);
      } else {
      dasm_put(Dst, 2411, &(val)->value);
      }
      dasm_put(Dst, 3612, Dt7([k-1].value), Dt7([k-1].tt));
        break;
      }
      case 4:
      dasm_put(Dst, 3623, Dt7([k-1].value), (ptrdiff_t)(gcvalue(val)), Dt7([k-1].tt));
        break;
      case 6:
      dasm_put(Dst, 3635, Dt7([k-1].value), (ptrdiff_t)(gcvalue(val)), Dt7([k-1].tt));
        break;
      default: lua_assert(0); break;
      }
# 1564 "ljit_x86.dasc"
    } else {
      //|  copyslot TVALUE:eax[k-1], BASE[rval], ecx, edx, TOP
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 2959, Dt2([rval].tt), Dt2([rval].value), Dt7([k-1].tt), Dt7([k-1].value));
      } else {
      dasm_put(Dst, 2977, Dt2([rval].value), Dt2([rval].value.na[1]), Dt2([rval].tt), Dt7([k-1].value), Dt7([k-1].value.na[1]), Dt7([k-1].tt));
      }
# 1566 "ljit_x86.dasc"
    }
    break;
  }

  //|.jsub SETTABLE_KNUM		// Settable fallback for const numeric keys.
# 1581 "ljit_x86.dasc"
  //|
  //|.jsub SETTABLE_NUM		// Settable fallback for variable numeric keys.
# 1589 "ljit_x86.dasc"
}

static void jit_op_self(jit_State *J, int dest, int tab, int rkey)
{
  //|  copyslot BASE[dest+1], BASE[tab]
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([tab].tt), Dt2([tab].value), Dt2([dest+1].tt), Dt2([dest+1].value));
  } else {
  dasm_put(Dst, 839, Dt2([tab].value), Dt2([tab].value.na[1]), Dt2([tab].tt), Dt2([dest+1].value), Dt2([dest+1].value.na[1]), Dt2([dest+1].tt));
  }
# 1594 "ljit_x86.dasc"
  jit_op_gettable(J, dest, tab, rkey);
}

/* ------------------------------------------------------------------------ */

static void jit_op_setlist(jit_State *J, int ra, int num, int batch)
{
  if (batch == 0) { batch = (int)(*J->nextins); J->combine++; }
  batch = (batch-1)*LFIELDS_PER_FLUSH;
  if (num == 0) {  /* Previous op was open and set TOP: {f()} or {...}. */
    //|  mov L->env.value, TOP		// Need to save TOP (edi).
    //|  lea eax, BASE[ra+1]
    //|  sub eax, TOP
    //|  neg eax
    //|  TValuediv eax			// num = (TOP-ra-1)/sizeof(TValue).
    //|  mov TABLE:edi, BASE[ra].value
    //|  jz >4				// Nothing to set?
    dasm_put(Dst, 3709, Dt1(->env.value), Dt2([ra+1]), Dt2([ra].value));
# 1611 "ljit_x86.dasc"
    if (batch > 0) {
      //|  add eax, batch
      dasm_put(Dst, 3733, batch);
# 1613 "ljit_x86.dasc"
    }
    //|  cmp dword TABLE:edi->sizearray, eax
    //|  jae >1				// Skip resize if not needed.
    //|  // A resize is likely, so inline it.
    //|  call &luaH_resizearray, L, TABLE:edi, eax
    //|1:
    //|  test byte TABLE:edi->marked, bitmask(BLACKBIT)  // isblack(table)
    //|  mov edx, TABLE:edi->array
    //|  jnz >6				// Unlikely, but set barrier back.
    //|  mov TOP, L->env.value
    //|
    //|.tail
    dasm_put(Dst, 3737, DtC(->sizearray), (ptrdiff_t)(luaH_resizearray), DtC(->marked), bitmask(BLACKBIT), DtC(->array), Dt1(->env.value));
# 1625 "ljit_x86.dasc"
    //|6:  // Avoid lots of valiswhite() checks -- black2gray(table) is ok.
    //|  call ->BARRIERBACK
    //|  jmp <1  // Need to reload edx.
    //|.code
    dasm_put(Dst, 3776);
# 1629 "ljit_x86.dasc"
  } else {  /* Set fixed number of args. */
    //|  mov TABLE:edi, BASE[ra].value	// edi is callee-save.
    //|  cmp dword TABLE:edi->sizearray, batch+num
    //|  jb >5				// Need to resize array?
    //|1:
    //|  test byte TABLE:edi->marked, bitmask(BLACKBIT)  // isblack(table)
    //|  mov edx, TABLE:edi->array
    //|  jnz >6				// Unlikely, but set barrier back.
    //|  lea TOP, BASE[ra+1+num]		// Careful: TOP is edi.
    //|
    //|.tail
    dasm_put(Dst, 3786, Dt2([ra].value), DtC(->sizearray), batch+num, DtC(->marked), bitmask(BLACKBIT), DtC(->array), Dt2([ra+1+num]));
# 1640 "ljit_x86.dasc"
    //|5:  // A resize is unlikely (impossible?). NEWTABLE should've done it.
    //|  call &luaH_resizearray, L, TABLE:edi, batch+num
    //|  jmp <1
    //|6:  // Avoid lots of valiswhite() checks -- black2gray(table) is ok.
    //|  call ->BARRIERBACK
    //|  jmp <1  // Need to reload edx.
    //|.code
    dasm_put(Dst, 3816, batch+num, (ptrdiff_t)(luaH_resizearray));
# 1647 "ljit_x86.dasc"
  }
  if (batch > 0) {
    //|  add edx, batch*#TVALUE		// edx = &t->array[(batch+1)-1]
    dasm_put(Dst, 3845, batch*sizeof(TValue));
# 1650 "ljit_x86.dasc"
  }
  //|  lea ecx, BASE[ra+1]
  //|3:					// Copy stack slots to array.
  //|  mov eax, [ecx]
  //|  add ecx, aword*1
  //|  mov [edx], eax
  //|  add edx, aword*1
  //|  cmp ecx, TOP
  //|  jb <3
  //|
  //|4:
  dasm_put(Dst, 3849, Dt2([ra+1]));
# 1661 "ljit_x86.dasc"
  if (num == 0) {  /* Previous op was open. Restore L->top. */
    //|  lea TOP, BASE[J->pt->maxstacksize]  // Faster than getting L->ci->top.
    //|  mov L->top, TOP
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
# 1664 "ljit_x86.dasc"
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_arith(jit_State *J, int dest, int rkb, int rkc, int ev)
{
  const TValue *kkb = ISK(rkb) ? &J->pt->k[INDEXK(rkb)] : NULL;
  const TValue *kkc = ISK(rkc) ? &J->pt->k[INDEXK(rkc)] : NULL;
  const Value *kval;
  int idx, rev;
  int target = (ev == TM_LT || ev == TM_LE) ? jit_jmp_target(J) : 0;
  int hastail = 0;

  /* The bytecode compiler already folds constants except for: k/0, k%0, */
  /* NaN results, k1<k2, k1<=k2. No point in optimizing these cases. */
  if (ISK(rkb&rkc)) goto fallback;

  /* Avoid optimization when non-numeric constants are present. */
  if (kkb ? !ttisnumber(kkb) : (kkc && !ttisnumber(kkc))) goto fallback;

  /* The TYPE hint selects numeric inlining and/or fallback encoding. */
  switch (ttype(hint_get(J, TYPE))) {
  case LUA_TNIL: hastail = 1; break;  /* No hint: numeric + fallback. */
  case LUA_TNUMBER: break;	      /* Numbers: numeric + deoptimization. */
  default: goto fallback;	      /* Mixed/other types: fallback only. */
  }

  /* The checks above ensure: at most one of the operands is a constant. */
  /* Reverse operation and swap operands so the 2nd operand is a variable. */
  if (kkc) { kval = &kkc->value; idx = rkb; rev = 1; }
  else { kval = kkb ? &kkb->value : NULL; idx = rkc; rev = 0; }

  /* Special handling for some operators. */
  switch (ev) {
  case TM_BAND:
  case TM_BOR: 
  case TM_BXOR: 
  case TM_BSHL:
  case TM_BSHR:
  case TM_BNOT: 
      hastail = 0;
      goto fallback;
  case TM_MOD:
    /* Check for modulo with positive numbers, so we can use fprem. */
    if (kval) {
      if (kval->na[1] < 0) { hastail = 0; goto fallback; }  /* x%-k, -k%x */
      //|  isnumber idx
      //|   mov eax, BASE[idx].value.na[1]
      //|  jne L_DEOPTIMIZEF
      //|   test eax, eax; js L_DEOPTIMIZEF
      //|// This will trigger deoptimization in some benchmarks (pidigits).
      //|// But it's still a win.
      dasm_put(Dst, 3874, Dt2([idx].tt), Dt2([idx].value.na[1]));
# 1717 "ljit_x86.dasc"
      if (kkb) {
	//|  fld qword BASE[rkc].value
	//|  fld qword [kval]
	dasm_put(Dst, 3892, Dt2([rkc].value), kval);
# 1720 "ljit_x86.dasc"
      } else {
	//|  fld qword [kval]
	//|  fld qword BASE[rkb].value
	dasm_put(Dst, 3899, kval, Dt2([rkb].value));
# 1723 "ljit_x86.dasc"
      }
    } else {
      //|  isnumber2 rkb, rkc
      //|   mov eax, BASE[rkb].value.na[1]
      //|  jne L_DEOPTIMIZEF
      //|   or eax, BASE[rkc].value.na[1]; js L_DEOPTIMIZEF
      //|  fld qword BASE[rkc].value
      //|  fld qword BASE[rkb].value
      dasm_put(Dst, 3906, Dt2([rkb].tt), Dt2([rkc].tt), Dt2([rkb].value.na[1]), Dt2([rkc].value.na[1]), Dt2([rkc].value), Dt2([rkb].value));
# 1731 "ljit_x86.dasc"
    }
    //|1: ; fprem; fnstsw ax; sahf; jp <1
    //|  fstp st1
    dasm_put(Dst, 1387);
# 1734 "ljit_x86.dasc"
    goto fpstore;
  case TM_POW:
    if (hastail || !kval) break;  /* Avoid this if not optimizing. */
    if (rev) {  /* x^k for k > 0, k integer. */
      lua_Number n = kval->n;
      int k;
      lua_number2int(k, n);
      /* All positive integers would work. But need to limit code explosion. */
      if (k > 0 && k <= 65536 && (lua_Number)k == n) {
	//|  isnumber idx; jne L_DEOPTIMIZEF
	//|  fld qword BASE[idx]
	dasm_put(Dst, 3940, Dt2([idx].tt), Dt2([idx]));
# 1745 "ljit_x86.dasc"
	for (; (k & 1) == 0; k >>= 1) {  /* Handle leading zeroes (2^k). */
	  //|  fmul st0
	  dasm_put(Dst, 3952);
# 1747 "ljit_x86.dasc"
	}
	if ((k >>= 1) != 0) {  /* Handle trailing bits. */
	  //|  fld st0
	  //|  fmul st0
	  dasm_put(Dst, 3955);
# 1751 "ljit_x86.dasc"
	  for (; k != 1; k >>= 1) {
	    if (k & 1) {
	      //|  fmul st1, st0
	      dasm_put(Dst, 3960);
# 1754 "ljit_x86.dasc"
	    }
	    //|  fmul st0
	    dasm_put(Dst, 3952);
# 1756 "ljit_x86.dasc"
	  }
	  //|  fmulp st1
	  dasm_put(Dst, 3963);
# 1758 "ljit_x86.dasc"
	}
	goto fpstore;
      }
    } else if (kval->n > (lua_Number)0) {  /* k^x for k > 0. */
      int log2kval[3];  /* Enough storage for a tword (80 bits). */
      log2kval[2] = 0;  /* Avoid leaking garbage. */
      /* Double precision log2(k) doesn't cut it (3^x != 3 for x = 1). */
      ((void (*)(int *, double))J->jsub[JSUB_LOG2_TWORD])(log2kval, kval->n);
      //|  mov ARG1, log2kval[0]			// Abuse stack for tword const.
      //|  mov ARG2, log2kval[1]
      //|  mov ARG3, log2kval[2]			// TODO: store2load fwd stall.
      //|  isnumber idx; jne L_DEOPTIMIZEF
      //|  fld tword [esp]
      //|  fmul qword BASE[idx].value		// log2(k)*x
      //|  fld st0; frndint; fsub st1, st0; fxch	// Split into fract/int part.
      //|  f2xm1; fld1; faddp st1; fscale		// (2^fract-1 +1) << int.
      //|  fstp st1
      dasm_put(Dst, 3966, log2kval[0], log2kval[1], log2kval[2], Dt2([idx].tt), Dt2([idx].value));
# 1775 "ljit_x86.dasc"

      //|.jsub LOG2_TWORD		// Calculate log2(k) with max. precision.
# 1784 "ljit_x86.dasc"
      goto fpstore;
    }
    break;
  }

  /* Check number type and load 1st operand. */
  if (kval) {
    //|  isnumber idx; jne L_DEOPTIMIZEF
    //|  loadnvaluek kval
    dasm_put(Dst, 4037, Dt2([idx].tt));
    if ((kval)->n == (lua_Number)0) {
    dasm_put(Dst, 2404);
    } else if ((kval)->n == (lua_Number)1) {
    dasm_put(Dst, 2408);
    } else {
    dasm_put(Dst, 2411, kval);
    }
# 1793 "ljit_x86.dasc"
  } else {
    if (rkb == rkc) {
      //|  isnumber rkb
      dasm_put(Dst, 4046, Dt2([rkb].tt));
# 1796 "ljit_x86.dasc"
    } else {
      //|  isnumber2 rkb, rkc
      dasm_put(Dst, 4051, Dt2([rkb].tt), Dt2([rkc].tt));
# 1798 "ljit_x86.dasc"
    }
    //|  jne L_DEOPTIMIZEF
    //|  fld qword BASE[rkb].value
    dasm_put(Dst, 3944, Dt2([rkb].value));
# 1801 "ljit_x86.dasc"
  }

  /* Encode arithmetic operation with 2nd operand. */
  switch ((ev<<1)+rev) {
  case TM_ADD<<1: case (TM_ADD<<1)+1:
    if (rkb == rkc) {
      //|  fadd st0
      dasm_put(Dst, 4065);
# 1808 "ljit_x86.dasc"
    } else {
      //|  fadd qword BASE[idx].value
      dasm_put(Dst, 4068, Dt2([idx].value));
# 1810 "ljit_x86.dasc"
    }
    break;
  case TM_SUB<<1:
    //|  fsub qword BASE[idx].value
    dasm_put(Dst, 4072, Dt2([idx].value));
# 1814 "ljit_x86.dasc"
    break;
  case (TM_SUB<<1)+1:
    //|  fsubr qword BASE[idx].value
    dasm_put(Dst, 4076, Dt2([idx].value));
# 1817 "ljit_x86.dasc"
    break;
  case TM_MUL<<1: case (TM_MUL<<1)+1:
    if (rkb == rkc) {
      //|  fmul st0
      dasm_put(Dst, 3952);
# 1821 "ljit_x86.dasc"
    } else {
      //|  fmul qword BASE[idx].value
      dasm_put(Dst, 4080, Dt2([idx].value));
# 1823 "ljit_x86.dasc"
    }
    break;
  case TM_DIV<<1:
    //|  fdiv qword BASE[idx].value
    dasm_put(Dst, 4084, Dt2([idx].value));
# 1827 "ljit_x86.dasc"
    break;
  case (TM_DIV<<1)+1:
    //|  fdivr qword BASE[idx].value
    dasm_put(Dst, 4088, Dt2([idx].value));
# 1830 "ljit_x86.dasc"
    break;
  case TM_POW<<1:
    //|  sub esp, S2LFRAME_OFFSET
    //|  fstp FPARG1
    //|  fld qword BASE[idx].value
    //|  fstp FPARG2
    //|  call &pow
    //|  add esp, S2LFRAME_OFFSET
    dasm_put(Dst, 4092, Dt2([idx].value), (ptrdiff_t)(pow));
# 1838 "ljit_x86.dasc"
    break;
  case (TM_POW<<1)+1:
    //|  sub esp, S2LFRAME_OFFSET
    //|  fstp FPARG2
    //|  fld qword BASE[idx].value
    //|  fstp FPARG1
    //|  call &pow
    //|  add esp, S2LFRAME_OFFSET
    dasm_put(Dst, 4112, Dt2([idx].value), (ptrdiff_t)(pow));
# 1846 "ljit_x86.dasc"
    break;
  case TM_UNM<<1: case (TM_UNM<<1)+1:
    //|  fchs				// No 2nd operand.
    dasm_put(Dst, 4132);
# 1849 "ljit_x86.dasc"
    break;
  default:  /* TM_LT or TM_LE. */
    //|  fld qword BASE[idx].value
    //|  fcomparepp
    dasm_put(Dst, 1325, Dt2([idx].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3305);
    } else {
    dasm_put(Dst, 3310);
    }
# 1853 "ljit_x86.dasc"
    //|  jp =>dest?(J->nextpc+1):target	// Unordered means false.
    dasm_put(Dst, 4135, dest?(J->nextpc+1):target);
# 1854 "ljit_x86.dasc"
    jit_assert(dest == 0 || dest == 1);  /* Really cond. */
    switch (((rev^dest)<<1)+(dest^(ev == TM_LT))) {
    case 0:
      //|  jb =>target
      dasm_put(Dst, 4139, target);
# 1858 "ljit_x86.dasc"
      break;
    case 1:
      //|  jbe =>target
      dasm_put(Dst, 4143, target);
# 1861 "ljit_x86.dasc"
      break;
    case 2:
      //|  ja =>target
      dasm_put(Dst, 4147, target);
# 1864 "ljit_x86.dasc"
      break;
    case 3:
      //|  jae =>target
      dasm_put(Dst, 4151, target);
# 1867 "ljit_x86.dasc"
      break;
    }
    goto skipstore;
  }
fpstore:
  /* Store result and set result type (if necessary). */
  //|  fstp qword BASE[dest].value
  dasm_put(Dst, 933, Dt2([dest].value));
# 1874 "ljit_x86.dasc"
  if (dest != rkb && dest != rkc) {
    //|  settt BASE[dest], LUA_TNUMBER
    dasm_put(Dst, 1309, Dt2([dest].tt));
# 1876 "ljit_x86.dasc"
  }

skipstore:
  if (!hastail) {
    jit_deopt_target(J, 0);
    return;
  }

  //|4:
  //|.tail
  dasm_put(Dst, 1626);
# 1886 "ljit_x86.dasc"
  //|L_DEOPTLABEL:  // Recycle as fallback label.
  dasm_put(Dst, 1541);
# 1887 "ljit_x86.dasc"

fallback:
  /* Generic fallback for arithmetic ops. */
  if (kkb) {
    //|  mov ecx, &kkb
    dasm_put(Dst, 3354, (ptrdiff_t)(kkb));
# 1892 "ljit_x86.dasc"
  } else {
    //|  lea ecx, BASE[rkb]
    dasm_put(Dst, 3216, Dt2([rkb]));
# 1894 "ljit_x86.dasc"
  }
  if (kkc) {
    //|  mov edx, &kkc
    dasm_put(Dst, 3196, (ptrdiff_t)(kkc));
# 1897 "ljit_x86.dasc"
  } else {
    //|  lea edx, BASE[rkc]
    dasm_put(Dst, 3357, Dt2([rkc]));
# 1899 "ljit_x86.dasc"
  }
  if (target) {  /* TM_LT or TM_LE. */
    //|  mov L->savedpc, &(J->nextins+1)
    //|  call &ev==TM_LT?luaV_lessthan:luaV_lessequal, L, ecx, edx
    //|  test eax, eax
    //|  mov BASE, L->base
    dasm_put(Dst, 4155, Dt1(->savedpc), (ptrdiff_t)((J->nextins+1)), (ptrdiff_t)(ev==TM_LT?luaV_lessthan:luaV_lessequal), Dt1(->base));
# 1905 "ljit_x86.dasc"
    if (dest) {  /* cond */
      //|  jnz =>target
      dasm_put(Dst, 1479, target);
# 1907 "ljit_x86.dasc"
    } else {
      //|  jz =>target
      dasm_put(Dst, 4178, target);
# 1909 "ljit_x86.dasc"
    }
  } else {
    //|  addidx BASE, dest
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
# 1912 "ljit_x86.dasc"
    //|  mov L->savedpc, &J->nextins
    //|  call &luaV_arith, L, BASE, ecx, edx, ev
    //|  mov BASE, L->base
    dasm_put(Dst, 4182, Dt1(->savedpc), (ptrdiff_t)(J->nextins), ev, (ptrdiff_t)(luaV_arith), Dt1(->base));
# 1915 "ljit_x86.dasc"
  }

  if (hastail) {
    //| jmp <4
    //|.code
    dasm_put(Dst, 1644);
# 1920 "ljit_x86.dasc"
  }
}

/* ------------------------------------------------------------------------ */

static void jit_fallback_len(lua_State *L, StkId ra, const TValue *rb)
{
  switch (ttype(rb)) {
  case LUA_TTABLE:
    setnvalue(ra, cast_num(luaH_getn(hvalue(rb))));
    break;
  case LUA_TSTRING:
    setnvalue(ra, cast_num(tsvalue(rb)->len));
    break;
  default: {
    const TValue *tm = luaT_gettmbyobj(L, rb, TM_LEN);
    if (ttisfunction(tm)) {
      ptrdiff_t rasave = savestack(L, ra);
      setobj2s(L, L->top, tm);
      setobj2s(L, L->top+1, rb);
      luaD_checkstack(L, 2);
      L->top += 2;
      luaD_call(L, L->top - 2, 1);
      ra = restorestack(L, rasave);
      L->top--;
      setobjs2s(L, ra, L->top);
    } else {
      luaG_typeerror(L, rb, "get length of");
    }
    break;
  }
  }
}

static void jit_op_len(jit_State *J, int dest, int rb)
{
  switch (ttype(hint_get(J, TYPE))) {
  case LUA_TTABLE:
    jit_deopt_target(J, 0);
    //|   istable rb
    //|  mov TABLE:ecx, BASE[rb].value
    //|   jne L_DEOPTIMIZE		// TYPE hint was wrong?
    //|  call &luaH_getn, TABLE:ecx
    //|  mov TMP1, eax
    //|  fild dword TMP1
    //|  fstp qword BASE[dest].value
    //|  settt BASE[dest], LUA_TNUMBER
    dasm_put(Dst, 4203, Dt2([rb].tt), Dt2([rb].value), (ptrdiff_t)(luaH_getn), Dt2([dest].value), Dt2([dest].tt));
# 1967 "ljit_x86.dasc"
    break;
  case LUA_TSTRING:
    jit_deopt_target(J, 0);
    //|   isstring rb
    //|  mov TSTRING:ecx, BASE[rb].value
    //|   jne L_DEOPTIMIZE		// TYPE hint was wrong?
    //|  fild aword TSTRING:ecx->tsv.len	// size_t
    //|  fstp qword BASE[dest].value
    //|  settt BASE[dest], LUA_TNUMBER
    dasm_put(Dst, 4236, Dt2([rb].tt), Dt2([rb].value), DtB(->tsv.len), Dt2([dest].value), Dt2([dest].tt));
# 1976 "ljit_x86.dasc"
    break;
  default:
    //|  lea TVALUE:ecx, BASE[rb]
    //|  addidx BASE, dest
    dasm_put(Dst, 3216, Dt2([rb]));
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
# 1980 "ljit_x86.dasc"
    //|  mov L->savedpc, &J->nextins
    //|  call &jit_fallback_len, L, BASE, TVALUE:ecx
    //|  mov BASE, L->base
    dasm_put(Dst, 4261, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(jit_fallback_len), Dt1(->base));
# 1983 "ljit_x86.dasc"
    break;
  }
}

static void jit_op_not(jit_State *J, int dest, int rb)
{
  /* l_isfalse() without a branch -- truly devious. */
  /* ((value & tt) | (tt>>1)) is only zero for nil/false. */
  /* Assumes: LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  //|  mov eax, BASE[rb].tt
  //|  mov ecx, BASE[rb].value
  //|  mov edx, 1
  //|  and ecx, eax
  //|  shr eax, 1
  //|  or ecx, eax
  //|  xor eax, eax
  //|  cmp ecx, edx
  //|  adc eax, eax
  //|  mov BASE[dest].tt, edx
  //|  mov BASE[dest].value, eax
  dasm_put(Dst, 4282, Dt2([rb].tt), Dt2([rb].value), Dt2([dest].tt), Dt2([dest].value));
# 2003 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

static void jit_op_concat(jit_State *J, int dest, int first, int last)
{
  int num = last-first+1;
  if (num == 2 && ttisstring(hint_get(J, TYPE))) {  /* Optimize common case. */
    //|  addidx BASE, first
    if (first) {
    dasm_put(Dst, 787, first*sizeof(TValue));
    }
# 2012 "ljit_x86.dasc"
    //|  call ->CONCAT_STR2
    //|  setsvalue BASE[dest], eax
    dasm_put(Dst, 4312, Dt2([dest].value), Dt2([dest].tt));
# 2014 "ljit_x86.dasc"
  } else {  /* Generic fallback. */
    //|  mov L->savedpc, &J->nextins
    //|  call &luaV_concat, L, num, last
    //|  mov BASE, L->base
    dasm_put(Dst, 4326, Dt1(->savedpc), (ptrdiff_t)(J->nextins), num, last, (ptrdiff_t)(luaV_concat), Dt1(->base));
# 2018 "ljit_x86.dasc"
    if (dest != first) {
      //|  copyslot BASE[dest], BASE[first]
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([first].tt), Dt2([first].value), Dt2([dest].tt), Dt2([dest].value));
      } else {
      dasm_put(Dst, 839, Dt2([first].value), Dt2([first].value.na[1]), Dt2([first].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
      }
# 2020 "ljit_x86.dasc"
    }
  }
  jit_checkGC(J);  /* Always do this, even for the optimized variant. */

  //|.jsub CONCAT_STR2			// Concatenate two strings.
# 2083 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

static void jit_op_eq(jit_State *J, int cond, int rkb, int rkc)
{
  int target = jit_jmp_target(J);
  int condtarget = cond ? (J->nextpc+1) : target;
  jit_assert(cond == 0 || cond == 1);

  /* Comparison of two constants. Evaluate at compile time. */
  if (ISK(rkb&rkc)) {
    if ((rkb == rkc) == cond) {  /* Constants are already unique. */
      //|  jmp =>target
      dasm_put(Dst, 665, target);
# 2097 "ljit_x86.dasc"
    }
    return;
  }

  if (ISK(rkb|rkc)) {  /* Compare a variable and a constant. */
    const TValue *kk;
    if (ISK(rkb)) { int t = rkc; rkc = rkb; rkb = t; }  /* rkc holds const. */
    kk = &J->pt->k[INDEXK(rkc)];
    switch (ttype(kk)) {
    case LUA_TNIL:
      //|  isnil rkb
      dasm_put(Dst, 4517, Dt2([rkb].tt));
# 2108 "ljit_x86.dasc"
      break;
    case LUA_TBOOLEAN:
      if (bvalue(kk)) {
	//|  mov eax, BASE[rkb].tt
	//|  mov ecx, BASE[rkb].value
	//|  dec eax
	//|  dec ecx
	//|  or eax, ecx
	dasm_put(Dst, 4522, Dt2([rkb].tt), Dt2([rkb].value));
# 2116 "ljit_x86.dasc"
      } else {
	//|  mov eax, BASE[rkb].tt
	//|  dec eax
	//|  or eax, BASE[rkb].value
	dasm_put(Dst, 4533, Dt2([rkb].tt), Dt2([rkb].value));
# 2120 "ljit_x86.dasc"
      }
      break;
    case LUA_TNUMBER:
      //|// Note: bitwise comparison is not faster (and needs to handle -0 == 0).
      //|  isnumber rkb
      //|  jne =>condtarget
      //|  fld qword BASE[rkb].value
      //|  fld qword [&kk->value]
      //|  fcomparepp
      dasm_put(Dst, 4541, Dt2([rkb].tt), condtarget, Dt2([rkb].value), &kk->value);
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 3305);
      } else {
      dasm_put(Dst, 3310);
      }
# 2129 "ljit_x86.dasc"
      //|  jp =>condtarget  // Unordered means not equal.
      dasm_put(Dst, 4135, condtarget);
# 2130 "ljit_x86.dasc"
      break;
    case LUA_TSTRING:
      //|  isstring rkb
      //|  jne =>condtarget
      //|  cmp aword BASE[rkb].value, &rawtsvalue(kk)
      dasm_put(Dst, 4555, Dt2([rkb].tt), condtarget, Dt2([rkb].value), (ptrdiff_t)(rawtsvalue(kk)));
# 2135 "ljit_x86.dasc"
      break;
    default: jit_assert(0); break;
    }
  } else {  /* Compare two variables. */
    //|  mov eax, BASE[rkb].tt
    //|  cmp eax, BASE[rkc].tt
    //|  jne =>condtarget
    dasm_put(Dst, 4567, Dt2([rkb].tt), Dt2([rkc].tt), condtarget);
# 2142 "ljit_x86.dasc"
    switch (ttype(hint_get(J, TYPE))) {
    case LUA_TNUMBER:
      jit_deopt_target(J, 0);
      //|// Note: bitwise comparison is not an option (-0 == 0, NaN ~= NaN).
      //|  cmp eax, LUA_TNUMBER; jne L_DEOPTIMIZE
      //|  fld qword BASE[rkb].value
      //|  fld qword BASE[rkc].value
      //|  fcomparepp
      dasm_put(Dst, 4577, Dt2([rkb].value), Dt2([rkc].value));
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 3305);
      } else {
      dasm_put(Dst, 3310);
      }
# 2150 "ljit_x86.dasc"
      //|  jp =>condtarget  // Unordered means not equal.
      dasm_put(Dst, 4135, condtarget);
# 2151 "ljit_x86.dasc"
      break;
    case LUA_TSTRING:
      jit_deopt_target(J, 0);
      //|  cmp eax, LUA_TSTRING; jne L_DEOPTIMIZE
      //|  mov ecx, BASE[rkb].value
      //|  cmp ecx, BASE[rkc].value
      dasm_put(Dst, 4592, Dt2([rkb].value), Dt2([rkc].value));
# 2157 "ljit_x86.dasc"
      break;
    default:
      //|// Generic equality comparison fallback.
      //|  lea edx, BASE[rkc]
      //|  lea ecx, BASE[rkb]
      //|  mov L->savedpc, &J->nextins
      //|  call &luaV_equalval, L, ecx, edx
      //|  dec eax
      //|  mov BASE, L->base
      dasm_put(Dst, 4607, Dt2([rkc]), Dt2([rkb]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_equalval), Dt1(->base));
# 2166 "ljit_x86.dasc"
      break;
    }
  }
  if (cond) {
    //|  je =>target
    dasm_put(Dst, 4178, target);
# 2171 "ljit_x86.dasc"
  } else {
    //|  jne =>target
    dasm_put(Dst, 1479, target);
# 2173 "ljit_x86.dasc"
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_test(jit_State *J, int cond, int dest, int src)
{
  int target = jit_jmp_target(J);

  /* l_isfalse() without a branch. But this time preserve tt/value. */
  /* (((value & tt) * 2 + tt) >> 1) is only zero for nil/false. */
  /* Assumes: 3*tt < 2^32, LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  //|  mov eax, BASE[src].tt
  //|  mov ecx, BASE[src].value
  //|  mov edx, eax
  //|  and edx, ecx
  //|  lea edx, [eax+edx*2]
  //|  shr edx, 1
  dasm_put(Dst, 4635, Dt2([src].tt), Dt2([src].value));
# 2191 "ljit_x86.dasc"

  /* Check if we can omit the stack copy. */
  if (dest == src) {  /* Yes, invert branch condition. */
    if (cond) {
      //|  jnz =>target
      dasm_put(Dst, 1479, target);
# 2196 "ljit_x86.dasc"
    } else {
      //|  jz =>target
      dasm_put(Dst, 4178, target);
# 2198 "ljit_x86.dasc"
    }
  } else {  /* No, jump around copy code. */
    if (cond) {
      //|  jz >1
      dasm_put(Dst, 4651);
# 2202 "ljit_x86.dasc"
    } else {
      //|  jnz >1
      dasm_put(Dst, 4656);
# 2204 "ljit_x86.dasc"
    }
    //|  mov edx, BASE[src].value.na[1]
    //|  mov BASE[dest].tt, eax
    //|  mov BASE[dest].value, ecx
    //|  mov BASE[dest].value.na[1], edx
    //|  jmp =>target
    //|1:
    dasm_put(Dst, 4661, Dt2([src].value.na[1]), Dt2([dest].tt), Dt2([dest].value), Dt2([dest].value.na[1]), target);
# 2211 "ljit_x86.dasc"
  }
}

static void jit_op_jmp(jit_State *J, int target)
{
  //|  jmp =>target
  dasm_put(Dst, 665, target);
# 2217 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

enum { FOR_IDX, FOR_LIM, FOR_STP, FOR_EXT };

static const char *const jit_for_coerce_error[] = {
  LUA_QL("for") " initial value must be a number",
  LUA_QL("for") " limit must be a number",
  LUA_QL("for") " step must be a number",
};

/* Try to coerce for slots with strings to numbers in place or complain. */
static void jit_for_coerce(lua_State *L, TValue *o)
{
  int i;
  for (i = FOR_IDX; i <= FOR_STP; i++, o++) {
    lua_Number num;
    if (ttisnumber(o)) continue;
    if (ttisstring(o) && luaO_str2d(svalue(o), &num)) {
      setnvalue(o, num);
    } else {
      luaG_runerror(L, jit_for_coerce_error[i]);
    }
  }
}

static void jit_op_forprep(jit_State *J, int ra, int target)
{
  const TValue *step = hint_get(J, FOR_STEP_K);
  if (ttisnumber(step)) {
    //|  isnumber2 ra+FOR_IDX, ra+FOR_LIM; jne L_DEOPTIMIZEF
    //|4:
    //|  fld qword BASE[ra+FOR_LIM].value // [lim]
    //|  fld qword BASE[ra+FOR_IDX].value // [idx lim]
    //|  fst qword BASE[ra+FOR_EXT].value	// extidx = idx
    //|  fcomparepp			// idx >< lim ?
    dasm_put(Dst, 4678, Dt2([ra+FOR_IDX].tt), Dt2([ra+FOR_LIM].tt), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3305);
    } else {
    dasm_put(Dst, 3310);
    }
# 2254 "ljit_x86.dasc"
    //|  settt BASE[ra+FOR_EXT], LUA_TNUMBER
    dasm_put(Dst, 1309, Dt2([ra+FOR_EXT].tt));
# 2255 "ljit_x86.dasc"
    if (nvalue(step) < (lua_Number)0) {
      //|  jb =>target+1			// step < 0 && idx < lim: skip loop.
      dasm_put(Dst, 4139, target+1);
# 2257 "ljit_x86.dasc"
    } else {
      //|  ja =>target+1			// step >= 0 && idx > lim: skip loop.
      dasm_put(Dst, 4147, target+1);
# 2259 "ljit_x86.dasc"
    }
  } else {
    //|4:
    //|  isnumber3 ra+FOR_IDX, ra+FOR_LIM, ra+FOR_STP
    //|   mov eax, BASE[ra+FOR_STP].value.na[1]	// Sign bit is in hi dword.
    //|  jne L_DEOPTIMIZEF
    //|  fld qword BASE[ra+FOR_LIM].value	// [lim]        (FP stack notation)
    //|  fld qword BASE[ra+FOR_IDX].value	// [idx lim]
    //|  test eax, eax			// step >< 0 ?
    //|  fst qword BASE[ra+FOR_EXT].value	// extidx = idx
    //|  js >1
    //|  fxch				// if (step > 0) [lim idx]
    //|1:
    //|  fcomparepp			// step > 0 ? lim < idx : idx < lim
    dasm_put(Dst, 4707, Dt2([ra+FOR_IDX].tt), Dt2([ra+FOR_LIM].tt), Dt2([ra+FOR_STP].tt), Dt2([ra+FOR_STP].value.na[1]), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3305);
    } else {
    dasm_put(Dst, 3310);
    }
# 2273 "ljit_x86.dasc"
    //|  settt BASE[ra+FOR_EXT], LUA_TNUMBER
    //|  jb =>target+1			// Skip loop.
    dasm_put(Dst, 4756, Dt2([ra+FOR_EXT].tt), target+1);
# 2275 "ljit_x86.dasc"
  }
  if (ttisnumber(hint_get(J, TYPE))) {
    jit_deopt_target(J, 0);
  } else {
    //|.tail
    dasm_put(Dst, 679);
# 2280 "ljit_x86.dasc"
    //|L_DEOPTLABEL:  // Recycle as fallback label.
    //|  // Fallback for strings as loop vars. No need to make this fast.
    //|  lea eax, BASE[ra]
    //|  mov L->savedpc, &J->nextins
    //|  call &jit_for_coerce, L, eax	// Coerce strings or throw error.
    //|  jmp <4				// Easier than reloading eax.
    //|.code
    dasm_put(Dst, 4767, Dt2([ra]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(jit_for_coerce));
# 2287 "ljit_x86.dasc"
  }
}

static void jit_op_forloop(jit_State *J, int ra, int target)
{
  const TValue *step = hint_getpc(J, FOR_STEP_K, target-1);
  if (ttisnumber(step)) {
    //|  fld qword BASE[ra+FOR_LIM].value	// [lim]        (FP stack notation)
    //|  fld qword BASE[ra+FOR_IDX].value	// [idx lim]
    //|  fadd qword BASE[ra+FOR_STP].value // [nidx lim]
    //|  fst qword BASE[ra+FOR_EXT].value	// extidx = nidx
    //|  fst qword BASE[ra+FOR_IDX].value	// idx = nidx
    //|  settt BASE[ra+FOR_EXT], LUA_TNUMBER
    //|  fcomparepp			// nidx >< lim ?
    dasm_put(Dst, 4790, Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_STP].value), Dt2([ra+FOR_EXT].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].tt));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3305);
    } else {
    dasm_put(Dst, 3310);
    }
# 2301 "ljit_x86.dasc"
    if (nvalue(step) < (lua_Number)0) {
      //|  jae =>target			// step < 0 && nidx >= lim: loop again.
      dasm_put(Dst, 4151, target);
# 2303 "ljit_x86.dasc"
    } else {
      //|  jbe =>target			// step >= 0 && nidx <= lim: loop again.
      dasm_put(Dst, 4143, target);
# 2305 "ljit_x86.dasc"
    }
  } else {
    //|  mov eax, BASE[ra+FOR_STP].value.na[1]	// Sign bit is in hi dword.
    //|  fld qword BASE[ra+FOR_LIM].value	// [lim]        (FP stack notation)
    //|  fld qword BASE[ra+FOR_IDX].value	// [idx lim]
    //|  fld qword BASE[ra+FOR_STP].value	// [stp idx lim]
    //|  faddp st1			// [nidx lim]
    //|  fst qword BASE[ra+FOR_IDX].value	// idx = nidx
    //|  fst qword BASE[ra+FOR_EXT].value	// extidx = nidx
    //|  settt BASE[ra+FOR_EXT], LUA_TNUMBER
    //|  test eax, eax			// step >< 0 ?
    //|  js >1
    //|  fxch				// if (step > 0) [lim nidx]
    //|1:
    //|  fcomparepp			// step > 0 ? lim >= nidx : nidx >= lim
    dasm_put(Dst, 4813, Dt2([ra+FOR_STP].value.na[1]), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_STP].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value), Dt2([ra+FOR_EXT].tt));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3305);
    } else {
    dasm_put(Dst, 3310);
    }
# 2320 "ljit_x86.dasc"
    //|  jae =>target			// Loop again.
    dasm_put(Dst, 4151, target);
# 2321 "ljit_x86.dasc"
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_tforloop(jit_State *J, int ra, int nresults)
{
  int target = jit_jmp_target(J);
  int i;
  if (jit_inline_tforloop(J, ra, nresults, target)) return;  /* Inlined? */
  for (i = 2; i >= 0; i--) {
    //|  copyslot BASE[ra+i+3], BASE[ra+i]  // Copy ctlvar/state/callable.
    if (J->flags & JIT_F_CPU_SSE2) {
    dasm_put(Dst, 821, Dt2([ra+i].tt), Dt2([ra+i].value), Dt2([ra+i+3].tt), Dt2([ra+i+3].value));
    } else {
    dasm_put(Dst, 839, Dt2([ra+i].value), Dt2([ra+i].value.na[1]), Dt2([ra+i].tt), Dt2([ra+i+3].value), Dt2([ra+i+3].value.na[1]), Dt2([ra+i+3].tt));
    }
# 2333 "ljit_x86.dasc"
  }
  jit_op_call(J, ra+3, 2, nresults);
  //|  isnil ra+3; je >1
  //|  copyslot BASE[ra+2], BASE[ra+3]	// Save control variable.
  dasm_put(Dst, 4851, Dt2([ra+3].tt));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([ra+3].tt), Dt2([ra+3].value), Dt2([ra+2].tt), Dt2([ra+2].value));
  } else {
  dasm_put(Dst, 839, Dt2([ra+3].value), Dt2([ra+3].value.na[1]), Dt2([ra+3].tt), Dt2([ra+2].value), Dt2([ra+2].value.na[1]), Dt2([ra+2].tt));
  }
# 2337 "ljit_x86.dasc"
  //|  jmp =>target
  //|1:
  dasm_put(Dst, 4673, target);
# 2339 "ljit_x86.dasc"
}

/* ------------------------------------------------------------------------ */

static void jit_op_close(jit_State *J, int ra)
{
  if (ra) {
    //|  lea eax, BASE[ra]
    //|  mov ARG2, eax
    dasm_put(Dst, 4860, Dt2([ra]));
# 2348 "ljit_x86.dasc"
  } else {
    //|  mov ARG2, BASE
    dasm_put(Dst, 4868);
# 2350 "ljit_x86.dasc"
  }
  //|  call &luaF_close, L  // , StkId level (ARG2)
  dasm_put(Dst, 1734, (ptrdiff_t)(luaF_close));
# 2352 "ljit_x86.dasc"
}

static void jit_op_closure(jit_State *J, int dest, int ptidx)
{
  Proto *npt = J->pt->p[ptidx];
  int nup = npt->nups;
  //|  getLCL edi				// LCL:edi is callee-saved.
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 4873, Dt2([-1].value));
  } else {
  dasm_put(Dst, 4877, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
# 2359 "ljit_x86.dasc"
  //|  mov edx, LCL:edi->env
  //|  call &luaF_newLclosure, L, nup, edx
  //|  mov LCL->p, &npt			// Store new proto in returned closure.
  //|  mov aword BASE[dest].value, LCL	// setclvalue()
  //|  settt BASE[dest], LUA_TFUNCTION
  dasm_put(Dst, 4887, Dt5(->env), nup, (ptrdiff_t)(luaF_newLclosure), Dt5(->p), (ptrdiff_t)(npt), Dt2([dest].value), Dt2([dest].tt));
# 2364 "ljit_x86.dasc"
  /* Process pseudo-instructions for upvalues. */
  if (nup > 0) {
    const Instruction *uvcode = J->nextins;
    int i, uvuv;
    /* Check which of the two types we need. */
    for (i = 0, uvuv = 0; i < nup; i++)
      if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) uvuv++;
    /* Copy upvalues from parent first. */
    if (uvuv) {
      /* LCL:eax->upvals (new closure) <-- LCL:edi->upvals (own closure). */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) {
	  //|  mov UPVAL:edx, LCL:edi->upvals[GETARG_B(uvcode[i])]
	  //|  mov LCL->upvals[i], UPVAL:edx
	  dasm_put(Dst, 4919, Dt5(->upvals[GETARG_B(uvcode[i])]), Dt5(->upvals[i]));
# 2378 "ljit_x86.dasc"
	}
    }
    /* Next find or create upvalues for our own stack slots. */
    if (nup > uvuv) {
      //|  mov LCL:edi, LCL  // Move new closure to callee-save register. */
      dasm_put(Dst, 909);
# 2383 "ljit_x86.dasc"
      /* LCL:edi->upvals (new closure) <-- upvalue for stack slot. */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_MOVE) {
	  int rb = GETARG_B(uvcode[i]);
	  if (rb) {
	    //|  lea eax, BASE[rb]
	    //|  mov ARG2, eax
	    dasm_put(Dst, 4860, Dt2([rb]));
# 2390 "ljit_x86.dasc"
	  } else {
	    //|  mov ARG2, BASE
	    dasm_put(Dst, 4868);
# 2392 "ljit_x86.dasc"
	  }
	  //|  call &luaF_findupval, L  // , StkId level (ARG2)
	  //|  mov LCL:edi->upvals[i], UPVAL:eax
	  dasm_put(Dst, 4926, (ptrdiff_t)(luaF_findupval), Dt5(->upvals[i]));
# 2395 "ljit_x86.dasc"
	}
    }
    J->combine += nup;  /* Skip pseudo-instructions. */
  }
  jit_checkGC(J);
}

/* ------------------------------------------------------------------------ */

static void jit_op_vararg(jit_State *J, int dest, int num)
{
  if (num < 0) {  /* Copy all varargs. */
    //|// Copy [ci->func+1+pt->numparams, BASE) -> [BASE+dest, *).
    //|1:
    //|  mov CI, L->ci
    //|  mov edx, CI->func
    //|  add edx, (1+J->pt->numparams)*#TVALUE  // Start of varargs.
    //|
    //|  // luaD_checkstack(L, nvararg) with nvararg = L->base - vastart.
    //|  // This is a slight overallocation (BASE[dest+nvararg] would be enough).
    //|  // We duplicate OP_VARARG behaviour so we can use luaD_growstack().
    //|  lea eax, [BASE+BASE+J->pt->maxstacksize*#TVALUE]  // L->base + L->top
    //|  sub eax, edx			// L->top + (L->base - vastart)
    //|  cmp eax, L->stack_last
    //|  jae >5				// Need to grow stack?
    //|
    //|  lea TOP, BASE[dest]
    //|  cmp edx, BASE
    //|  jnb >3
    //|2:  // Copy loop.
    //|  mov eax, [edx]
    //|  add edx, aword*1
    //|  mov [TOP], eax
    //|  add TOP, aword*1
    //|  cmp edx, BASE
    //|  jb <2
    //|3:
    //|// This is an open ins. Must keep TOP for next instruction.
    //|
    //|.tail
    dasm_put(Dst, 4935, Dt1(->ci), Dt4(->func), (1+J->pt->numparams)*sizeof(TValue), J->pt->maxstacksize*sizeof(TValue), Dt1(->stack_last), Dt2([dest]));
# 2435 "ljit_x86.dasc"
    //|5:  // Grow stack for varargs.
    //|  sub eax, L->top
    //|  TValuediv eax
    //|  call &luaD_growstack, L, eax
    //|  mov BASE, L->base
    //|  jmp <1  // Just restart op to avoid saving/restoring regs.
    //|.code
    dasm_put(Dst, 4991, Dt1(->top), (ptrdiff_t)(luaD_growstack), Dt1(->base));
# 2442 "ljit_x86.dasc"
  } else if (num > 0) {  /* Copy limited number of varargs. */
    //|// Copy [ci->func+1+pt->numparams, BASE) -> [BASE+dest, BASE+dest+num).
    //|  mov CI, L->ci
    //|  mov edx, CI->func
    //|  add edx, (1+J->pt->numparams)*#TVALUE
    //|  lea TOP, BASE[dest]
    //|  lea ecx, BASE[dest+num]
    //|  cmp edx, BASE			// No varargs present: only fill.
    //|  jnb >2
    //|
    //|1:  // Copy loop.
    //|  mov eax, [edx]
    //|  add edx, aword*1
    //|  mov [TOP], eax
    //|  add TOP, aword*1
    //|  cmp TOP, ecx			// Stop if all dest slots got a vararg.
    //|  jnb >4
    //|  cmp edx, BASE			// Continue if more varargs present.
    //|  jb <1
    //|
    //|2:					// Fill remaining slots with nils.
    //|  xor eax, eax			// Assumes: LUA_TNIL == 0
    //|3:  // Fill loop.
    //|  settt TOP[0], eax
    //|  add TOP, #TVALUE
    //|  cmp TOP, ecx
    //|  jb <3
    //|4:
    dasm_put(Dst, 5017, Dt1(->ci), Dt4(->func), (1+J->pt->numparams)*sizeof(TValue), Dt2([dest]), Dt2([dest+num]), Dt3([0].tt), sizeof(TValue));
# 2470 "ljit_x86.dasc"
  }
}

/* ------------------------------------------------------------------------ */

