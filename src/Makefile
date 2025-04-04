
EXE = cpuhog

OBJ = cpuhog.o

CROSS_COMPILE ?=
CC             ?= $(CROSS_COMPILE)gcc
CFLAGS         += -Wall -W -pthread
LDFLAGS        += -pthread

.PHONY: all
all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c  $<

.PHONY: clean

clean:
	rm -f $(EXE) $(OBJ) *~

