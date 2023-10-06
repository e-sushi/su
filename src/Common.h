/*

    typedefs, functions, and macros used practically everywhere in the project
    as well as some platform/compiler checking

    this largely takes from kigu's original common.h

*/

#ifndef AMU_COMMON_H
#define AMU_COMMON_H

////////////////////////////////////////// compilers: COMPILER_CL, COMPILER_CLANG, COMPILER_GCC
//// compiler, platform, architecture //// platforms: OS_WINDOWS. OS_LINUX, OS_MAC
////////////////////////////////////////// architectures: ARCH_X64, ARCH_X86, ARCH_ARM64, ARCH_ARM32
//// CL Compiler //// (used for windows)
#if defined(_MSC_VER)
#  define COMPILER_CL 1
//#  pragma message("Compiler: cl")

#  if defined(_WIN32)
#    define OS_WINDOWS 1
#  else //_WIN32
#    error "unhandled compiler/platform combo"
#  endif //_WIN32

#  if defined(_M_AMD64)
#    define ARCH_X64 1
#  elif defined(_M_IX86) //_M_AMD64
#    define ARCH_X86 1
#  elif defined(_M_ARM64) //_M_IX86
#    define ARCH_ARM64 1
#  elif defined(_M_ARM) //_M_ARM64
#    define ARCH_ARM32 1
#  else //_M_ARM
#    error "unhandled architecture"
#  endif

//// CLANG Compiler //// (used for mac/linux)
#elif defined(__clang__) //_MSC_VER
#  define COMPILER_CLANG 1
//#  pragma message("Compiler: clang")

#  if   defined(__APPLE__) && defined(__MACH__)
#    define OS_MAC 1
#  elif defined(_WIN32)
#    define OS_WINDOWS 1
#  elif defined(__gnu_linux__)
#   undef M_PI
#   undef M_E
#	define OS_LINUX 1
#  else //__APPLE__ || __MACH__
#    error "unhandled compiler/platform combo"
#  endif //__APPLE__ || __MACH__

#  if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#    define ARCH_X64 1
#  elif defined(i386) || defined(__i386) || defined(__i386__) //__amd64__ || __amd64 || __x86_64 || __x86_64__
#    define ARCH_X86 1
#  elif defined(__aarch64__) //i386 || __i386__ || __i386__
#    define ARCH_ARM64 1
#  elif defined(__arm__) //__aarch64__
#    define ARCH_ARM32 1
#  else //__arm__
#    error "unhandled architecture"
#  endif

//// GCC Compiler //// (used for linux)
#elif defined(__GNUC__) || defined(__GNUG__) //__clang__
#  define COMPILER_GCC 1
//#  pragma message("Compiler: gcc")

#  if defined(__gnu_linux__)
#    define OS_LINUX 1
#  else //__gnu_linux__
#    error "unhandled compiler/platform combo"
#  endif

#  if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#    define ARCH_X64 1
#  elif defined(i386) || defined(__i386) || defined(__i386__) //__amd64__ || __amd64 || __x86_64 || __x86_64__
#    define ARCH_X86 1
#  elif defined(__aarch64__) //i386 || __i386__ || __i386__
#    define ARCH_ARM64 1
#  elif defined(__arm__) //__aarch64__
#    define ARCH_ARM32 1
#  else //__arm__
#    error "unhandled architecture"
#  endif

//// Unhandled Compiler ////
#else //__GNUC__ || __GNUG__
#  error "unhandled compiler"
#endif

#if !defined(COMPILER_CL)
#  define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
#  define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
#  define COMPILER_GCC 0
#endif

#if !defined(OS_WINDOWS)
#  define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#  define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#  define OS_MAC 0
#endif

#if !defined(ARCH_X64)
#  define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#  define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#  define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#  define ARCH_ARM32 0
#endif

//// Supported Compiler Features ////
#if COMPILER_CLANG || COMPILER_GCC
#  define COMPILER_FEATURE_TYPEOF 0
#else
#  define COMPILER_FEATURE_TYPEOF 0
#endif //#if COMPILER_CLANG || COMPILER_GCC

