# CPPDATASTREAM:
Simple repo exploring the performance of a general-use, block-based processing pipeline.

## Usage

### How to use in your project? 
* It's header only, so you could simply copy and paste header files
* Recommended - CMake FetchContent
    ```cmake
    ...
    FetchContent_Declare(
      cppdatastream
      GIT_REPOSITORY https://github.com/SeeRich/cppdatastream.git
      GIT_TAG        v0.2.0
    )
    FetchContent_MakeAvailable(cppdatastream)
    ...
    target_link_libraries(target_name PUBLIC cppdatastream)
    ```

### Code examples
* See [examples](src/examples/)

## DEVELOPERS:

### Prerequisites:
* Install just (Justfile)
* Install docker (for linux/mac testing)

### Linux/Mac development uses docker:
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