CC= gcc
RM= rm -vf
CFLAGS= -Wall `pkg-config fuse --cflags --libs` -lzip -g
fusemake:
        gcc -Wall fusezip.c `pkg-config fuse --cflags --libs` -lzip -g -o fusezip

.PHONY: all clean

all: $(PROGFILES)
clean:
        $(RM) $(OBJFILES) $(PROGFILES) *~
