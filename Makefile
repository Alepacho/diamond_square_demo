CC = g++ 
STD = --std=c++17
CARGS = -Wall -g

LFLAGS = `sdl2-config --libs`
IFLAGS = `sdl2-config --cflags`

INC_DIR = ./inc/
LIB_DIR = ./lib/
IMGUI_DIR = ./inc/imgui/

all: clean imgui demo
	@echo "Done!"

clean:
	rm -rf ./bin/demo
	rm -rf ./lib/libimgui.a

demo:
	$(CC) $(STD) $(CARGS) ./src/*.cpp -I$(INC_DIR)/imgui -L$(LIB_DIR) $(LFLAGS) -limgui -o ./bin/demo

imgui:
	mkdir -p $(LIB_DIR)imgui
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui.o $(IMGUI_DIR)imgui.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_demo.o $(IMGUI_DIR)imgui_demo.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_draw.o $(IMGUI_DIR)imgui_draw.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_tables.o $(IMGUI_DIR)imgui_tables.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_widgets.o $(IMGUI_DIR)imgui_widgets.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_impl_sdl.o $(IFLAGS) $(IMGUI_DIR)backends/imgui_impl_sdl.cpp
	clang++ $(STD) -I$(INC_DIR)imgui -c -o $(LIB_DIR)imgui/imgui_impl_sdlrenderer.o $(IFLAGS) $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer.cpp
	ar cr $(LIB_DIR)libimgui.a $(LIB_DIR)imgui/*.o