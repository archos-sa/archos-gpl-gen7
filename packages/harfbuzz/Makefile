ifeq ($(ARCH), arm)
CROSS = arm-linux-
else
CROSS = 
endif

INCS = -I./src
VPATH = ./src

CC = $(CROSS)gcc
LINK = $(CROSS)gcc
CFLAGS += $(INCS) -g
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CMD_CC_O_C =  $(CC) $(CFLAGS) -o $@ -c $<

SRC =  \
	harfbuzz-global.c \
	harfbuzz-buffer.c \
	harfbuzz-stream.c \
	harfbuzz-gdef.c \
	harfbuzz-gpos.c \
	harfbuzz-gsub.c \
	harfbuzz-open.c \
	harfbuzz-shaper.c \
	harfbuzz-arabic.c \
	harfbuzz-oem.c

OTHR_SRC = \
	harfbuzz-hebrew.c \
	harfbuzz-tibetan.c \
	harfbuzz-indic.c \
	harfbuzz-khmer.c \
	harfbuzz-hangul.c \
	harfbuzz-myanmar.c

OBJS = $(patsubst %,$(ARCH)/%,$(SRC:.c=.o))

DEPDIR = .deps

MAKEDEPEND = $(CC) -MM $(CFLAGS) -MT $@ -o $(ARCH)/$(DEPDIR)/$*.dep $<

all: $(TGT_PATH)/libharfbuzz.a

$(ARCH)/%.o : %.c
	@mkdir -p $(ARCH)/$(DEPDIR)
	@$(MAKEDEPEND); 
	$(CMD_CC_O_C)

$(ARCH)/libharfbuzz.a: $(OBJS) Makefile
	$(AR) r $(ARCH)/libharfbuzz.a $(OBJS)

clean:
	@rm -rf $(ARCH)

# include deps depending on target
ifneq (,$(ARCH))
-include $(SRC:%.c=$(ARCH)/$(DEPDIR)/%.dep)
endif
