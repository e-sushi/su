#include "Token.h"
#include "Code.h"
#include "Source.h"

namespace amu {

void
to_string(DString& start, Token* t) {
	start.append("Token<", t->code->source->name, ":", t->l0, ":", t->c0, ": '", t->raw, "'>");
}

void
to_string(DString& start, Token& t) {
	return to_string(start, &t);
}

}
