#/bin/sh

set -e
mkdir -p build/cov
cd build/
cmake .. -DCOVERAGE=ON
make
ctest -T Test -T Coverage --output-on-failure
cd ..
rm -f build/cov/coverage.info
lcov -c -o build/cov/coverage.info -d build/CMakeFiles --exclude=$PWD/test*.c
genhtml build/cov/coverage.info -o build/cov

mkdir -p build-nothreads
cd build-nothreads
cmake .. -DCMAKE_C_FLAGS="-DMYMALLOC_NO_THREADING" 
make
ctest  -T Test --output-on-failure
