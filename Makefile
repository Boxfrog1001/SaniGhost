CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lsqlite3

TARGET = sanighost
SRC = src/main.c src/shredder.c src/sqlite_wipe.c src/dir_walker.c src/thumbnail_wipe.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
