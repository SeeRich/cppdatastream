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
conan profile detect # Only first time (modify profile to your liking ./conan/home/profiles/default)
make deps
make build
```

### DEVELOPERS:

#### TODO:
[ ] Handle MPSC ThreadedBuffer

### Using GPROF:
```bash
# Build with GPROF data
make build_gprof
# Run the program of interest (simple example)
./build/src/examples/ex_simple
# Convert
gprof ./build/src/examples/ex_simple > ex_simple.txt
```