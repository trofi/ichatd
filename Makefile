TARGET  = ichatd

# features:
# LINUX_PPOLL

CC      := gcc
CFLAGS  += -MMD -W -Wall -g -O0 -D_REENTRANT -D_THREAD_SAFE -D__POSIX__ # -DLINUX_PPOLL
LIBS    += -pthread

SRC     := $(wildcard src/*.c)
OBJS    := $(SRC:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS)    $^ -o $@ $(LIBS)
	@echo "[LD] $@"

obj/%.o: src/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC] $@"

obj:
	mkdir $@

clean:
	rm -rf $(TARGET) src/*~ TAGS BROWSE

TAGS:
	src/etags *.[ch]

-include obj/*.d
