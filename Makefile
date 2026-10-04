CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
TARGET := mini-shell
SOURCES := src/main.cpp src/shell.cpp src/parser.cpp src/executor.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) src/shell.h src/parser.h src/executor.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
