O      ?= .
TARGET  = $(O)/ichatd

# features:
# mudflap=1
# debug=1
# noopt=1
#
# example: make debug=1 mudflap=1

CFLAGS  += -MMD -W -Wall -Werror -Isrc -Wformat

ifdef mudflap
    CFLAGS  += -fmudflap
    LDFLAGS += -lmudflap
endif

ifdef debug
    CFLAGS   += -g
    LDFLAGS  += -g
else
    CFLAGS   += -s
    LDFLAGS  += -s
endif

ifdef noopt
    CFLAGS   += -O0
    LDFLAGS  += -O0
else
    CFLAGS   += -O2
    LDFLAGS  += -O2
endif

####################################################

SRC     := $(wildcard src/*.c src/*/*.c src/*/*/*.c)
HDR     := $(wildcard src/*.h src/*/*.h src/*/*/*.h)

OBJS    := $(SRC:%.c=$(O)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[LD] $@"

$(O)/%.o: %.c
	@mkdir -p $$(dirname $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC] $@"

$(O)/%.C: %.c
	@mkdir -p $$(dirname $@)
	@$(CC) $(CFLAGS) -E $< -o $@
	@echo "[CC] $@"

clean:
	rm -rf $(TARGET) src/*~ src/*/*~ src/*/*/*~ $(OBJS) $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d TAGS BROWSE

TAGS: $(SRC) $(HDR)
	etags $^

run: $(TARGET)
	./$(TARGET)

-include $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d
