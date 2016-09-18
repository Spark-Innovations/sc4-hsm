# set directory for output products
BUILD = build
TOP := .
BUILD_VERBOSE=1

ifeq ($(BUILD_VERBOSE),0)
Q = @
else
Q =
endif

# default settings; can be overriden in main Makefile
RM = rm
ECHO = @echo
CP = cp
MKDIR = mkdir
SED = sed
PYTHON = python

CROSS_COMPILE = ~/devel/spark/sc4/gcc-arm/arm-none-eabi-

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

# NOTE: "-u _printf_float" is supposed to enable the %f printf directive
# but it doesn't seem to work
LD = $(CROSS_COMPILE)ld -u _printf_float

OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
ifeq ($(MICROPY_FORCE_32BIT),1)
CC += -m32
CXX += -m32
LD += -m32
endif

all:
.PHONY: all

.DELETE_ON_ERROR:

DFU=../tools/dfu.py
USE_PYDFU ?= 0
PYDFU = ../tools/pydfu.py
DFU_UTIL ?= dfu-util
DEVICE=0483:df11

CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -Og -fmessage-length=0 -fsigned-char 
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c
CFLAGS += -ffunction-sections -fdata-sections -ffreestanding
CFLAGS += -fno-move-loop-invariants -Wall -Wextra  
CFLAGS += -g3 -DDEBUG -DUSE_FULL_ASSERT -DSTM32F405xx -DUSE_HAL_DRIVER
CFLAGS += -DHSE_VALUE=8000000 -std=gnu11
CFLAGS+= $(INC)

INC = -I./sc4 -I./lib -I./stm -I./tinyscheme -I./u2f -I./mbedtls/include

LIBS= -lm -lc -specs=nosys.specs

SRC = .

SRC_ASM = $(SRC)/lib/startup_stm32f415xx.s

