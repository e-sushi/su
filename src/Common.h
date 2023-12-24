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
#  else
#    error "amu must be compiled with the C++20 standard"
#  endif
#endif //#if __cplusplus

///////////////////////// //NOTE this file is included is almost every other file of the project, so be frugal with includes here
//// common includes ////
/////////////////////////
/// TODO(sushi) move these to the cpp files in which they are used 
///             so they don't need to be included everywhere
#include <stdio.h>
#include <stddef.h> //size_t, ptrdiff_t
#include <stdlib.h> //malloc, calloc, free
#include <stdarg.h> //va_start, va_list, va_arg, va_end
#include <string.h> //memcpy, memset, strcpy, strlen, etc
#include <math.h>   //log2

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

#endif // AMU_COMMON_H
