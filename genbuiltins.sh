#!/bin/bash

n=$'\n'
decl=""
def=""

for file in builtins/*.cpp; do
    while read line; do
        if [[ $line =~ ^BUILTIN\(([^,]*?),\ *([^,]*?), ]]; then
            decl="${decl}${n}extern NativeInfo ${BASH_REMATCH[2]}_native_info;"
            def="${def}${n}    c.define_native(${BASH_REMATCH[1]}, &${BASH_REMATCH[2]}_native_info);"
        elif [[ $line =~ ^SYNTAX\((.*?),\ *(.*?)\) ]]; then
            decl="${decl}${n}Value ${BASH_REMATCH[2]}(Context &, Value, Value, bool);"
            def="${def}${n}    c.define_syntax(${BASH_REMATCH[1]}, ${BASH_REMATCH[2]});"
        fi
    done < $file
done


echo "#include \"../pars.hpp\"

namespace pars { namespace builtins {
$decl

void define_all(Context &c) {$def
}

} }"
