#ifndef AMU_DIAGNOSTICS_H
#define AMU_DIAGNOSTICS_H

namespace amu {

namespace diagnostic {
#include "data/diagnostics-data.generated"
extern language lang;
}

struct Diagnostic {
    u64 code;
    diagnostic::severity severity;
};
} // namespace 

#endif // AMU_DIAGNOSTICS_H