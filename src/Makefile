CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr -lXi
A10: A10.cpp
	g++ $(CFLAGS) -o A10 -I headers -I models -I shaders -I textures A10.cpp $(LDFLAGS)

.PHONY: test clean

test: A10
	./A10
clean:
	rm -f A10

