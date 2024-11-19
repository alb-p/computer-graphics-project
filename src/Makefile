CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr -lXi
PacMaze: PacMaze.cpp
	g++ $(CFLAGS) -o PacMaze -I headers -I models -I shaders -I textures PacMaze.cpp $(LDFLAGS)

.PHONY: test clean

test: PacMaze
	./PacMaze
clean:
	rm -f PacMaze

