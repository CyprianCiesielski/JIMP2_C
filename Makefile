CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3 -march=native -flto
CFLAGS += -pthread
CFLAGS += -I./tests
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/main

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

LDFLAGS = -pthread -flto

TEST_SRCS = $(wildcard tests/*.c)
TEST_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

all: check_dirs $(TARGET)

check_dirs:
	@echo "Checking source directory..."
	@if [ ! -d $(SRC_DIR) ]; then echo "$(SRC_DIR) directory not found!"; exit 1; fi
	@echo "Source files found: $(SRCS)"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) -lm
	@echo "Build successful!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Targets for individual tests
test_file_reader: check_dirs
	@echo "Building and running file reader tests..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_file_reader \
		tests/test_file_reader.c \
		$(TEST_OBJS)
	./$(BIN_DIR)/test_file_reader

test_region_growing: check_dirs
	@echo "Building and running region growing tests..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_region_growing \
		tests/test_region_growing.c \
		$(TEST_OBJS)
	./$(BIN_DIR)/test_region_growing

test_graph: check_dirs
	@echo "Building and running graph tests..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_graph \
		tests/test_graph.c \
		$(TEST_OBJS)
	./$(BIN_DIR)/test_graph

test_partition: check_dirs
	@echo "Building and running partition tests..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_partition \
		tests/test_partition.c \
		$(TEST_OBJS)
	./$(BIN_DIR)/test_partition

test_fm_optimization: check_dirs
	@echo "Building and running partition tests..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_fm_optimization \
		tests/test_fm_optimization.c \
		$(TEST_OBJS)
	./$(BIN_DIR)/test_fm_optimization

# Main test target that runs all tests
tests: test_file_reader test_region_growing test_graph test_partition test_fm_optimization
	@echo "All tests completed."

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

debug:
	@echo "Source directory: $(SRC_DIR)"
	@echo "Source files: $(SRCS)"
	@echo "Object files: $(OBJS)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)

.PHONY: all clean debug check_dirs tests test_file_reader test_region_growing test_graph test_partition