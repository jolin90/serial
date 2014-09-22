# Cross compiler
CROSS_COMPILE   ?= arm-linux-gnueabihf-
CC      	:= $(CROSS_COMPILE)gcc
LD      	:= $(CROSS_COMPILE)ld
STRIP       := $(CROSS_COMPILE)strip
OBJCOPY     := $(CROSS_COMPILE)objcopy
OBJDUMP     := $(CROSS_COMPILE)objdump

# Tools
RM      	:= rm -rf
MKDIR		:= mkdir -p
MV      	:= mv -f
SED     	:= sed

CFLAGS		:= -Wall -I ./../include/

OBJS 		:= tmgetfile.o crc16.o
BIN			:= tm-recv

all: $(BIN)

$(BIN): $(OBJS)
	@ echo "[LD] $@ <= $^"
	@ $(CC) -o $@ $^

%.o: %.c
	@ echo "[CC] $^ => $@"
	@ $(COMPILE.c) -o $@ $^ $(CFLAGS)

clean:
	@$(RM) $(BIN) *.o