SRC_CPP = $(wildcard $(SRC)/sc4/*.cpp)
SRC_CPP += $(wildcard ./lib/*.cpp)
SRC_CPP += $(wildcard ./src/stubs/*.cpp)

SRC_C = $(wildcard ./stm/*.c)
SRC_C += $(wildcard ./lib/*.c)
SRC_C += $(wildcard ./tinyscheme/*.c)
SRC_C += $(wildcard ./stubs/*.c)
SRC_C += $(wildcard ./sc4/*.c)
SRC_C += $(wildcard ./mbedtls/library/*.c)
SRC_C += $(wildcard ./u2f/*.c)

OBJ = $(addprefix $(BUILD)/, $(SRC_ASM:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CPP:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

all: $(BUILD)/firmware.dfu

.PHONY: deploy

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
ifeq ($(USE_PYDFU),1)
	sudo $(Q)$(PYTHON) $(PYDFU) -u $<
else
	sudo $(Q)$(DFU_UTIL) -a 0 -d $(DEVICE) -D $<
endif

$(BUILD)/firmware.dfu: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector $^ $(BUILD)/firmware0.bin
	$(Q)$(OBJCOPY) -O binary -j .text -j .inits -j .data $^ $(BUILD)/firmware1.bin
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware0.bin -b 0x08020000:$(BUILD)/firmware1.bin $@

$(BUILD)/firmware.hex: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O ihex $< $@

obj: $(OBJ)

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(CXX) -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -T ldscripts/mem.ld -T ldscripts/sections.ld -nostartfiles -Xlinker --gc-sections -Wl,-Map,"build/sc4port.map" --specs=nano.specs -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@
#	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

INSERT_USB_IDS = ../tools/insert-usb-ids.py
FILE2H = ../tools/file2h.py

ifneq ($(lastword a b),b)
$(error These Makefiles require make 3.81 or newer)
endif

# Set TOP to be the path to get from the current directory (where make was
# invoked) to the top of the tree. $(lastword $(MAKEFILE_LIST)) returns
# the name of this makefile relative to where make was invoked.
#
# We assume that this file is in the py directory so we use $(dir ) twice
# to get to the top of the tree.

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
TOP := ".."

# Turn on increased build verbosity by defining BUILD_VERBOSE in your main
# Makefile or in your environment. You can also use V=1 on the make command
# line.

ifeq ("$(origin V)", "command line")
BUILD_VERBOSE=$(V)
endif
ifndef BUILD_VERBOSE
BUILD_VERBOSE = 0
endif
ifeq ($(BUILD_VERBOSE),0)
Q = @
else
Q =
endif
# Since this is a new feature, advertise it
ifeq ($(BUILD_VERBOSE),0)
$(info Use make V=1 or set BUILD_VERBOSE in your environment to increase build verbosity.)
endif

# default settings; can be overriden in main Makefile

PY_SRC ?= $(TOP)/py
BUILD ?= build

RM = rm
ECHO = @echo
CP = cp
MKDIR = mkdir
SED = sed
PYTHON = python

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++ 
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
ifeq ($(MICROPY_FORCE_32BIT),1)
CC += -m32
CXX += -m32
LD += -m32
endif

all:
.PHONY: all

.DELETE_ON_ERROR:

######################################

# This file expects that OBJ contains a list of all of the object files.
# The directory portion of each object file is used to locate the source
# and should not contain any ..'s but rather be relative to the top of the 
# tree.
#
# So for example, py/map.c would have an object file name py/map.o
# The object files will go into the build directory and mantain the same
# directory structure as the source tree. So the final dependency will look
# like this:
#
# build/py/map.o: py/map.c
#
# We set vpath to point to the top of the tree so that the source files
# can be located. By following this scheme, it allows a single build rule
# to be used to compile all .c files.

vpath %.S . $(TOP)
$(BUILD)/%.o: %.S
	$(ECHO) "CC $<"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

vpath %.s . $(TOP)
$(BUILD)/%.o: %.s
	$(ECHO) "AS $<"
	$(Q)$(AS) -o $@ $<

define compile_c
$(ECHO) "CC $<"
$(Q)$(CC) $(CFLAGS) -c -MD -o $@ $<
@# The following fixes the dependency file.
@# See http://make.paulandlesley.org/autodep.html for details.
@# Regex adjusted from the above to play better with Windows paths, etc.
@$(CP) $(@:.o=.d) $(@:.o=.P); \
  $(SED) -e 's/#.*//' -e 's/^.*:  *//' -e 's/ *\\$$//' \
      -e '/^$$/ d' -e 's/$$/ :/' < $(@:.o=.d) >> $(@:.o=.P); \
  $(RM) -f $(@:.o=.d)
endef

define compile_cpp
$(ECHO) "CPP $<"
$(Q)$(CXX) $(CFLAGS) -c -MD -o $@ $<
@# The following fixes the dependency file.
@# See http://make.paulandlesley.org/autodep.html for details.
@# Regex adjusted from the above to play better with Windows paths, etc.
@$(CP) $(@:.o=.d) $(@:.o=.P); \
  $(SED) -e 's/#.*//' -e 's/^.*:  *//' -e 's/ *\\$$//' \
      -e '/^$$/ d' -e 's/$$/ :/' < $(@:.o=.d) >> $(@:.o=.P); \
  $(RM) -f $(@:.o=.d)
endef


vpath %.c . ..
$(BUILD)/%.o: %.c
	$(call compile_c)

vpath %.cpp . ..
$(BUILD)/%.o: %.cpp
	$(call compile_cpp)

$(BUILD)/%.pp: %.c
	$(ECHO) "PreProcess $<"
	$(Q)$(CC) $(CFLAGS) -E -Wp,-C,-dD,-dI -o $@ $<

# The following rule uses | to create an order only prereuisite. Order only
# prerequisites only get built if they don't exist. They don't cause timestamp
# checking to be performed.
#
# We don't know which source files actually need the generated.h (since
# it is #included from str.h). The compiler generated dependencies will cause
# the right .o's to get recompiled if the generated.h file changes. Adding
# an order-only dependendency to all of the .o's will cause the generated .h
# to get built before we try to compile any of them.
# $(OBJ): | $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/mpversion.h

# $(sort $(var)) removes duplicates
#
# The net effect of this, is it causes the objects to depend on the
# object directories (but only for existence), and the object directories
# will be created if they don't exist.
OBJ_DIRS = $(sort $(dir $(OBJ)))
$(OBJ): | $(OBJ_DIRS)
$(OBJ_DIRS):
	$(MKDIR) -p $@

clean:
	$(RM) -rf $(BUILD)
	$(RM) -f scheme firmware.bin
.PHONY: clean

print-cfg:
	$(ECHO) "TOP = " $(TOP)
	$(ECHO) "PY_SRC = $(PY_SRC)"
	$(ECHO) "BUILD  = $(BUILD)"
	$(ECHO) "OBJ    = $(OBJ)"
	$(ECHO) "SRC_C  = $(SRC_C)"
	$(ECHO) "SRC_CPP  = $(SRC_CPP)"
	$(ECHO) "SRC_LIB  = $(SRC_LIB)"
.PHONY: print-cfg

print-def:
	@$(ECHO) "The following defines are built into the $(CC) compiler"
	touch __empty__.c
	@$(CC) -E -Wp,-dM __empty__.c
	@$(RM) -f __empty__.c

scheme: ./tinyscheme/scheme.c
	gcc ./tinyscheme/scheme.c -I./tinyscheme -o scheme

dump:
	rm -f firmware.bin
	dfu-util -a 0 -s 0:0x100000 -U firmware.bin
