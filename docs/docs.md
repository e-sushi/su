# amu

### Table of Contents
1. [Introduction](#introduction)
1. [Labels](#labels)
    1. [Identifiers](#identifiers)
1. [Entities](#entities)
    1. [Expressions](#expressions)
        1. [Literals](#literals)
            1. [Integers](#integers)
            1. [Floats](#floats)
            1. [Arrays](#arrays)
            1. [Strings](#strings)
            1. [Tuples](#tuples)
        1. [Blocks](#blocks)
        1. [If and else](#if-and-else)
        1. [Loop](#loop)
        1. [Switch](#switch)
    1. [Variables](#variables)
    1. [Types](#types)
        1. [Native types](#native-types)
        1. [Arrays](#arrays-1)
            1. [Static arrays](#static-arrays)
            1. [Dynamic arrays](#dynamic-arrays)
            1. [Implicit conversion to pointer](#implicit-conversion-to-pointer)
        1. [Tuple types](#tuple-types)
        1. [Structured types](#structured-types)
        1. [Variant types](#variant-types)
        1. [Traits](#traits)
            1. [Builtin traits](#builtin-traits)
                1. [ImplicitConversion](#implicitconversion)
1. [Runtime vs Compile time](#runtime-vs-compile-time)
1. [Tuples](#tuples-1)
    1. [Valued Tuples](#valued-tuples)
        1. [Structure initializer](#structure-initializer)
        


## Introduction
This document covers all features of the language of amu as well as the standard library that ships with it. The style of this document is somewhat non-linear. You are encouraged to click around to go to different sections that are linked. This may change in the future when it becomes more clear how this document should be laid out.

## Labels
A label is an identifier that points to some [entity](#entities). A label declaration is always of the form
```
<id> : 
```
that is, an identifier followed by a colon. What follows must be some form of [expression](#expressions).

### Identifiers
The identifier of a label is restricted to a subset of unicode characters and must not start with a number. These characters are: all alphanumeric ascii characters, the underscore, and, excluding whitespace, all other characters outside of the range of ascii. For example, the following are all valid identifiers to use as labels: 
```
a
b
Apple
My_Apple123
こんにちは６
```
but these are not:
```
1apple
my variable
/my*fancy/name
```

## Entities
An entity is anything that a [label](#labels) may point to. This includes [variables](#variables), [functions](#functions), [types](#types), [modules](#modules), [expressions](#expressions), and [type references](#meta). 

What kind of entity may be given to a label depends on the context in which the label is being created. 

### Expressions
An expression is an [entity](#entities) that returns a [typed](#types) value. Amu is an expression-oriented language, which means that nearly everything you write will be classified as an expression. 

#### Literals
The most basic expressions are the literals.

##### Integers 
Integer literals are contiguous numbers that may be separated by underscores, for example: 
```
123
1_000
1_000_000
1_0_0_0_0_0_0
```
are all valid integer literals. The default type of an integer literal is [s64](#native-types).

##### Floats
Float literals represent the real numbers and may also be separated by underscores, for example:
```
1.0
1.5
1_000.42
1.234_567
```
are all valid float literals. The default type of a float is [f64](#native-types).

##### Arrays
Amu supports implicitly typed [array](#arrays-1) literals in a manner similar to Python. For example:
```
a := [1,2,3];
```
would create an array of type `s64[3]`. The first element of the literal determines the type of the array, and so all following elements must either be of the same type, or be [implicitly convertable](#implicitconversion) to the type of the first element.

##### Strings
String literals are [static arrays](#static-arrays) of [u8](#native-types) and conform to utf-8. 
```
a := "hello";
print(a$.type); // u8[5]
```
They are not null-terminated as in C, since they are represented by amu's builtin array structure, which keeps a count.

TODO(sushi) discuss dynamic string literals once we have an idea of how they'll work

##### Tuples
[Tuple](#tuples-1) literals may either result in a valued tuple or a Tuple type depending on the elements provided. 

#### Blocks
Blocks in amu are expressions, much like in rust and other functional languages. This means blocks can return values. Blocks allow [statements](#statements) and are scoped. For example: 
```
main :: () -> s32 {
    a := 1;
    {
        b := 2;
    }
    // error: unknown identifier 'b'
    c := a + b;
}
```
Declarations in blocks shadow declarations made outside of it:
```
main :: () -> s32 {
    a := 1;
    {
        a := 2;
        print(a); // 2
    }
    print(a); // 1
}
```
Blocks return the value of the last expression when it is not followed by a semicolon:
```
a := {
    a := 1;
    b := 2;
    a + b
}
print(a); // 3
```
if the last expression does have a semicolon, the block returns void:
```
// error: cannot create a variable of type 'void'
a := {
    a := 1;
    b := 2;
    a + b;
}
```

#### If and else
The conditional `if` and `else` is an expression in amu. The condition of an if expression must include parenthesis:
```
if (1) {
    // do something
}
```
Like [blocks](#blocks), `if` expressions return a value if the last expression of its block is not terminated by a semicolon. `If`/`else` chains return the value of whatever block is chosen:
```
print(if(0) {
    "first"
} else {
    "second"
}); // second
```
Unlike rust, the block braces are not required due to the if condition requiring parenthesis, the previous example can be equivalently written: 
```
print(if(0) "first" else "second");
```
Each block in an if/else chain has to return the same [type](#types) of expression, or they must be implictly convertable to the same type:
```
// error: mismatched if/else returns 
a := if(0) {
    1
} else {
    "hello"
}
```

#### Loop
A loop expression is an infinite loop:
```
loop {
    print("hello");
}
```
results in:
```
hello
hello
hello
hello
hello
hello
hello
...
```
forever. Loops may be stopped by using the `break` expression within them:
```
loop {
    print("hello");
    break;
}
```
results in: 
```
hello
```

TODO(sushi) figure out what loops return, if anything

#### Switch
A switch expression in amu branches based on the result of an expression to some 'arm':
```
a := 1;
switch(a) {
    0 => print("0");
    1 => print("1");
    2 => print("2");
}
```
results in:
```
1
```
Switches return the last unterminated expression used in the chosen arm:
```
a := 1;
b := switch(a) {
    0 => 1,
    1 => 2,
    2 => 3,
}
print(b); // 2
```

### Variables
> Support: <html><span style="color: cyan">Parsed and typed</span></html>

A variable is a [typed](#types) location in stack memory. By default, a variable is mutable. The syntax for declaring a variable whose memory is stored at runtime is:
```
<id> ":" [ <type> ] [ "=" <expr> ]
``` 
and for one whose memory is stored at compile time:
```
<id> ":" [ <type> ] ":" <expr>
```
For details on the difference see: [Runtime vs Compile time](#runtime-vs-compile-time). Variables may be implicitly typed when initialized without a type and with an expression:
```
a := 1;
print(a$.type); // s64
```
And this can propagate: 
```
a : u32 = 1;
b : u32 = 2;
c := a + b;
d := c;
print(d$.type); // u32
```

## Types
Everything in amu has a type, even [entities](#entities) have a [metatype](#meta).

### Native types
Native types represent things that are representable on the hardware. Amu defines several builtin types to represent them: 
| typename | description |
| - | - |
| u8  | 8 bit unsigned integer  |
| u16 | 16 bit unsigned integer |
| u32 | 32 bit unsigned integer |
| u64 | 64 bit unsigned integer |
| s8  | 8 bit signed integer  |
| s16 | 16 bit signed integer |
| s32 | 32 bit signed integer |
| s64 | 64 bit signed integer |
| f32 | 32 bit float |
| f64 | 64 bit float |

### Arrays 
Amu supports builtin typed static arrays, and dynamic arrays. Something important to note is that while the language of amu supports syntax for creating these arrays, there's no builtin implementation for manipulating them. This functionality is either found in the [standard array library](#arrays-2), some other library, or implemented by the user themself.

#### Static arrays 
Static arrays store a [pointer](#pointers) to the start of the array in memory, and a [u64](#native-types) count of the amount of elements in the array. The syntax for declaring an array with a specific size is:
```
<type> [ <int> ] 
```
for example: 
```
a: u32[2];
```
is a static array of 2 u32s. An array supports indexing using integers, negative integers to index in reverse, and [ranges](#ranges) to create a slice of the array. 
```
a: u32[5] = [1, 2, 3, 4, 5];

print(a[0]);    // 1
print(a[1]);    // 2
print(a[-1]);   // 5
print(a[1..3]); // [2, 3]
```
To access the number of elements in the array: 
```
a.count; // 2
```
To access the data pointer directly:
```
a.ptr; // pointer to a location on the stack
```
When an integer is provided, the array is stored on the stack, much like a C array. 

An array type may be declared without an integer to indicate to the compiler that the count is unknown and will be set later. Unlike providing an integer, this no longer makes room on the stack for the array, unless an array literal is given to initialize it. This is useful when you want to explicitly specify the type of an array literal and infer the size from that array literal:
```
a: u32[] = [1,2,3,4];
print(a$.type); // u32[4]
```
This also allows you to take in 'views' of arrays as arguments to functions, for example: 
```
func :: (arr: u32[]) -> void {
    print(arr.count);
}

main :: () -> s32 {
    a: u32[] = [1,2];
    func(a); // 2
    
    a: u32[10];
    func(a); // 10

    func([1,2,3,4]); // 4
}
```

#### Dynamic arrays
Dynamic arrays store a [pointer](#pointers) to the start of the array in memory, a [u64](#native-types) count of the amount of elements in the array, and a [u64](#native-types) count of the space allocated for the array, in units of the size of the underlying type. The syntax for declaring a dynamic array is: 
```
<type> [ .. ]
```
for example: 
```
a: u32[..];
```
Keep in mind, this is only syntactic sugar for declaring a variable of a type whose structure is:
```
struct(?T) {
    ptr: T*; 
    count: u64;
    space: u64;
}
```
No functionality comes with the base language. The compiler will not allocate space on the heap for you in any way. This functionality may be found in the [standard array library](#array):
```
#import array;

main :: () -> s32 {
    a := u32[..].create();
    a.push(1);
    a.push(2);
    print(a); // d[1, 2]
}
```

#### Implicit conversion to pointer
For convenience, arrays support implicitly converting to their underlying pointer: 
```
func :: (a: u32*) -> void {...}

main :: () -> s32 {
    a: u32[] = [1,2,3];
    
    func(a);
    // equivalent to
    func(a.ptr);
}
```

### Tuple types
A tuple type is a tuple representing a collection of types. It is declared using [tuple literal](#tuples) syntax, but only uses types as elements. For example: 
```
(u32, f32, u8);
```
is a tuple type representing the types [u32](#native-types), [f32](#native-types), and [u8](#native-types) in the order that they are declared. This represents a contiguous memory region whose size is the sum of the sizes of the underlying types. In this example, the size of the type would be `9` bytes. Elements of the tuple may be accessed in the same way elements are accessed with [arrays](#arrays-1). A full example:
```
a: (u32, f32, u8);
a[0] = 65565;
a[1] = 1.2;
a[2] = 235;
print(a); // (65565, 1.2, 235)
```
Tuples sizes are static. A tuple type may be inferred from a tuple literal:
```
a := (1, 2, "hello");
print(a$.type); // (s64, s64, u8[5]) 
```
Tuple elements may be named, and in the context of a tuple type, allows you to access the elements by name: 
```
a: (x: f32, y: f32, z: f32);
a.x = 1.0;
a.y = 50.4;
a.z = 1095.2;
```
In the context of declaring tuple types, one element being named means *all* elements must be named: 
```
// error: element 4 of tuple type must be named
a: (x: f32, y: f32, z: f32, s32);
```
Elements of a valued tuple literal may also be named, which resolves into a tuple type whose elements have the same names: 
```
a := (x: 1.2, y: 1.3, z: 1.4);
print(a.y); // 1.3
```
In this context, all elements must be named. In the context of [initializers](#initializers), this is not true.

### Structured types
A structured type in amu is a type representing a contiguous region of memory with labels indexing that memory. It is a special kind of tuple type. A structure is declared using the syntax: 
```
struct {
    <id> ":" <type> [ "=" <exp> ] ";" 
    ...
}
```
for example: 
```
Apple :: struct {
    is_delicious: u32;
    is_rotten: u32;
}
```
In this form, the structure `Apple` is exactly equivalent to the named tuple type `(is_delicious: u32, is_rotten: u32)`. This declares the structured type `Apple`, which may now be used to declare [variables](#variables): 
```
a: Apple;
```
Accessing members of `a`:
```
a.is_delicious = 1;
a.is_rotten = 0;
print(a.is_delicious); // 1
print(a.is_rotten); // 0
```
A structured type may be initialized using valued tuples, as explained [here](#structure-initializer).

Structured types may also be [parameterized](#parameterization), generating a template that when instantiated generates other structured types. For example, if we don't like the C pointer syntax amu provides, we can define a `Ptr` structure: 
```
Ptr :: struct(T:$type) {
    p: T*;
}
```
This structure must then be instantiated when it is used: 
```
// error: must provide argument 'T' of structure 'Ptr'
a: Ptr; 

b: Ptr(u32);
```
`b` is now represented by a generated structure that would look like this:
```
struct {
    p: u32*;
}
```
See the [parameterization](#parameterization) section for more details on this subject.

### Variant types
A variant type is a collection of things that a single type may assume the form of. It is usually known as a 'tagged union' or a 'sum type'. The syntax for declaring a variant type is:
```
variant {
    <id> [ <tuple-type> ]
    ...
}
```
Rust users should be used to this syntax from rust's version of enums. A variant may simply be a collection of tags that a single type may take on:
```
Fruit :: variant {
    Apple,
    Orange,
    Banana,
}
```
An expression of a variant type may be [switched](#switch) on to match the underlying type, much like in rust:
```
a: Fruit;

switch(a) {
    Apple => {...},
    Orange => {...},
    Banana => {...}
}
```
Similarly to rust, switching on a variant typed expression requires you to handle all variations of that type. The wildcard identifier may be used to handle all variants unhandled in the rest of the switch: 
```
switch(a) {
    Apple => {...},
    _ => {...}
}
```

Variants may define data that each variant carries with it. Stealing rust's IP example: 
```
Ip :: variant {
    v4(u8,u8,u8,u8),
    v6(u8[..])
}
```

TODO(sushi) show how this is used once I decide how this data is retrieved 


### Traits
Amu's type system supports traits, which are specifications of functionality that a [type](#types) is expected to implement when the trait is applied to it. A trait is defined using the syntax: 
```
trait {
    <func-signature>
}
```
Trait declarations have an implicit [parameter](#parameterization) `Self`, which represents the identifier of the [type](#types) that the trait is being implemented on. If I wanted to define the trait `Hashable`, I would write something like:
```
Hashable :: trait {
    hash :: (Self*) -> u64;
}
```
The trait defines the function signature `hash :: (Self*) -> u64`, which, which the trait is applied to a type, is expected to be implemented. Traits are applied to types using the syntax: 
```
<type> impls <trait>;
```
For example:
```
Apple :: struct {
    is_delicious: u32;
    is_rotten: u32;
}

Apple impls Hashable;
```
`Apple` now has the trait `Hashable`, but we have to implement the functions that `Hashable` wants:
```
hash :: (x: Apple*) -> u64 {
    x.is_delicious + x.is_rotten
}
```
This isn't a very good hash, but the compiler requires you to implement the function because the `Hashable` trait was applied to `Apple`. This is useful for restricting templated types when using [parameterization](#parameterization). For example, if we have the templated [function](#functions):
```
func :: (a:?T) -> void {}
```
but we want to make sure that `T` is `Hashable`. We can tell the compiler that `T` needs to include that trait:
```
func :: (a:?T but Hashable) -> void {}
```
Now, when we call `func`, the type we call it with has to implement the trait `Hashable`:
```
main :: () -> s32 {
    a: Apple;
    b: u32 = 1;
    // ok, Apple implements Hashable
    func(a);
    // error: type 'u32' does not implement the trait Hashable
    func(b);
}
```

#### Builtin traits
Amu handles much of special user-defined functionality through the use of traits, and so there are a collection of traits builtin to handle various behavoirs. Most of these traits require you to define function(s) that the compiler will implicitly invoke in certain situations.

##### ImplicitConversion
`ImplicitConversion` instructs the compiler that one type may be implicitly converted into another through the use of a conversion function. Its definition would look something like:
```
ImplicitConversion :: trait(?To) {
    convert :: (Self*) -> To;
}
```
This allows the compiler to coerce types into other types automatically, for example, the following code would normally throw an error:
```
MyType :: struct {...}
func :: (a: u32) -> void {...}

main :: () -> s32 {
    a: MyType;
    // error: cannot convert from MyType to u32
    func(a);
}

```
but if you implement `ImplicitConversion` on `MyType`, the error goes away:
```
MyType impls ImplicitConversion(u32);
convert :: (x: MyType*) -> u32 {...}
```
Now, when you say `func(a)`, the compiler will first call `convert` on a, then pass the result to `func`. 

### Pointers

## Tuples
Parts of tuples have been explained above, but this section will go into more detail about them as a whole. A tuple is any contiguous set of things and are either [type tuples](#tuple-types) or valued tuples. Type tuples represent a [type](#types) consisting of an ordered set of subtypes and more information about them may be found in that section. Valued tuples are contiguous regions of memory that store a collection of values. 

The kind of tuple you may use, and even rules on how you may use that kind of tuple, depends on the context in which you use them. The syntax for declaring any kind of tuple is a list of elements bounded by parenthesis:
```
(<elem>, <elem>, ...)
```
What these elements are determines what kind of tuple is being created. When all of the elements are references to types, and the tuple does not immediately follow a [function](#functions) identifier, a [type tuple](#tuple-types) is created. More information about the behavoir of type tuples is found in their section. When a value other than a type is used as *any* element, then the tuple is a valued tuple. Valued tuples have an underlying tuple type, which is generated by the values used in the tuple. 

### Valued tuples
Valued tuples may be used in different contexts and so following is a collection of those contexts and the rules on using a valued tuple in them.

#### Structure initializer
> Support: <html><span style="color:red">Nothing</span></html>

A valued tuple may be used as an 'initializer' for a type where that type is expected. Say we have the [structured type](#structured-types):
```
Player :: struct {
    health: u32;
    mana: u32;

    is_poisoned: u32;
    is_bleeding: u32;
}
```
A variable declaration of this type could be initialized using a valued tuple:
```
player: Player = (100, 50, 0, 0);
```
You don't have to initialize the entire struct:
```
player: Player = (100, 50);
```
Uninitialized members are filled with 0. The elements of the valued tuple may also be named, allowing you to selectively fill parts of the struct as well as fill it out of order. You may also fill the struct positionally before naming members, much like you can with function calls:
```
player: Player = (mana: 50, health: 100);
player: Player = (100, 50, is_bleeding: 0);
```
However, once you use a name, the following elements must be named as well:
```
// error: positional elements may not come after named elements
player: Player = (mana: 50, 100);
```
This works in a nested fasion too. Say we have this:
```
Player :: struct {
    health: u32;
    mana: u32;

    status: StatusFlags;
}

StatusFlags :: struct {
    is_poisoned: u32;
    is_bleeding: u32;
}
```
The player struct can be initialized in the following ways:
```
player: Player = (100, 50, (0,0));
player: Player = (mana: 50, status: (0,1));
player: Player = (
    status: (
        is_bleeding: 0, 
        is_poisoned: 1
    )
);
```
In order to keep initialization clear, however, this is not permitted to work: 
```
// error: cannot convert s64 to StatusFlags
// error: too many elements in valued tuple initializer of Player
player: Player = (100, 50, 0, 0);
```
This is not limited to variable initialization, a valued tuple may be used in any context where a value is expected:
```
kill_player :: (p: Player) -> void {...}

main :: () -> s32 {
    kill_player((100, 50));
}
```

## Runtime vs Compile time
Amu is capable of arbitrary code execution at compile time and tries to make a clear distinction between code that is performed while compiling and code that will be performed at runtime. This distinction is primarily made through the use of the colon '`:`'. For example, [labels](#labels) have to be created and resolved at compile time, and so they use the colon in their declaration syntax: 
```
<id> ":"
```
This declares a symbol whose name is `<id>`, which is saved in the compiler's memory. The other major use of colons is when defining a compile time [expression](#expressions). When an expression follows a colon (that does not belong to a label) that expression is interpretted as one that needs to be ran at compile time. For example: 
```
: 1 + 1;
```
When the compiler comes across this, it will evaluate the expression and emplace the resulting value where the expression was used. This underlines how the majority of [entities](#entities) are declared in amu:
```
func :: () -> s32 {...}
Apple :: struct {...}
math :: module {...}
```
all of these can actually be written in the following way:
```
func: :()->s32{...}
Apple: :struct {...}
math: :module {...}
```
what you are saying with this syntax is that the label belongs to the compile time expression that results in a [function](#functions), [struct type](#structured-types), and [module](#modules). This is how constant [variables](#variables) are handled in amu: 
```
a :: 1;
```
`a` is a variable that is stored in compiler memory, it does not exist to the executable that will be produced. When `a` is used in other expressions, its value is emplaced as if it were a literal. Even though `a` is constant to the executable produced, it's not actually constant at compile time: 
```
main :: () -> s32 {
    :{
        a = 2;
    }
}
```
The [block](#blocks) after the colon is evaluated at compile time, when the symbol `a` still exists, thus its memory is still mutable. 
