CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
CPPFLAGS += -Iinclude $(shell pkg-config --cflags ftxui)
LDLIBS += $(shell pkg-config --libs ftxui)

ROOT := $(CURDIR)
BUILD_DIR := $(ROOT)/build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := $(ROOT)/src
TEST_DIR := $(ROOT)/tests
INCLUDE_DIR := $(ROOT)/include

SORD_BIN := $(BUILD_DIR)/sord
TEST_BIN := $(BUILD_DIR)/sord_tests

SORD_OBJS := \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/editor/document.o \
	$(OBJ_DIR)/editor/editor.o \
	$(OBJ_DIR)/renderer/editor_renderer.o \
	$(OBJ_DIR)/app/application.o \
	$(OBJ_DIR)/app/save_manager.o

TEST_OBJS := \
	$(OBJ_DIR)/tests/document_test.o \
	$(OBJ_DIR)/editor/document.o

.PHONY: all test run clean distclean

all: $(SORD_BIN)

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.cpp $(INCLUDE_DIR)/app/application.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/editor/document.o: $(SRC_DIR)/editor/document.cpp $(INCLUDE_DIR)/editor/document.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/editor/editor.o: $(SRC_DIR)/editor/editor.cpp $(INCLUDE_DIR)/editor/editor.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/renderer/editor_renderer.o: $(SRC_DIR)/renderer/editor_renderer.cpp $(INCLUDE_DIR)/renderer/editor_renderer.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/app/application.o: $(SRC_DIR)/app/application.cpp $(INCLUDE_DIR)/app/application.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/app/save_manager.o: $(SRC_DIR)/app/save_manager.cpp $(INCLUDE_DIR)/app/save_manager.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/document_test.o: $(TEST_DIR)/document_test.cpp $(INCLUDE_DIR)/editor/document.hpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(SORD_BIN): $(SORD_OBJS)
	$(CXX) $(SORD_OBJS) $(LDLIBS) -o $@

$(TEST_BIN): $(TEST_OBJS)
	$(CXX) $(TEST_OBJS) $(LDLIBS) -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

run: $(SORD_BIN)
	$(SORD_BIN)

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
