CXX := clang++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -pedantic

INCLUDES := -I. \
            -Ithird_party \
            -Ithird_party/SDL2 \
            -Ithird_party/SDL2_image \
            -Ithird_party/SDL2_mixer \
            -Ithird_party/SDL2_ttf \
            -Ithird_party/lua \
            -Ithird_party/LuaBridge \
            -Ithird_party/box2d/include \
            -Ithird_party/box2d/src

LIBS := -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -llua5.4

TARGET := game_engine_linux

SRCS := \
main.cpp \
TemplateDB.cpp \
Actor.cpp \
SceneDB.cpp \
Renderer.cpp \
ImageDB.cpp \
Input.cpp \
AudioDB.cpp \
TextDB.cpp \
ComponentDB.cpp \
Component.cpp \
Rigidbody.cpp \
ParticleSystem.cpp \
ContactListener.cpp \
EventBus.cpp

BOX2D_SRCS := $(shell find third_party/box2d/src -name '*.cpp')

ALL_SRCS := $(SRCS) $(BOX2D_SRCS)
OBJS := $(ALL_SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
