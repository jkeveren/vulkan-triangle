DEBUG = 0

# CFLAGS
CFLAGS = -std=c++17
ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O3
endif

# LDFLAGS
## Vulkan
LDFLAGS = -lvulkan
## GLFW
LDFLAGS += -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

triangle: main.cpp
	g++ $(CFLAGS) main.cpp -o triangle $(LDFLAGS)

.PHONY: debug watch clean

debug:
	make triangle DEBUG=1 -B && gdb -ex run ./triangle

watch:
	ls *.cpp | entr -cr make debug

clean:
	rm ./triangle
