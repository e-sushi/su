#pragma once
#ifndef SU_PARSER_H
#define SU_PARSER_H

#include "su-lexer.h"
#include "su-types.h"

namespace suParser {
	b32 parse(Program& mother);
}

#endif //SU_PARSER_H