#if __cplusplus
#  define COMPILER_FEATURE_CPP 1
#  if   (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 20202L)
#    define COMPILER_FEATURE_CPP_20 1
#    define COMPILER_FEATURE_CPP_17 0
#    define COMPILER_FEATURE_CPP_14 0
#    define COMPILER_FEATURE_CPP_11 0
#    define COMPILER_FEATURE_CPP_98 0
#    define COMPILER_FEATURE_CPP_PRE98 0
#    define COMPILER_FEATURE_CPP_AUTO 1
#  elif (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define COMPILER_FEATURE_CPP_20 0
#    define COMPILER_FEATURE_CPP_17 1
#    define COMPILER_FEATURE_CPP_14 0
#    define COMPILER_FEATURE_CPP_11 0
#    define COMPILER_FEATURE_CPP_98 0
#    define COMPILER_FEATURE_CPP_PRE98 0
#    define COMPILER_FEATURE_CPP_AUTO 1
#  elif (__cplusplus >= 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
#    define COMPILER_FEATURE_CPP_20 0
#    define COMPILER_FEATURE_CPP_17 0
#    define COMPILER_FEATURE_CPP_14 1
#    define COMPILER_FEATURE_CPP_11 0
#    define COMPILER_FEATURE_CPP_98 0
#    define COMPILER_FEATURE_CPP_PRE98 0
#    define COMPILER_FEATURE_CPP_AUTO 1
#  elif (__cplusplus >= 201103L)
#    define COMPILER_FEATURE_CPP_20 0
#    define COMPILER_FEATURE_CPP_17 0
#    define COMPILER_FEATURE_CPP_14 0
#    define COMPILER_FEATURE_CPP_11 1
#    define COMPILER_FEATURE_CPP_98 0
#    define COMPILER_FEATURE_CPP_PRE98 0
#    define COMPILER_FEATURE_CPP_AUTO 1
#  elif (__cplusplus >= 199711L)
#    define COMPILER_FEATURE_CPP_20 0
#    define COMPILER_FEATURE_CPP_17 0
#    define COMPILER_FEATURE_CPP_14 0
#    define COMPILER_FEATURE_CPP_11 0
#    define COMPILER_FEATURE_CPP_98 1
#    define COMPILER_FEATURE_CPP_PRE98 0
#    define COMPILER_FEATURE_CPP_AUTO 0
#  else
#    define COMPILER_FEATURE_CPP_20 0
#    define COMPILER_FEATURE_CPP_17 0
#    define COMPILER_FEATURE_CPP_14 0
#    define COMPILER_FEATURE_CPP_11 0
#    define COMPILER_FEATURE_CPP_98 0
#    define COMPILER_FEATURE_CPP_PRE98 1
#    define COMPILER_FEATURE_CPP_AUTO 0
#  endif
#else
#  define COMPILER_FEATURE_CPP 0
#  define COMPILER_FEATURE_CPP_20 0
#  define COMPILER_FEATURE_CPP_17 0
#  define COMPILER_FEATURE_CPP_14 0
#  define COMPILER_FEATURE_CPP_11 0
#  define COMPILER_FEATURE_CPP_98 0
#  define COMPILER_FEATURE_CPP_PRE98 0
#  define COMPILER_FEATURE_CPP_AUTO 0
#endif //#if __cplusplus

///////////////////////// //NOTE this file is included is almost every other file of the project, so be frugal with includes here
//// common includes ////
/////////////////////////
#include <stdio.h>
#include <stddef.h> //size_t, ptrdiff_t
#include <stdlib.h> //malloc, calloc, free
#include <stdarg.h> //va_start, va_list, va_arg, va_end
#include <string.h> //memcpy, memset, strcpy, strlen, etc
#include <math.h>   //log2


///////////////////////
//// static macros //// for clarity
/////////////////////// 
#define local    static //inside a .cpp
#define persist  static //inside a function
#define global   static //inside a .h

/////////////////////////////////////
//// compiler-dependent builtins ////
/////////////////////////////////////
#if COMPILER_CL
#  define FORCE_INLINE __forceinline
#  define DebugBreakpoint __debugbreak()
#  define ByteSwap16(x) _byteswap_ushort(x)
#  define ByteSwap32(x) _byteswap_ulong(x)
#  define ByteSwap64(x) _byteswap_uint64(x)
#elif COMPILER_CLANG || COMPILER_GCC
#  define FORCE_INLINE inline __attribute__((always_inline))
#  if defined(__i386__) || defined(__x86_64__)
#    define DebugBreakpoint __builtin_debugtrap()
#  else
#    include "signal.h"
#    define DebugBreakpoint raise(SIGTRAP)
#  endif
#  define ByteSwap16(x) __builtin_bswap16(x)
#  define ByteSwap32(x) __builtin_bswap32(x)
#  define ByteSwap64(x) __builtin_bswap64(x)
#else
#  error "unhandled compiler"
#endif //#if COMPILER_CL

//// stack allocation ////
#if OS_WINDOWS
#  include <malloc.h>
#  define StackAlloc(bytes) _alloca(bytes)
#elif OS_LINUX || OS_MAC
#  include <alloca.h>
#  define StackAlloc(bytes) alloca(bytes)
#else
#  error "unhandled os for stack allocation"
#endif //#if OS_WINDOWS

