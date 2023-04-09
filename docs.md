# amu docs

The style of this documentation is inspired by the ISO/IEC 9899:TC3.

## Table of contents
1. [Notation](#notation)
1. [Identifiers](#identifiers)
1. [Labels](#labels)
1. [Entities](#entities)
1. [Keywords](#keywords)
1. [Literals](#literals)
    1. [Integers](#integers)
    1. [Floats](#floats)
    1. [Arrays](#arrays)
    1. [Strings](#strings)
1. [Declarations](#declarations)
    1. [Variables](#variables)
    1. [Structures](#structures)
1. [Scopes](#scopes)
1. [Directives](#directives)
1. [Using](#using)

## Notation 
The syntax of amu is defined as a context free grammar in 'BN.txt', in the root of its repo. amu's syntax is inspired by C, jai, odin, and python. 

## Identifiers
An identifier in amu must start with any alphabetic ascii character, an underscore, or any unicode codepoint outside of the range of ascii, and may be followed by any character satisfying the previous requirement plus any ascii digit. For example:
```
a 
a10
identifier
my_variable10
_
π_
σ2
ππ‽_←꙰1
```
are all considered valid identifiers, while something like `2var`, is not. Identifiers are what label any declaration in amu. This includes variables, functions, structures, and modules. 

## Labels
**Labels**, technically known as **name bindings**, are [identifiers](#identifiers) that have been assigned to an [entity](#entities) using the label operator `:`. For example:
```
name : u32;
```
defines a label, `name`, which represents a [variable](#variables) of type `u32`.

## Entities
In amu, elements of the language that may be assigned a [label](#labels) are collectively referred to as 'entities'. 

## Keywords
amu attempts to reserve as few keywords as possible, prefering to instead use [directives](#directives). Keywords reserved globally are:
```
struct, module, enum
defer
break, continue
return
switch, case 
using
if, else
u8, u16, u32, u64
s8, s16, s32, s64
f32, f64
void
``` 
These words may not be used in any case other than what they are defined for.

## Types
In amu, the word **type** refers to the structure of data that a [label](#labels) takes on. 

## Literals
amu supports string, float, integer, and array literals. 

### Integers
Integer literals may be given in decimal or hexadecimal. Decimal literals are sequences of ascii numerical digits, and hexadecimal literals are sequences of digits and ascii characters a-f (case insensitive) prefixed by the characters '0x'. Integer literals may split digits by using the character `_`, similar to the use of `,` in written numbers. The `_` is not required to be in any spot, however. For example:
```
// decimal digits:
100
2456
99_99_9 // seen as 99999
65_535

// the same digits, in hexadecimal:
0x64
0x998
0x1869f
0xffff
```
An integer literal is given the type `s64`, which is a 64 bit signed integer. 
> **2023-04-09 14:38:54** <br>
This should eventually be adjustable in build settings

### Floats
Float literals are sequences of digits, including at most one period ( `.` ). Float literals may also use `_` in the same way as [integers](#integers). For example:
```
3.1415
1_000_000.0
```
Float literals may omit `0` if either the integer or fractional part is zero:
```
.70711 // same as 0.070711
1. // same as 1.0
```
Obviously, just writing '`.`' for 0, is invalid!

Float literals are, by default, typed as `f64`, also known as `double` in C, which is 64 bit floating point. Floats may be suffixed with `f` to tell the compiler that it should be viewed as a 32 bit float:
```
1.0f
1.f
```

### Arrays
amu supports array literals, which means that, unlike C, arrays can be declared in place. Array literals may be implicitly typed, taking on the type of the first given element:
```
[1,2,3,4]; // an implicitly typed array
```
This is equivalent to writing:
```
u64[][1,2,3,4];
```

### Strings
String literals are sequences of zero or more characters enclosed in double quotes, such as `"hello"`. String literals are encoded in UTF-8. 

A string literal becomes a static array whose size is the total number of bytes the string uses.

## Declarations
There are _ different kinds of declarations in amu: variables, functions, structures, modules, and enums.

### Variables
A variable in amu is a region of memory on the stack that takes on some type and may change throughout the lifetime of the program. The declaration of a variable is done by referencing, in an appropriate scope, a label that refers to a type. For example:
```
u32;
``` 
declares a variable of the builtin type `u32`. In amu, [labels](#labels) are not required for variables to be declared, because labels are distinct from the entity it is assigned to. This means that when a type is referred to where an expression is expected, space on the stack is reserved to make room for that type. However, doing this causes the space reserved for `u32` to never be accessible, which will cause the compiler to emit a warning about the unlabeled memory, and when optimization is enabled, the compiler will ignore creating the memory at all. To avoid this, we give it a label:
```
a: u32;
```
The label `a` now refers to the memory address at which space was made for the type `u32`. This label may be used to reference that memory in expressions which expect it, such as:
```
a + a; 
print(a);
a = 2;
a = a + a;
```
and such.

### Structures
A structure in amu is an ordered collection of [variables](#variables), also called **fields** in this context, that may be used as the type of other variables. Structures may take parameters which determine how a structure is defined. This is refered to as **generics**, and a struct that takes parameters is referred to as a **generic struct**.

A struct definition is of the form:
```
struct{
    field0: u32;
    field1: f32;
    ...
}
```
Just like with variables, a label is not required for a struct declaration, and similarly the compiler will throw a warning about an unaccessible structure definition, and will not even define it internally if optimization is enabled.

## Scopes
Scopes are regions of a program in which [labels](#labels) are valid. Because the same label may be used throughout a program to refer to different entities, scopes are used to help prevent name conflicts. Most scopes in amu are defined by blocks. 

## Directives



## Using
`using` is a special keyword in amu that serves many purposes. Its behavoir is highly dependent on what it is being used on, but general idea behind it is the same. `using` takes an entity on its right and expands its contents into the current scope. Whether this be a 