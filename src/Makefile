CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr -lXi
App: App.cpp
	g++ $(CFLAGS) -o App -I headers -I models -I shaders -I textures App.cpp $(LDFLAGS)

.PHONY: test clean

test: App
	./App
clean:
	rm -f App

