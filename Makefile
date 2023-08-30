SHELL:=/bin/bash

build_:
	mkdir -p build
	CMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold CXX=$$(which c++) cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -GNinja -B build
	cmake --build build

debug:
	make clean
	mkdir -p build
	CMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold CXX=$$(which c++) cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -GNinja -B build
	cmake --build build
	ln -fs $$PWD/build/compile_commands.json $$PWD/compile_commands.json

debug-asan:
	make clean
	mkdir -p build
	#pushd build && ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-12/bin/llvm-symbolizer ASAN_OPTIONS=symbolize=1 CXX=$$(which c++) cmake -DSANITIZER=1 -DDEBUG=on ..
	#pushd build && ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-12/bin/llvm-symbolizer ASAN_OPTIONS=symbolize=1 make VERBOSE=1 -j $$(nproc)
	ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-12/bin/llvm-symbolizer ASAN_OPTIONS=symbolize=1 CMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold CXX=$$(which c++) cmake -DSANITIZER=1 -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -GNinja -B build
	cmake --build build
	ln -fs $$PWD/build/compile_commands.json $$PWD/compile_commands.json

format:
	cmake --build build --target clangformat

clean:
	rm -rf build

get_data_dir:
	automount
	ls -al data_dir
	rsync -raPv /mnt2/NAS/photos/photo_app/data_dir/ data_dir/

