#!/bin/sh -e

test_on="

gcc
gcc-3.4.6
gcc-4.1.2
gcc-4.2.3

x86_64-pc-linux-gnu-gcc
arm-vfp-linux-gnueabi-gcc
mingw32-gcc
"

for compiler in $test_on
do
    if $compiler -v 2>/dev/null
      then
        build_dir="$(pwd)/.build-$compiler"
        build_log="$build_dir/build_log"

        mkdir -p "$build_dir"
        make -C .. CC="$compiler" O="$build_dir" $@ > "$build_log" 2>&1 &&
        echo "$compiler: PASS" ||
        echo "$compiler: FAIL"
      else
        echo "$compiler: SKIP"
    fi
done
