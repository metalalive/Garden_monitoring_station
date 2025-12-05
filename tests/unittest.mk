UNITY_ROOT ?= /usr/local/include/unity

# Path to jsmn library (assuming jsmn.h is in this directory)
JSMN_ROOT ?= /usr/local/include/jsmn

CC = gcc

UNITY_SRC = \
	$(UNITY_ROOT)/extras/fixture/src/unity_fixture.c \
	$(UNITY_ROOT)/src/unity.c

TEST_BUILD_DIR = $(BUILD_DIR_TOP)/utest

TEST_SRC = tests/mocks.c tests/entry.c tests/app_msg.c tests/util_str_proc.c \
		   tests/IO/actuator.c tests/IO/sensor_event.c tests/IO/display.c \
		   tests/IO/sensor_sample.c tests/util_stats.c

APP_SRC = src/util.c src/app_msg.c src/IO/sensor_event.c src/IO/actuator.c \
		  src/IO/display.c

# All source files for the test executable
ALL_TEST_SOURCES = $(APP_SRC) $(TEST_SRC) $(UNITY_SRC) $(JSMN_SRC)

# Object files
TEST_OBJS = $(patsubst %.c, $(TEST_BUILD_DIR)/%.o, $(ALL_TEST_SOURCES))

# Compiler flags
TEST_CFLAGS = -Wall -Wextra -std=c2x -g -O0
TEST_CFLAGS += -I$(MONT_STATION_PROJ_HOME)/include
TEST_CFLAGS += -I$(MONT_STATION_PROJ_HOME)/tests
TEST_CFLAGS += -I$(UNITY_ROOT)/src -I$(UNITY_ROOT)/extras/fixture/src
TEST_CFLAGS += -I$(JSMN_ROOT)
TEST_CFLAGS += -DUNITY_EXCLUDE_SETJMP_H  -DUNITY_EXCLUDE_MATH_H  -DUNITY_FIXTURE_NO_EXTRAS

# Linker flags
TEST_LDFLAGS = -lm

# Target executable name
TEST_EXE = $(TEST_BUILD_DIR)/utest.out

.PHONY: test test_clean

# Test build rule
test: $(TEST_BUILD_DIR) $(TEST_EXE)
	@echo "Running unit tests..."
	@$(TEST_EXE)

$(TEST_EXE): $(TEST_OBJS)
	@mkdir -p $(@D)
	@$(CC) $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)
	@echo "Unit test executable built: $@"

$(TEST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_BUILD_DIR):
	@mkdir -p $@


# Clean rules
test_clean:
	@echo "Cleaning unit test build artifacts..."
	@$(RM) -r $(TEST_BUILD_DIR)
