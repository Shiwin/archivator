CC=$(TOOLCHAIN)gcc
LINK := $(CC)
CFLAGS += -std=c99 -Wall -g
SOURCES := archivator.c
EXECUTABLE := archivator
TEST_RESULT_FILE := test_result
TEST_DIR := test_dir

build: $(SOURCES) $(EXECUTABLE)

test: build
	rm -rf $(TEST_RESULT_FILE)
	echo "=== Test dumping ==="
	./$(EXECUTABLE) $(TEST_DIR)
	echo "=== Test archiving ==="
	./$(EXECUTABLE) $(TEST_DIR) $(TEST_RESULT_FILE)
	echo "=== Dump archive ==="
	./$(EXECUTABLE) $(TEST_RESULT_FILE) -C /tmp/out

all: test

clean:
	rm -rf $(EXECUTABLE)

commit: clean
	git add -u
	git commit
