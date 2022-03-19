CFLAGS = -std=c++17 -O2

# LDFLAGS
## Vulkan
LDFLAGS = -lvulkan
## GLFW
LDFLAGS += -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

triangle: main.cpp
	g++ $(CFLAGS) main.cpp -o triangle $(LDFLAGS)

.PHONY: watch clean

watch:
	ls *.cpp | entr -cr sh -c "make triangle && ./triangle"

clean:
	rm ./triangle