//////////////////////
//// common types ////
//////////////////////
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef ptrdiff_t          spt;   //signed pointer type
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef size_t             upt;   //unsigned pointer type
typedef float              f32;
typedef double             f64;
typedef s32                b32;   //sized boolean type
typedef wchar_t            wchar;

//////////////////////////
//// common constants ////
//////////////////////////
global const u8  MAX_U8  = 0xFF;
global const u16 MAX_U16 = 0xFFFF;
global const u32 MAX_U32 = 0xFFFFFFFF;
global const u64 MAX_U64 = 0xFFFFFFFFFFFFFFFF;

global const s8  MIN_S8  = -127 - 1;
global const s8  MAX_S8  = 127;
global const s16 MIN_S16 = -32767 - 1;
global const s16 MAX_S16 = 32767;
global const s32 MIN_S32 = -2147483647 - 1;
global const s32 MAX_S32 = 2147483647;
global const s64 MIN_S64 = -9223372036854775807 - 1;
global const s64 MAX_S64 = 9223372036854775807;

global const f32 MAX_F32 = 3.402823466e+38f;
global const f32 MIN_F32 = -MAX_F32;
global const f64 MAX_F64 = 1.79769313486231e+308;
global const f64 MIN_F64 = -MAX_F64;

global const f32 M_ONETHIRD        = 0.33333333333f;
global const f32 M_ONESIXTH        = 0.16666666667f;
global const f32 M_EPSILON         = 0.00001f;
global const f32 M_FOURTHPI        = 0.78539816339f;
global const f32 M_HALFPI          = 1.57079632679f;
#undef M_PI
global const f32 M_PI              = 3.14159265359f;
global const f64 M_PId             = 3.14159265358979323846;
global const f64 π                 = M_PId;
global const f32 M_2PI             = 6.28318530718f;
global const f32 M_TAU             = M_2PI;
#undef M_E
global const f32 M_E               = 2.71828182846f;
global const f32 M_SQRT_TWO        = 1.41421356237f;
global const f32 M_SQRT_THREE      = 1.73205080757f;
global const f32 M_HALF_SQRT_TWO   = 0.707106781187f;
global const f32 M_HALF_SQRT_THREE = 0.866025403784f;

global const u32 npos = -1;

/////////////////////// //NOTE some are two level so you can use the result of a macro expansion (STRINGIZE, GLUE, etc)
//// common macros ////
///////////////////////
#define STMNT(s) do{ s }while(0)
#define UNUSED_VAR(a) ((void)(a))
#define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)
#define GLUE_(a,b) a##b
#define GLUE(a,b) GLUE_(a,b)

#define Kilobytes(a) (((u64)(a)) << 10)
#define Megabytes(a) (((u64)(a)) << 20)
#define Gigabytes(a) (((u64)(a)) << 30)
#define Terabytes(a) (((u64)(a)) << 40)

#define Thousand(a) (((u64)(a)) * 1000)
#define Million(a)  (((u64)(a)) * 1000000)
#define Billion(a)  (((u64)(a)) * 1000000000)

#define Nanoseconds(ms) (ms * 1000000);
#define Seconds(ms) (ms          / 1000)
#define Minutes(ms) (Seconds(ms) / 60)
#define Hours(ms)   (Minutes(ms) / 60)
#define Days(ms)    (Hours(ms)   / 24)
#define SecondsToMS(n) (n * 1000)
#define MinutesToMS(n) (n * SecondsToMS(60))
#define HoursToMS(n)   (n * MinutesToMS(60))
#define DaysToMS(n)    (n * HoursToMS(24))

/////////////////////// //NOTE the ... is for a programmer message at the assert; it is unused otherwise
//// assert macros //// //TODO(delle) assert message popup thru the OS
/////////////////////// 
#define AssertAlways(expression, ...) STMNT( if(!(expression)){*(volatile int*)0 = 0;} ) //works regardless of SLOW or INTERNAL
#define AssertBreakpoint(expression, ...) STMNT( if(!(expression)){ DebugBreakpoint; } )
#define StaticAssertAlways(expression, ...) char GLUE(GLUE(__ignore__, GLUE(__LINE__,__default__)),__COUNTER__)[(expression)?1:-1]

#if BUILD_INTERNAL
#  define Assert(expression, ...) AssertBreakpoint(expression)
#  define StaticAssert(expression, ...) StaticAssertAlways(expression)
#elif BUILD_SLOW
#  define Assert(expression, ...) AssertAlways(expression)
#  define StaticAssert(expression, ...) StaticAssertAlways(expression)
#else
#  define Assert(expression, ...)
#  define StaticAssert(expression, ...) 
#endif //#if BUILD_INTERNAL

