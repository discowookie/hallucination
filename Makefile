wookie_glfw : wookie_glfw.cc
	g++ -o wookie_glfw -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lassimp wookie_glfw.cc

simple_sample : simple_sample.cc
	g++ -o simple_sample -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lassimp simple_sample.cc
