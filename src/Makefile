CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr -lXi
provaScene: provaScene.cpp
	g++ $(CFLAGS) -o provaScene -I headers -I models -I shaders -I textures provaScene.cpp $(LDFLAGS)

.PHONY: test clean

test: provaScene
	./provaScene
clean:
	rm -f provaScene

