TARGET = main

UPLOAD_PORT=/dev/ttyACM0
BUILD_DIR = ./build
BASE_DIR  = ../ATSAM3X8EA-AU-bare-metal

.SECONDEXPANSION:
.PRECIOUS: $(BUILD_DIR)/ $(BUILD_DIR)%/

# Define the linker script location and chip architecture.
LD_SCRIPT = $(BASE_DIR)/system/sam3x8a.ld
MCU_SPEC  = cortex-m3

# Toolchain definitions (ARM bare metal defaults)
TOOLCHAIN = /usr/local
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OC = arm-none-eabi-objcopy
OD = arm-none-eabi-objdump
OS = arm-none-eabi-size

# Assembly directives.
ASFLAGS += -c
#ASFLAGS += -O0
ASFLAGS += -mcpu=$(MCU_SPEC)
ASFLAGS += -mthumb
ASFLAGS += -Wall
# (Set error messages to appear on a single line.)
ASFLAGS += -fmessage-length=0

# C compilation directives
REQ_CFLAGS += -mcpu=$(MCU_SPEC)
REQ_CFLAGS += -mthumb
REQ_CFLAGS += -Wall
#CFLAGS += -g
CFLAGS += -Os
REQ_CFLAGS += --specs=nosys.specs
# (Set error messages to appear on a single line.)
#REQ_CFLAGS += -fmessage-length=0 -ffreestanding
REQ_CFLAGS += -ffreestanding
# (Set system to ignore semihosted junk)
#REQ_CFLAGS += --specs=nano.specs

# Linker directives.
REQ_LFLAGS += -nostdlib -nodefaultlibs
#LFLAGS += -lgcc
REQ_LFLAGS += -nostartfiles
LFLAGS += --cref -Map $(BUILD_DIR)/$(TARGET).map
REQ_LFLAGS += -T./$(LD_SCRIPT)

STARTUP_SRC	= startup_sam3x8a.c
C_SRC    	= 
APP_SRC    	= main.c

include $(BASE_DIR)/board/board.mk
include $(BASE_DIR)/system/system.mk
include $(BASE_DIR)/os/os.mk
include $(BASE_DIR)/hal/hal.mk

#OBJS = $(BUILD_DIR)/$(STARTUP_SRC:.s=.o)
#OBJS = $(BUILD_DIR)/$(STARTUP_SRC:.s=.o) $(BUILD_DIR)/$(STARTUP_SRC:.c=.o)
OBJS = $(BUILD_DIR)/$(STARTUP_SRC:.c=.o)
OBJS += $(addprefix $(BUILD_DIR)/, $(notdir $(C_SRC:.c=.o)))
OBJS += $(BUILD_DIR)/$(APP_SRC:.c=.o)

.PHONY: all, list
all: $(BUILD_DIR)/$(TARGET).bin

list: $(C_SRC) $(APP_SRC) $(STARTUP_SRC)
	$(CC) -g -Wa,-adhln $(INCLUDE_DIRS) $(REQ_CFLAGS) $$(CFLAGS) $$^ > $(BUILD_DIR)/$(TARGET).lst

$(BUILD_DIR)/%.o: $(BASE_DIR)/system/%.s | $$(@D)/
	$(CC) -x assembler-with-cpp $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(BASE_DIR)/system/%.S | $$(@D)/
	$(CC) -x assembler-with-cpp $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c | $$(@D)/
	$(CC) -c $(INCLUDE_DIRS) $(REQ_CFLAGS) $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(LD) $^ $(REQ_LFLAGS) $(LFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OC) -S -O binary $< $@
	$(OS) $<

$(BUILD_DIR)/:
	mkdir -p $@

$(BUILD_DIR)%/:
	mkdir -p $@

dump: $(BUILD_DIR)/$(TARGET).elf
	$(OD) -D -m armv6-m $< -Mforce-thumb

dumpbin: $(BUILD_DIR)/$(TARGET).bin
	$(OD) -D -m armv6-m -b binary $< -Mforce-thumb

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

run:
	sudo stty -F "$(UPLOAD_PORT)" speed 1200 cs8 -cstopb -parenb 1200
	sudo stty -F "$(UPLOAD_PORT)" speed 1200 cs8 -cstopb -parenb 1200
	bossac -i -d --port="$(UPLOAD_PORT)" -U false -e -w -v -b "$(BUILD_DIR)/main.bin" -R
