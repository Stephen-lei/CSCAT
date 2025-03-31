extract:
	clang++ `llvm-config --cxxflags --ldflags` -lclang-cpp FindClassDecls.cpp data_encode.cpp -g -o ./build/clang-extract
	cp ./build/clang-extract /home/ubuntu/install/bin

cmp:
	clang++ `llvm-config --cxxflags --ldflags` json_cmp.cpp data_encode.cpp -g -o ./build/declcmp

all:
	make extract cmp