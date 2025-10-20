CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I include
SRCS     := $(wildcard src/*.cpp)
OBJS     := $(SRCS:.cpp=.o)
TARGET   := server

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean