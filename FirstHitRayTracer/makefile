# Compiler
CXX = g++

# Make sure the folder paths are correct
CXXFLAGS = -g -std=c++17 -I"C:/path/to/cap5705_a2/include"
LDFLAGS = -L"C:/path/to/cap5705_a2/lib"


LDLIBS = -lglew32 -lglfw3dll -lopengl32

TARGET = rt2.exe
SRC = rt2.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	del /Q $(TARGET)
