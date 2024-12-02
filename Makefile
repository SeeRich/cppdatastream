default: deps config build

deps: FORCE
	conan install . --output-folder=deps --build missing

config: FORCE
	@cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_EXAMPLES=ON

build: FORCE
	@cmake --build build

install:
	@echo "TODO..."

build_debug: FORCE
	conan install . --output-folder=deps --build missing -s "&:build_type=Debug" -s :build_type=Release
	@cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_EXAMPLES=ON
	@cmake --build build

build_gprof: FORCE
	conan install . --output-folder=deps --build missing -s "&:build_type=RelWithDebInfo" -s :build_type=Release
	@cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DCMAKE_CXX_FLAGS=-pg \
		-DCMAKE_EXE_LINKER_FLAGS=-pg \
		-DCMAKE_SHARED_LINKER_FLAGS=-pg \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_EXAMPLES=ON
	@cmake --build build

clean:
	@rm -rf install
	@rm -rf build
	@rm -rf deps

FORCE: