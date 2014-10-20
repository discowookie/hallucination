wookie_glfw : wookie_glfw.cc common/controls.cpp
	g++ -o wookie_glfw -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -lglfw3 -lportaudio -laubio -I. -Ithird_party -Ithird_party/aubio wookie_glfw.cc common/controls.cpp
