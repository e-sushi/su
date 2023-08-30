/*

    The virtual machine of the compiler which interprets amu's intermediate representation (AIR) bytecode
    as generated by Generator.

*/

#ifndef AMU_MACHINE_H
#define AMU_MACHINE_H

namespace amu {

struct Machine {
    Code* code;
    
    Array<Register> registers; // currently loaded registers


    // ~~~~~~~ interface ~~~~~~~


    static Machine*
    create(Code* code); 

    void
    run();
};

} // namespace amu

#endif // AMU_MACHINE_H