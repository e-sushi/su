#pragma once
#ifndef SU_ASSEMBLER_H
#define SU_ASSEMBLER_H

#include "su-parser.h"

namespace suAssembler {
	b32 assemble(Program& program, string& assembly);
}


#endif //SU_ASSEMBLER_H