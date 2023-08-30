/*

    The virtual machine of the compiler which interprets amu's intermediate representation (AIR) bytecode
    as generated by Generator.

*/

#ifndef AMU_MACHINE_H
#define AMU_MACHINE_H

namespace amu {

struct Machine {
    // the arbitrary memory used by the machine

};


namespace machine {

void
run(Code* code);

} // namespace machine 

} // namespace amu

#endif // AMU_MACHINE_H