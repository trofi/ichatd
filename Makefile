TARGET  = ichatd

# features:
# LINUX_PPOLL (unimpl ;])

CFLAGS  += -MMD -W -Wall -Werror -g -O0 -Isrc -fmudflap # -DLINUX_PPOLL
LIBS    += -lmudflap

SRC     := $(wildcard src/*.c src/*/*.c src/*/*/*.c)
HDR     := $(wildcard src/*.h src/*/*.h src/*/*/*.h)

OBJS    := $(SRC:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "[LD] $@"

obj/%.o: src/%.c
	@mkdir -p $$(dirname $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC] $@"

clean:
	rm -rf $(TARGET) src/*~ src/*/*~ src/*/*/*~ obj/* TAGS BROWSE

TAGS: $(SRC) $(HDR)
	etags $^

run: $(TARGET)
	./$(TARGET)

-include obj/*.d obj/*/*.d obj/*/*/*.d
