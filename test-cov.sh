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
