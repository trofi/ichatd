TARGET  = ichatd

# features:
# LINUX_PPOLL

CC      := gcc
CFLAGS  := -MMD -W -Wall -g -O0 -D_REENTRANT -D_THREAD_SAFE -D__POSIX__ # -DLINUX_PPOLL
LIBS    := -pthread

O_PFX   := obj

VPATH   := src:$(O_PFX)


SRC     := main.c     \
          log.c      \
          options.c  \
          clist.c    \
          listener.c \
          getword.c  \
          rc4.c      \
          globals.c
OBJS    := $(SRC:%.c=$(O_PFX)/%.o)


all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS)    $^ -o $@ $(LIBS)
	@echo "[LD] $@"

$(O_PFX)/%.o: %.c $(O_PFX) 
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC] $*.o"

$(O_PFX):
	mkdir $@

clean:
	rm -rf $(TARGET) $(O_PFX) src/*~ src/TAGS src/BROWSE

-include $(O_PFX)/*.d
