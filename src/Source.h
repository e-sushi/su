/*
    Internal representation of a file that is to be compiled
    This is called 'Source' to avoid conflicting with deshi's File.

    All files are compiled into modules, this just holds information relating directly to the file
*/

#include "Entity.h"
#include "core/file.h"

namespace amu {

struct Source {
    File* file;

    Entity::Module* module;
};

}