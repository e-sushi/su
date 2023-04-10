# amu docs

The style of this documentation is inspired by the ISO/IEC 9899:TC3.

The organization of this document is a mix of linear and non-linear. Major categories are ordered such that the major concepts of amu build upon each other, but subcategories do not necessarily follow this trend. For example, the major section [types](#types) begins by introducing what a type generally is in amu, but its subcategories use concepts and syntax introduced later in the document. 

It is separated into two major sections, the first detailing information about the language and the second detailing information about the inner workings of the compiler.

## Table of contents
1. [Language](#language)
    1. [Notation](#notation)
    1. [Identifiers](#identifiers)
    1. [Labels](#labels)
    1. [Entities](#entities)
    1. [Keywords](#keywords)
    1. [Types](#types)
        1. [Integers](#integers)
        1. [Floats](#floats)
        1. [Arrays](#arrays)
        1. [User defined](#user-defined)
    1. [Literals](#literals)
        1. [Integers](#integers-1)
        1. [Floats](#floats)
        1. [Arrays](#arrays)
        1. [Strings](#strings)
    1. [Declarations](#declarations)
        1. [Variables](#variables)
        1. [Functions](#functions)
        1. [Structures](#structures)
    1. [Scopes](#scopes)
    1. [Directives](#directives)
    1. [Using](#using)
    1. [Advanced](#advanced)
1. [Compiler](#compiler)
    1. [Types](#types-1)

## Language

### Notation 
The syntax of amu is defined as a context free grammar in 'BN.txt', in the root of its repo. amu's syntax is inspired by C, jai, odin, and python. 

### Identifiers
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

### Labels
**Labels**, technically known as **name bindings**, are [identifiers](#identifiers) that have been assigned to an [entity](#entities) using the label operator `:`. For example:
```
name : u32;
```
defines a label, `name`, which represents a [variable](#variables) of [type](#types) `u32`.

### Entities
In amu, elements of the language that may be assigned a [label](#labels) are collectively referred to as 'entities'. 

### Keywords
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

### Types
In the language, the word **type** refers to the structure of data that a variable represents. This allows the compiler to ensure that different types of memory are being used correctly, a process known as **type-checking**. When you declare a variable, you are required to explicitly give it a type, or allow the compiler to infer its type based on the value you initialize it with. 

amu comes with builtin types for representing scalar types, as well as static and dynamic arrays of any type.

#### Integers
amu provides eight builtin types representing integers: `u8`, `u16`, `u32`, `u64`, `s8`, `s16`, `s32`, and `s64`. The digit following the character indicates the width of the integer in bits, while the character indicates whether the integer is **s**igned or **u**nsigned.

#### Floats
There are two floating point types in amu, `f32` and `f64`, known in C as `float` and `double` respectively. 

#### Arrays
amu provides two kinds of builtin arrays, static and dynamic. The syntax for arrays, much like in C, is special; typing any type name and following it with one of 3 variations of `[]` declares an array that holds that type. 

Arrays are structures, unlike in C where they are seen as actually being a pointer to the first element. A static array contains a pointer to the first element of the array as well as an integer, `count`, that indicates how many elements there are in the array. A dynamic array contains these two members as well, but also stores a `space` variable, which keeps track of the actual amount of reserved space.

Static arrays are constructed in two ways, either explicitly declaring the size of the array, or allowing the compiler to figure it out based on the array used to initialize it:
```rust 
integers : u32[5]; // an uninitialized array of 5 integers
floats : f32[] = [1.2, 1.3]; // an initialized array of two floats
```
Not giving an array to a variable declared using `[]` results in an error. Dynamic arrays are declared using the syntax `[..]`:
```rust
integers : u32[..]; // uninitialized dynamic array of u32
floats : f32[..] = [2.3, 2.4];
```

Even though amu differenciates between builtin static and dynamic arrays, it does not provide any native methods for using them. The user must either import the standard `array` module, or build their own methods for using these arrays. This is done so that no method for using the builtin array type is enforced, while still supporting the nice syntax of C for declaring them.

amu also supports [array literals](#arrays-1).

#### User defined
User defined types are created by [declaring a structure](#structures), which may then be used as the type of variables. Information about the creation and use of structures is detailed in its section in [Declarations](#declarations).

### Literals
amu supports float, integer, character, array, and string literals. Literals of user defined types are also supported through initializer lists.

#### Integers
Integer literals may be given in decimal or hexadecimal. Decimal literals are sequences of contiguous ascii numerical digits, and hexadecimal literals are sequences of digits and ascii characters a-f (case insensitive) prefixed by the characters '0x'. Integer literals may split digits by using the character `_`, similar to the use of `,` in written numbers. The `_` is not required to be in any spot, however. For example:
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
By default, an integer literal is given the type `s64`, which is a 64 bit signed integer. 
> **2023-04-09 14:38:54** <br>
This should eventually be adjustable in build settings

#### Floats
Float literals are sequences of digits, including at most one period ( `.` ). Float literals may also use `_` in the same way as [integers](#integers). For example:
```rust
3.1415
1_000_000.0
```
Float literals may omit `0` if either the integer or fractional part is zero:
```rust
.70711 // same as 0.070711
1. // same as 1.0
```
Obviously, just writing '`.`' for 0, is invalid!

Float literals are, by default, typed as `f64`, also known as `double` in C, which is 64 bit floating point. Floats may be suffixed with `f` to tell the compiler that it should be viewed as a 32 bit float:
```rust
1.0f
1.f
```

#### Arrays
amu supports array literals, which means that, unlike C, arrays can be declared in place. Array literals may be implicitly typed, taking on the type of the first given element:
```rust
[1,2,3,4]; // an implicitly typed array
```
This is equivalent to writing:
```rust
u64[][1,2,3,4];
// or
u64[4][1,2,3,4];
```



#### Strings
String literals are sequences of zero or more characters enclosed in double quotes, such as `"hello"`. String literals are encoded in UTF-8. 

A string literal becomes a static array whose size is the total number of bytes the string uses.

### Declarations
There are _ different kinds of declarations in amu: variables, functions, structures, modules, and enums.

#### Variables
A variable in amu is a region of memory on the stack that takes on some type and may change throughout the lifetime of the program. The declaration of a variable is done by referencing, in an appropriate scope, a label that refers to a type. For example:
```rust
u32;
``` 
declares a variable of the builtin type `u32`. In amu, [labels](#labels) are not required for variables to be declared, because labels are distinct from the entity it is assigned to. This means that when a type is referred to where an expression is expected, space on the stack is reserved to make room for that type. However, doing this causes the space reserved for `u32` to never be accessible, which will cause the compiler to emit a warning about the unlabeled memory, and when optimization is enabled, the compiler will ignore creating the memory at all. To avoid this, we give it a label:
```rust
a: u32;
```
The label `a` now refers to the memory address at which space was made for the type `u32`. This label may be used to reference that memory in expressions which expect it, such as:
```rust
a + a; 
print(a);
a = 2;
a = a + a;
```
and such.

#### Functions
A function in amu is a 

#### Structures
A structure in amu is an ordered collection of [variables](#variables) that may be used as the [type](#types) of other variables. Structures may take parameters which determine how a structure is defined. This is refered to as **generics**, and a struct that takes parameters is referred to as a **generic struct**.

A struct definition is of the form:
```rust
struct{
    field0: u32;
    field1: f32;
    ...
}
```
Just like with variables, a label is not required for a struct declaration, and similarly the compiler will throw a warning about an unaccessible structure definition, and will not even define it internally if optimization is enabled.

#### Modules
Modules are the main unit of code organization in amu. A module may be seen as functionally the same as a file. Variables, functions, structs, etc. may be declared in a module and are unique just as they are in the global scope of a file. They may be seen as analogous to namespaces in C++. However, modules may take parameters similarly to [structures](#structures). 


### Scopes
Scopes are regions of a program in which [labels](#labels) are valid. Because the same label may be used throughout a program to refer to different entities, scopes are used to help prevent name conflicts. Most scopes in amu are defined by blocks, but other scopes 

### Files
An amu file, (extension `.amu`) is a collection of [declarations](#declarations) that may be [imported](#import) into other files using `#import`. More detailed discussion of `#import` and how it behaves in different contexts is described in its section of [Directives](#directives).

### Directives
Directives are used extensively in amu for purposes in which the user must directly tell the compiler that they want it to do something. They use the same syntax as in C, but are not processed by a preprocessor, rather they are integrated into the language and parsed like anything else. Directives should be seen as the language used to communicate with the compiler.

The syntax of directives is also much more flexible than C, since they are parsed like anything else. Most directives are considered statements and thus follow the same syntactic rules as them, namely, they must be followed by a semicolon, easily allowing multiline directives.

#### import
The `import` directive is used to tell the compiler to import a [file's](#files) declarations into the scope it is used in. This directive behaves like a statement, and can be seen as functionally equivalent to [using](#using). 

### Using
`using` is a special keyword in amu that serves many purposes. Its behavoir is highly dependent on what it is being used on, but general idea behind it is the same. `using` takes an entity on its right and expands its contents into the current scope. 

#### Module Labels
When `using` is used with a label that refers to a module, it will take its declarations and bring them into the current scope.

### Metaprogramming
amu supports metaprogramming through an interface that provides access to all information generted at compile time. The interface is accessed through the use of the meta operator, `$`. This operator allows accessing information about any [entity](#entities) as it would be stored in the compiler.


### Interpretation
The design of amu is based around trying to keep a consistent interpretation behind its syntax possible. This primarily involves ensuring that the use of some syntax in one situation is more or less consistent with how it may be used in other situations. One example of this concept is the design of `using`. `using` may be used in many different contexts, but the general idea behind `using` is that it takes whatever is given on its right hand side and expands whatever it may contain into the current scope, though what exactly it does is dependent on what is given to it.

#### The Separation Between Label and Declaration
The name of an [entity](#entities) in amu takes the same form regardless of what [type of entity](#metatypes) it is being assigned to. This allows us to decouple declarations from their names, which makes it easy to support anonymous entities in the syntax by allowing the user to not provide a label. This primarily addresses the concept of anonymous [functions](#functions), but allows for 

#### Metatypes
A **metatype** is a property of all [entities](#entities). It is called 'meta' to distinguish it from the concept of the [type of a variable](#types). This concept of the language overlaps very strongly with how elements are represented in the compiler, however the language is meant to be able to use this information, and the process of the construction of this information and its use is described in this section.

For example, in the code
```
name : struct{}
```
there are two entities: `name` and `struct{}`. The `name` entity has the metatype `label_structure`, indiciating that it is a label which refers to a structure definition. The `struct {}` entity has the metatype `structure`, which indicates that it's a declaration of a structure. 

But what would you do with this information? Who cares that the syntax has types attached to it, I just want to write code!

Building these concepts helps to form understandings of certain kinds of syntax, as well as developing new forms of syntax that may not have seemed viable without a proper interpretation of what is really going on when you write things. For example, amu supports the syntax:
```rust
a : using struct {
    b : u32;
    c : u32;
}
```
to declare a variable that uses an anonymous struct. This may seem to conflict with the usual syntax for aliasing a structure:
```
Apple : struct {is_rotten:b32;}

my_apple : using Apple;
```
however, using our concept of metatypes, we can say this:
when `using` is used on a `label_structure`, it removes the label from it and returns the entity `struct{is_rotten:b32;}`, which has the metatype `structure`. Just like when we first declared the `Apple` structure, we now have the pattern `<label> ":" <structure-decl>`, which the compiler recognizes as assigning a label to a structure declaration. However, when we use the syntax shown in the previous code, we are using `using` on an entity with the metatype `structure`. Before this system, one might think that this is undefined knowing how `using` generally behaves. For example, if you have a module `Guy` who declares a function `do_thing`, doing `using Guy;` will bring that function into the current scope. This is okay, because the function exists as a 

## Compiler

### Types