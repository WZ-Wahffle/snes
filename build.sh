#!/bin/sh

function build() {
    mkdir -p out
    # g++ -O3 -c -o src/ui.o src/ui.cpp -Isrc/include/ -Wall -Wextra -Werror -Wno-unused-function
	gcc -g -o out/snes src/*.c -Isrc/include/ -Lsrc/lib/ -l:libraylib.a -l:libSDL2.a -lm -lrlImGui -limgui -Wall -Wextra -Werror -lstdc++ -DLOG_LEVEL=0
}

function build_raylib() {
    mkdir -p src
    mkdir -p src/include
	mkdir -p src/lib
	make -s PLATFORM=PLATFORM_DESKTOP_SDL -C raylib/src/
	cp raylib/src/libraylib.a src/lib/libraylib.a
	cp raylib/src/raylib.h src/include/raylib.h
}

function build_rlimgui() {
    mkdir -p src
    mkdir -p src/include
	mkdir -p src/lib
	cp rlImGui/imgui_impl_raylib.h src/include/imgui_impl_raylib.h
	cp rlImGui/rlImGui.h src/include/rlImGui.h
	cd rlImGui
    g++ -c -o rlImGui.o rlImGui.cpp
	ar rcs librlImGui.a rlImGui.o
	cd ..
	cp rlImGui/librlImGui.a src/lib/librlImGui.a
}

function build_imgui() {
    mkdir -p src
    mkdir -p src/include
	mkdir -p src/lib
	cd imgui
	g++ -c *.cpp -DNO_FONT_AWESOME
	ar rcs libimgui.a *.o
	cd ..
	cp imgui/libimgui.a src/lib/libimgui.a
	cp imgui/libimgui.a rlImGui/libimgui.a
	cp imgui/imgui.h src/include/imgui.h
	cp imgui/imgui.h rlImGui/imgui.h
	cp imgui/imconfig.h src/include/imconfig.h
	cp imgui/imconfig.h rlImGui/imconfig.h
}

function build_sdl() {
    mkdir -p src
    mkdir -p src/include
	mkdir -p src/lib
	mkdir -p tmp
	cd tmp
	cmake -S ../SDL/ -B .
	make
	cd ..
	cp tmp/libSDL2.a src/lib/libSDL2.a
	mkdir -p raylib/src/external/SDL2
	mkdir -p raylib/src/external/SDL2/include
	mkdir -p raylib/src/external/SDL2/lib
	cp tmp/libSDL2.a raylib/src/external/SDL2/lib
	cp tmp/include/SDL2/* raylib/src/external/SDL2/include
	cp tmp/include-config-/SDL2/* raylib/src/external/SDL2/include
	rm -rf tmp
}

if [ "$1" = "raylib" ] ; then
    build_raylib
elif [ "$1" = "rlimgui" ] ; then
    build_rlimgui
elif [ "$1" = "imgui" ] ; then
    build_imgui
elif [ "$1" = "sdl" ] ; then
    build_sdl
else
    if [ ! -f src/lib/libSDL2.a ]; then
        echo "building sdl2"
        build_sdl
    fi
    if [ ! -f src/include/raylib.h ]; then
        echo "building raylib"
        build_raylib
    fi
    if [ ! -f src/include/imgui.h ]; then
        echo "building imgui"
        build_imgui
    fi
    if [ ! -f src/include/rlImGui.h ]; then
        echo "building rlimgui"
        build_rlimgui
    fi
    echo "building"
    build
    echo "finished"
fi
