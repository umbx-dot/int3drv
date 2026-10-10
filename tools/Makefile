CC ?= gcc
CFLAGS ?= -std=c23 -Wall -Wextra -Werror -O2 -Iinclude -I../include -MMD -MP

BUILD_DIR ?= ../build/tools
BIN_DIR ?= ../bin
TARGET ?= $(BIN_DIR)/int3ctl
LIB ?= $(BIN_DIR)/libint3ctl.a
TESTLIB ?= $(BIN_DIR)/libtestmap.so

SRCS := main.c \
        util/dev.c \
        util/hex.c \
        util/dump.c \
        lib/api.c \
        commands/cmd_version.c \
        commands/cmd_stats.c \
        commands/cmd_probe.c \
        commands/cmd_read.c \
        commands/cmd_write.c \
        commands/cmd_write_verify.c \
        commands/cmd_force.c \
        commands/cmd_scalar.c \
        commands/cmd_deref.c \
        commands/cmd_chain.c \
        commands/cmd_scan.c \
        commands/cmd_scan_all.c \
        commands/cmd_vscan.c \
        commands/cmd_cksum.c \
        commands/cmd_copy.c \
        commands/cmd_vma.c \
        commands/cmd_modbase.c \
        commands/cmd_maps.c \
        commands/cmd_procs.c \
        commands/cmd_caps.c \
        commands/cmd_manualmap.c \
        commands/cmd_bench.c \
        commands/cmd_selftest.c \
        manualmap/io.c \
        manualmap/elf.c \
        manualmap/open.c \
        manualmap/dyn.c \
        manualmap/copy.c \
        manualmap/hash.c \
        manualmap/lookup.c \
        manualmap/off.c \
        manualmap/reloc.c \
        manualmap/sym.c \
        manualmap/load.c \
        manualmap/run.c \
        manualmap/dlfcn.c \
        manualmap/mprotectv.c \
        manualmap/mprotect.c \
        selftest/t_scalar.c \
        selftest/t_batch.c \
        selftest/t_writes.c \
        selftest/t_validate.c \
        selftest/t_query.c \
        selftest/t_scan.c \
        selftest/t_scanall.c \
        selftest/t_deref.c \
        selftest/t_verify.c \
        selftest/t_scanall.c \
        selftest/t_maps_procs.c \
        selftest/t_v5.c \
        selftest/t_mmap.c \
        selftest/t_thread.c \
        selftest/t_lifecycle.c \
        selftest/run.c

LIB_SRCS := util/dev.c \
            util/hex.c \
            util/dump.c \
            lib/api.c \
            manualmap/io.c \
            manualmap/elf.c \
            manualmap/open.c \
            manualmap/dyn.c \
            manualmap/copy.c \
            manualmap/hash.c \
            manualmap/lookup.c \
            manualmap/off.c \
            manualmap/reloc.c \
            manualmap/sym.c \
            manualmap/load.c \
            manualmap/run.c \
            manualmap/dlfcn.c \
            manualmap/mprotectv.c \
            manualmap/mprotect.c

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
LIB_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
DEPS := $(OBJS:.o=.d) $(LIB_OBJS:.o=.d)

all: $(TARGET) $(LIB) $(TESTLIB)

$(TESTLIB): manualmap/testlib.c | $(BIN_DIR)
	$(CC) -shared -fPIC -O2 -o $@ $<

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(LIB): $(LIB_OBJS) | $(BIN_DIR)
	$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(LIB) $(TESTLIB)

-include $(DEPS)

.PHONY: all clean
