# Makefile for project1-shell

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Executable name
TARGET = my_shell

# Source files
SRCS = main.c parser.c history.c

# Build rule
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean rule
clean:
	rm -f $(TARGET) *.o
