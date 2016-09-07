ROOTDIR = $(N64_INST)
GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header


LINK_FLAGS = -O1 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib -ldragon -lmikmod -lmad -lyaml -lc -lm -ldragonsys -lnosys $(LIBS) -Tn64ld.x
PROG_NAME = OS64
CFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -O1 -I./inc -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -lpthread -lrt -D_REENTRANT -DUSE_TRUETYPE $(SET_DEBUG)
ASFLAGS = -mtune=vr4300 -march=vr4300
CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
OBJS = menu.o everdrive.o fat.o disk.o mem.o sys.o ini.o strlib.o utils.o sram.o stb_image.o chksum64.o mp3.o

$(PROG_NAME).v64: $(PROG_NAME).elf $(PROG_NAME).dfs
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).v64
	$(N64TOOL) -l 4M -t "EverDrive OS" -h ./res/header.ed64 -o $(PROG_NAME).v64 $(PROG_NAME).bin -s 1M $(PROG_NAME).dfs
	$(CHKSUM64PATH) $(PROG_NAME).v64

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)

copy: $(PROG_NAME).v64
	sh upload.sh

$(PROG_NAME).dfs:
	$(MKDFSPATH) $(PROG_NAME).dfs ./res/filesystem/

all: $(PROG_NAME).v64

debug: $(PROG_NAME).v64

debug: SET_DEBUG=-DDEBUG

clean:
	rm -f *.v64 *.elf *.o *.bin *.dfs
