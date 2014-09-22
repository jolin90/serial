# Cross compiler
CROSS_COMPILE   ?=
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

OBJS 		:= tmpushfile.o progress_bar.o crc16.o
BIN			:= tm-send

all: $(BIN)

$(BIN): $(OBJS)
	@ echo "[LD] $@ <= $^"
	@ $(CC) -o $@ $^

%.o: %.c
	@ echo "[CC] $^ => $@"
	@ $(COMPILE.c) -o $@ $^ $(CFLAGS)

clean:
	@$(RM) $(BIN) $(OBJS)
