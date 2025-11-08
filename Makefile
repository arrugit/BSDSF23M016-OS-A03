# === Makefile for MyShell (Feature 4: Readline) ===

CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lreadline
OBJDIR = obj
BINDIR = bin

SRC = src/main.c src/shell.c src/execute.c
OBJS = $(OBJDIR)/main.o $(OBJDIR)/shell.o $(OBJDIR)/execute.o
TARGET = $(BINDIR)/myshell

# Default target
all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
