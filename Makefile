MUSL_ROOT ?= /usr/lib/musl

CC = clang
CPPFLAGS += -D_GNU_SOURCE=1 -nostdinc -isystem $(MUSL_ROOT)/include
CFLAGS += -std=c99 -Wall -Wextra -Wshadow -Wpedantic -O3

V ?= 0

LD = clang
LDFLAGS += -fuse-ld=/usr/bin/mold -L$(MUSL_ROOT)/lib -static
LIBS += 

HDR = $(wildcard *.h)
SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
MAIN_SRC = $(shell grep -Pl '^int\s+main\s*\x28int\b' $(SRC))
BIN = $(MAIN_SRC:%.c=%)

.PHONY: all clean

.SECONDARY: $(OBJ)

all: $(BIN)

$(BIN): $(OBJ)
ifeq ($V, 0)
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
else
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
endif

%.o: %.c $(HDR)
ifeq ($V, 0)
	@echo "CC $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
else
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
endif

clean:
	rm -f $(OBJ) $(BIN)
