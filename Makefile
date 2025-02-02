# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -O2

# Executable name
TARGET = cache_simulator

# Source files
SRCS = cache_simulator.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Rule to rebuild the project
rebuild: clean all

# Phony targets (not actual files)
.PHONY: all clean rebuild
