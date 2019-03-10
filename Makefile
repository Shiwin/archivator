CC=$(TOOLCHAIN)gcc
LINK := $(CC)
CFLAGS += -std=c99 -Wall -g
SOURCES := archivator.c
EXECUTABLE := archivator
TEST_RESULT_FILE := test_result
TEST_RESULT_UNARCHIVE := /tmp/archivator_test
TEST_DIR := test_dir

build: $(SOURCES) $(EXECUTABLE)

test: build
	rm -rfv $(TEST_RESULT_FILE) $(TEST_RESULT_UNARCHIVE)
	echo "=== Test dumping ==="
	./$(EXECUTABLE) $(TEST_DIR)
	echo "=== Test archiving ==="
	./$(EXECUTABLE) $(TEST_DIR) $(TEST_RESULT_FILE)
	echo "=== Dump archive ==="
	./$(EXECUTABLE) $(TEST_RESULT_FILE) -C $(TEST_RESULT_UNARCHIVE)

all: test

clean:
	rm -rf $(EXECUTABLE)

commit: clean
	git add -u
	git commit
