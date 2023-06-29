/*

    The virtual machine of the compiler

    This performs any form of work related to evaluating something at compile time

*/

#ifndef AMU_MACHINE_H
#define AMU_MACHINE_H

#include "Arena.h"

namespace amu {

struct Machine {
    // the arbitrary memory used by the machine
    Arena<u8> memory;

    


};

} // namespace amu

#endif // AMU_MACHINE_H