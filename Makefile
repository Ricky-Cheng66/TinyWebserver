#----------- 基本工具 -----------
CXX      := g++
RM       := rm -rf

#----------- 源文件 & 目录 -----------
SRC_DIR  := src
BUILD_DIR:= build
TARGET   := server

# 递归查找所有 cpp 文件
SRCS     := $(shell find $(SRC_DIR) -name '*.cpp')
# 把 src/xxx.cpp 映射成 build/xxx.o
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

#----------- 默认 flags -----------
CXXFLAGS := -std=c++20 -Wall -Wextra
INCLUDES := -I $(SRC_DIR)                   # 让 #include "http/xxx.h" 能找到
LDFLAGS  :=

# 两种模式
release: CXXFLAGS += -O2 -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

#----------- 隐含规则 -----------
# 自动创建子目录再编译
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

#----------- 链接 -----------
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

#----------- 清理 -----------
clean:
	$(RM) $(BUILD_DIR) $(TARGET)

#----------- 伪目标 -----------
.PHONY: all release debug clean