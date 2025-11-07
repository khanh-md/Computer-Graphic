# Compiler
CXX = g++

# Make sure the folder paths are correct - UPDATE THESE PATHS
CXXFLAGS = -g -std=c++17 -I"./include"
LDFLAGS = -L"./lib"

LDLIBS = -lglew32 -lglfw3dll -lopengl32

# Targets
HELLO_TRIANGLE_TARGET = helloTriangle.exe
OBJECT_TARGET = object.exe
BENCHMARK_TARGET = benchmark.exe
ROTATE_TARGET = rotate.exe

# Sources
HELLO_TRIANGLE_SRC = helloTriangle.cpp
OBJECT_SRC = object.cpp
BENCHMARK_SRC = benchmark.cpp
ROTATE_SRC = rotate.cpp

all: $(HELLO_TRIANGLE_TARGET) $(OBJECT_TARGET) $(ROTATE_TARGET)


$(HELLO_TRIANGLE_TARGET): $(HELLO_TRIANGLE_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJECT_TARGET): $(OBJECT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(BENCHMARK_TARGET): $(BENCHMARK_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(ROTATE_TARGET): $(ROTATE_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# Build specific questions
q1: $(HELLO_TRIANGLE_TARGET)
q2: $(OBJECT_TARGET)
q4: $(BENCHMARK_TARGET)
q5: $(ROTATE_TARGET)

# Run targets
run-q1: $(HELLO_TRIANGLE_TARGET)
	./$(HELLO_TRIANGLE_TARGET)

run-q2: $(OBJECT_TARGET)
	./$(OBJECT_TARGET)

run-benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)

run-q5: $(ROTATE_TARGET)
	./$(ROTATE_TARGET)

clean:
	del /Q *.exe

.PHONY: all clean q1 q2 q4 q5 run-q1 run-q2 run-benchmark run-q5