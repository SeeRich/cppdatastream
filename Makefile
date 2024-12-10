# NOTE: usage of makefile target-specific variables: https://stackoverflow.com/questions/26382846/change-a-make-variable-and-call-another-rule-from-a-recipe-in-same-makefile

default: build_gcc

conan_deps: FORCE
# We build all the dependencies in Release but we "tell" the consumer it's a different build type
	conan install . --output-folder=deps --build missing \
	--profile:build=conan/profiles/$(CONAN_PROFILE) \
	--profile:host=conan/profiles/$(CONAN_PROFILE) \
	-s "&:build_type=$(CONAN_CONSUMER_BUILD_TYPE)" -s :build_type=Release

cmake_config: FORCE
	cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS=$(CXX_FLAGS) \
		-DCMAKE_CXX_STANDARD=20 \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_EXAMPLES=ON

cmake_build: FORCE
	cmake --build build

cmake_install: FORCE
	cmake --install build

build_gcc: CONAN_CONSUMER_BUILD_TYPE=RelWithDebInfo
build_gcc: CONAN_PROFILE=gcc
build_gcc: CC=gcc-13
build_gcc: CXX=g++-13
build_gcc: CXX_FLAGS=
build_gcc: BUILD_TYPE=RelWithDebInfo
build_gcc: clean conan_deps cmake_config cmake_build cmake_install

build_clang: CONAN_CONSUMER_BUILD_TYPE=RelWithDebInfo
build_clang: CONAN_PROFILE=clang
build_clang: CC=clang-19
build_clang: CXX=clang++-19
build_clang: CXX_FLAGS="-stdlib=libc++"
build_clang: BUILD_TYPE=RelWithDebInfo
build_clang: clean conan_deps cmake_config cmake_build cmake_install

# Build with gcc and gprof support
build_gcc_profile: CXX_FLAGS=-pg
build_gcc_profile: CONAN_CONSUMER_BUILD_TYPE=RelWithDebInfo
build_gcc_profile: CONAN_PROFILE=gcc
build_gcc_profile: CC=gcc-13
build_gcc_profile: CXX=g++-13
build_gcc_profile: BUILD_TYPE=RelWithDebInfo
build_gcc_profile: clean conan_deps cmake_config cmake_build cmake_install

format: FORCE
	 cmake --build build --target format_code

lint: FORCE
	run-clang-tidy -p build -checks=-*,clang-analyzer-*,-clang-analyzer-osx* -quiet

test: FORCE
	cd tests && make cmake-fetch-content-consumer

clean:
	@rm -rf install
	@rm -rf build
	@rm -rf deps

FORCE: