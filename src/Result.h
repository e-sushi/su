/*
    Tagged union to be returned from functions which may return an error

    This is just an idea and may not be used, as it may be more trouble than it's worth
*/

#include "kigu/common.h"

namespace amu {

template<typename R, typename E>
struct Result {
    enum {
        Ok,
        Error,
    };

    Type type;
    union{
        R result;
        E error;
    };

    b32 is_ok() { return type == Ok; }
    b32 is_error() { return type == Error; } 
    
    // returns the result if it is valid, stops the program otherwise
    R& unwrap() {
        if(type == Ok) {
            return result;
        } else {
            Assert(0);
        }
    }

    // for implicit conversions
    // useful when we are returning this, eg. if the return type is Result<b32, u32>
    // saying return (b32)1; will automatically construct an Ok Result
    // and saying return (u32)1; will do the opposite
    Result(R result) : type(Ok) { this.result = result; }
    Result(E error) : type(Error) { this.error = error; }

    operator bool() const { return type == Ok; }
};
}