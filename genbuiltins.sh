#!/bin/bash

n=$'\n'
decl=""
def=""

for file in builtins/*.cpp; do
    while read line; do
        if [[ $line =~ ^BUILTIN\((\"[^\"]*?\")\)\ *([a-zA-Z0-9_]+)\ *\(([^\)]+) ]]; then
            sname=${BASH_REMATCH[1]}
            fname=${BASH_REMATCH[2]}
            args=${BASH_REMATCH[3]}

            nreq=0
            nopt=0
            rest=0
            while read a; do
                if [ $a == "rest" ]; then
                    rest=1
                elif [[ $a =~ ^_ ]]; then
                    nopt=$((nopt + 1))
                else
                    nreq=$((nreq + 1))
                fi
            done < <(echo $args | grep -oE ", *Value *[a-zA-Z0-9_]+" | cut -d " " -f 3)

            decl="${decl}${n}Value $fname($args);"
            def="${def}${n}    c.define_native($sname, $nreq, $nopt, $rest, (VoidFunc)$fname);"
        elif [[ $line =~ ^SYNTAX\((\"[^\"]*?\")\)\ *([a-zA-Z0-9_]+) ]]; then
            sname=${BASH_REMATCH[1]}
            fname=${BASH_REMATCH[2]}

            decl="${decl}${n}Value $fname(Context &, Value, Value, bool);"
            def="${def}${n}    c.define_syntax($sname, $fname);"
        fi
    done < $file
done


echo "#include \"../pars.hpp\"

namespace pars { namespace builtins {
$decl

void define_all(Context &c) {$def
}

} }"
