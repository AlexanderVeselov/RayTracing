APP = RayTrace

all: $(APP)

SRC += camera.cpp
SRC += grid.cpp
SRC += main.cpp
SRC += scene.cpp

CPPFLAGS += -std=gnu++11

#LIBS += SDL
LIBS += GL
LIBS += GLU
LIBS += glut
LIBS += glfw3

LIBS += X11
LIBS += Xxf86vm
LIBS += Xrandr
LIBS += Xi
LIBS += Xinerama
LIBS += Xcursor

LIBS += OpenCL

LIBS += dl
LIBS += rt
LIBS += m
LIBS += pthread

$(APP): $(SRC:.cpp=.o)
	g++ -o $@ $^ $(addprefix -l,$(LIBS))

clean:
	rm -f $(APP) *.o
