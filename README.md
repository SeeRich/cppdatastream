# CPPDATASTREAM:
Simple repo exploring the performance of a general-use, block-based processing pipeline.

### Prerequisites:
* Install just (Justfile)
* Install docker

### Getting setup:
```bash
just docker-build
just docker-run
# Now inside docker container
# Build with GCC (clean build)
make build_gcc
# OR with Clang
make build_clang
```

* Trigger a rebuild using: `make cmake_build`

### DEVELOPERS:

#### TODO:
[ ] Handle MPSC ThreadedBuffer

### Using GPROF:
```bash
# Build with GPROF data
make build_gcc_profile
# Run the program of interest (simple example)
./build/src/examples/ex_simple
# Convert
gprof ./build/src/examples/ex_simple > ex_simple.txt
```