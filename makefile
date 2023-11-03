# ----------------------------
# Makefile Options
# ----------------------------

NAME = HEXAEDIT
ICON = icon.png
DESCRIPTION = "HexaEdit CE v3"
COMPRESSED = YES
COMPRESSED_MODE = zx0
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

HAS_PRINTF = NO

# ----------------------------

include $(shell cedev-config --makefile)
