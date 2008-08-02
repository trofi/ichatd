O      ?= .
TARGET  = $(O)/ichatd

# features:
# mudflap=1
# debug=1
# noopt=1
#
# example: make debug=1 mudflap=1

CFLAGS  += -MMD -W -Wall -Werror -Isrc -Wformat

# some gcc's do not support all the features
# TODO: make such flags triggered by an option
UNUSED_CFLAGS := -Wc++-compat

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
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(O)/%.o: %.c
	@mkdir -p $$(dirname $@)
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.C: %.c
	@mkdir -p $$(dirname $@)
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -E $< -o $@

clean:
	rm -rf $(TARGET) src/*~ src/*/*~ src/*/*/*~ $(OBJS) $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d $(O)/*/*/*/*.d TAGS BROWSE

TAGS: $(SRC) $(HDR)
	@echo "[TAGS]"
	@etags $^

run: $(TARGET)
	$(TARGET)

-include $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d
