
CC = gcc
CFLAGS = -I. -Wall -std=c99

WAD_TOOL = wad
WAD_TOOL_OBJS = wad.o wadtool.o

TEXDUMP_TOOL = texdump
TEXDUMP_OBJS = wad.o texdump.o

all: Makefile $(WAD_TOOL) $(TEXDUMP_TOOL)

$(WAD_TOOL): Makefile $(WAD_TOOL_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(WAD_TOOL_OBJS) -o $(WAD_TOOL)


$(TEXDUMP_TOOL): Makefile $(TEXDUMP_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TEXDUMP_OBJS) -o $(TEXDUMP_TOOL)

clean:
	rm -f *.o *~
	rm -f $(WAD_TOOL)
	rm -f $(TEXDUMP_TOOL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@



