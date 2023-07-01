/*
    Tagged union to be returned from functions which may return an error

    This is just an idea and may not be used, as it may be more trouble than it's worth
*/

#ifndef AMU_RESULT_H
#define AMU_RESULT_H

#include "kigu/common.h"

namespace amu {

template<typename R, typename E>
struct Result {
    b32 is_ok;
    union{
        R result;
        E error;
    };

    // returns the result if it is valid, stops the program otherwise
    R& unwrap() {
        if(is_ok) {
            return result;
        } else {
            Assert(0);
        }
    }

    // for implicit conversions
    // useful when we are returning this, eg. if the return type is Result<b32, u32>
    // saying return (b32)1; will automatically construct an Ok Result
    // and saying return (u32)1; will do the opposite
    Result(R result) : is_ok(true) { this->result = result; }
    Result(E error) : is_ok(false) { this->error = error; }

    operator bool() const { return is_ok; }
};

} // namespace amu

#endif // AMU_RESULT_H