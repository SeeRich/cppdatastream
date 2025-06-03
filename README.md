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

* Trigger a rebuild (not a clean build) using: `make cmake_build`

### DEVELOPERS:

### Windows (amd64) build:
```shell
python -m pip install -U conan ninja
conan install . --output-folder=deps --build missing -s :build_type=Release
deps/conanbuild.bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=y
cmake --build build
```

### Using GPROF:
```bash
# Build with GPROF data
make build_gcc_profile
# Run the program of interest (simple example)
./build/src/examples/ex_simple
# Convert
gprof ./build/src/examples/ex_simple > ex_simple.txt
```