CC=$(TOOLCHAIN)gcc
LINK := $(CC)
CFLAGS += -std=c99 -Wall -g
SOURCES := archivator.c
EXECUTABLE := archivator

all:  $(SOURCES) $(EXECUTABLE)

clean:
	rm -rf $(EXECUTABLE)

commit: clean
	git add -u
	git commit
