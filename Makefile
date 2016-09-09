ROOTDIR = $(N64_INST)
GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header

SRCDIR = ./src
RESDIR = ./res
OBJDIR = ./obj
BINDIR = ./bin

LINK_FLAGS = -O1 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib -ldragon -lmikmod -lmad -lyaml -lc -lm -ldragonsys -lnosys $(LIBS) -Tn64ld.x
PROG_NAME = OS64
CFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -O1 -I./inc -I$(SRCDIR) -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -lpthread -lrt -D_REENTRANT -DUSE_TRUETYPE $(SET_DEBUG)
ASFLAGS = -mtune=vr4300 -march=vr4300
CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
#SOURCES := menu.c everdrive.c fat.c disk.c mem.c sys.c ini.c strlib.c utils.c sram.c stb_image.c chksum64.c mp3.c  
#SOURCES := $(wildcard $(SRCDIR)/*.c)
#OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJECTS = menu.o everdrive.o fat.o disk.o mem.o sys.o ini.o strlib.o utils.o sram.o chksum64.o mp3.o

$(PROG_NAME).v64: $(PROG_NAME).elf $(PROG_NAME).dfs
	$(OBJCOPY) $(BINDIR)/$(PROG_NAME).elf $(BINDIR)/$(PROG_NAME).bin -O binary
	rm -f $(BINDIR)/$(PROG_NAME).v64
	$(N64TOOL) -l 4M -t "EverDrive OS" -h $(RESDIR)/header.ed64 -o $(BINDIR)/$(PROG_NAME).v64 $(BINDIR)/$(PROG_NAME).bin -s 1M $(BINDIR)/$(PROG_NAME).dfs
	$(CHKSUM64PATH) $(BINDIR)/$(PROG_NAME).v64

$(PROG_NAME).elf : $(OBJECTS)
	$(LD) -o $(BINDIR)/$(PROG_NAME).elf $(OBJECTS) $(LINK_FLAGS)

#$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	#@$(CC) $(CFLAGS) -c $< -o $@

copy: $(PROG_NAME).v64
	sh ./tools/upload.sh

$(PROG_NAME).dfs:
	$(MKDFSPATH) $(BINDIR)/$(PROG_NAME).dfs $(RESDIR)/filesystem/

all: $(PROG_NAME).v64

debug: $(PROG_NAME).v64

debug: SET_DEBUG=-DDEBUG

clean:
	rm -f $(BINDIR)/*.v64 $(BINDIR)/*.elf *.o $(OBJDIR)/*.o $(BINDIR)/*.bin $(BINDIR)/*.dfs
