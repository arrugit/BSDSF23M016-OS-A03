# === Makefile for Base Shell ===

CC = gcc
CFLAGS = -Wall -Iinclude
OBJDIR = obj
BINDIR = bin

SRC = src/main.c src/shell.c src/execute.c
OBJS = $(OBJDIR)/main.o $(OBJDIR)/shell.o $(OBJDIR)/execute.o
TARGET = $(BINDIR)/myshell

# Default target
all: $(TARGET)

# Create bin and obj directories if not exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Rule to compile object files
$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files into the final binary
$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Run the shell
run: all
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)
