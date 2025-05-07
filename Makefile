CC = gcc
# Zmień flagi kompilacji - dodaj optymalizacje
CFLAGS = -Wall -Wextra -Iinclude -O3 -march=native -flto
CFLAGS += -pthread
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/main

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

LDFLAGS = -pthread -flto

all: check_dirs $(TARGET)

# Sprawdź czy istnieją katalogi
check_dirs:
	@echo "Checking source directory..."
	@if [ ! -d $(SRC_DIR) ]; then echo "$(SRC_DIR) directory not found!"; exit 1; fi
	@echo "Source files found: $(SRCS)"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

debug:
	@echo "Source directory: $(SRC_DIR)"
	@echo "Source files: $(SRCS)"
	@echo "Object files: $(OBJS)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

.PHONY: all clean debug check_dirs