wookie_glfw : wookie_glfw.cc common/controls.cpp
	g++ -o wookie_glfw -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lassimp -I. wookie_glfw.cc common/controls.cpp

simple_sample : simple_sample.cc
	g++ -o simple_sample -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lassimp -I. simple_sample.cc

simple_obj : simple_obj.cc
	g++ -o simple_obj -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lassimp -I. simple_obj.cc
