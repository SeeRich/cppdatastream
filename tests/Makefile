cmake-fetch-content-consumer: FORCE
	cd cmake-fetch-content-consumer && \
	cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_COMPILER=g++-13 \
		-DCMAKE_CXX_STANDARD=20 && \
	cmake --build build

FORCE: