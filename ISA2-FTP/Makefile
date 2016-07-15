PROJECT = ftpclient

CC = `command -v g++48 >/dev/null 2>&1 && { echo "g++48"; true; } || echo "g++"`
RM = rm -rf
TAR = tar -czf

CFLAGS = -Wall -Wextra -std=c++11 -DSLOG_MAX_LEVEL=Logger::Severity::Notice -O3

SRC_FILES = main.cpp App.cpp FtpClient.cpp Utilities.cpp Slog.cpp
OBJ_FILES = main.o App.o FtpClient.o Utilities.o Slog.o

TAR_FILE = xspisi03.tar.gz

all:
	$(CC) $(CFLAGS) -o $(PROJECT) $(SRC_FILES)

clean:
	$(RM) $(PROJECT) $(OBJ_FILES)

pack:
	$(TAR) $(TAR_FILE) *.cpp *.h Makefile

.PHONY: all clean pack