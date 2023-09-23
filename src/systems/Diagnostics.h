#ifndef AMU_DIAGNOSTICS_H
#define AMU_DIAGNOSTICS_H

// #include "Messenger.h"

namespace amu {

namespace diagnostic {
#include "data/diagnostics-data.generated"
extern language lang;
}

struct Diagnostic {
    u64 code;
    diagnostic::severity severity;
};
} // namespace amu
  
#include "data/diagnostic-header.generated"

#endif // AMU_DIAGNOSTICS_H