#define NotImplemented AssertAlways(false, "not implemented yet")
#define InvalidPath Assert(false, "invalid path")
#define TestMe AssertBreakpoint(false, "this needs to be tested")
#define FixMe AssertBreakpoint(false, "this is broken in some way")
#define DontCompile (0=__dont_compile_this__)
#define WarnFuncNotImplemented(extra) LogW("FUNC", "Function ", __FUNCTION__, " has not been implemented or is not finished", (extra ? "\n" : ""), extra);

//// defer statement ////
//ref: https://stackoverflow.com/a/42060129 by pmttavara
//defers execution inside the block to the end of the current scope; this works by placing
//that code in a lambda specific to that line that a dummy object will call in its destructor
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#  define DEFER_(LINE) zz_defer##LINE
#  define DEFER(LINE) DEFER_(LINE)
#  define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif //#ifndef defer

/////////////////////////
//// for-loop macros ////
/////////////////////////
#define forX(var_name,iterations) for(int var_name=0; var_name<(iterations); ++var_name)
#define forX_reverse(var_name,iterations) for(int var_name=(iterations)-1; var_name>=0; --var_name)
#define forI(iterations) for(int i=0; i<(iterations); ++i)
#define forI_reverse(iterations) for(int i=(iterations)-1; i>=0; --i)
#if COMPILER_FEATURE_TYPEOF
#  define For(start,count) for(typeof(*(start))* it = start; it < start+(count); ++it)
#  define ForX(var_name,start,count) for(typeof(*(start))* var_name = start; var_name < start+(count); ++var_name)
#endif //#if COMPILER_FEATURE_TYPEOF

//// terminal colors ////
#define VTS_Default         "\x1b[0m"
#define VTS_Bold            "\x1b[1m"
#define VTS_NoBold          "\x1b[22m"
#define VTS_Underline       "\x1b[4m"
#define VTS_NoUnderline     "\x1b[24m"
#define VTS_Negative        "\x1b[7m"
#define VTS_Positive        "\x1b[27m"
#define VTS_BlackFg         "\x1b[30m"
#define VTS_RedFg           "\x1b[31m"
#define VTS_GreenFg         "\x1b[32m"
#define VTS_YellowFg        "\x1b[33m"
#define VTS_BlueFg          "\x1b[34m"
#define VTS_MagentaFg       "\x1b[35m"
#define VTS_CyanFg          "\x1b[36m"
#define VTS_WhiteFg         "\x1b[37m"
#define VTS_ExtendedFg      "\x1b[38m"
#define VTS_DefaultFg       "\x1b[39m"
#define VTS_BlackBg         "\x1b[40m"
#define VTS_RedBg           "\x1b[41m"
#define VTS_GreenBg         "\x1b[42m"
#define VTS_YellowBg        "\x1b[43m"
#define VTS_BlueBg          "\x1b[44m"
#define VTS_MagentaBg       "\x1b[45m"
#define VTS_CyanBg          "\x1b[46m"
#define VTS_WhiteBg         "\x1b[47m"
#define VTS_RGBBg(r,g,b)    "\x1b[48;2;" STRINGIZE(r) ";" STRINGIZE(g) ";" STRINGIZE(b) "m"
#define VTS_DefaultBg       "\x1b[49m"
#define VTS_BrightBlackFg   "\x1b[90m"
#define VTS_BrightRedFg     "\x1b[91m"
#define VTS_BrightGreenFg   "\x1b[92m"
#define VTS_BrightYellowFg  "\x1b[93m"
#define VTS_BrightBlueFg    "\x1b[94m"
#define VTS_BrightMagentaFg "\x1b[95m"
#define VTS_BrightCyanFg    "\x1b[96m"
#define VTS_BrightWhiteFg   "\x1b[97m"
#define VTS_BrightBlackBg   "\x1b[100m"
#define VTS_BrightRedBg     "\x1b[101m"
#define VTS_BrightGreenBg   "\x1b[102m"
#define VTS_BrightYellowBg  "\x1b[103m"
#define VTS_BrightBlueBg    "\x1b[104m"
#define VTS_BrightMagentaBg "\x1b[105m"
#define VTS_BrightCyanBg    "\x1b[106m"
#define VTS_BrightWhiteBg   "\x1b[107m"

#define ErrorFormat(str)   VTS_RedFg    str VTS_Default
#define WarningFormat(str) VTS_YellowFg str VTS_Default
#define SuccessFormat(str) VTS_GreenFg  str VTS_Default

#endif // AMU_COMMON_H
