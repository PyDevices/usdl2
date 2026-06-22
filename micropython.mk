# MicroPython build glue for usdl2 (unix-only SDL2 subset).
#
# Discovered via USER_C_MODULES pointing at the cmods workspace root.
# Requires libsdl2-dev (pkg-config sdl2). Only builds on the unix port.

USDL2_MOD_DIR := $(USERMOD_DIR)

IS_UNIX_PORT := $(findstring /ports/unix,$(abspath $(CURDIR)))
ifneq ($(IS_UNIX_PORT),)

SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)

ifeq ($(SDL2_CFLAGS),)
$(error pkg-config sdl2 failed — install libsdl2-dev)
endif

CFLAGS_USERMOD += $(SDL2_CFLAGS) -I$(USDL2_MOD_DIR) -Wno-sign-compare -Wno-unused-parameter -Wno-shadow
LDFLAGS_USERMOD += $(SDL2_LIBS)

SRC_USERMOD_C += \
    $(USDL2_MOD_DIR)/modusdl2.c \
    $(USDL2_MOD_DIR)/usdl2_wrappers.c \
    $(USDL2_MOD_DIR)/usdl2_event.c

endif
