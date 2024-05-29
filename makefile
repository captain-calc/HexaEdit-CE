# ----------------------------
# Makefile Options
# ----------------------------

NAME = HEXAEDIT
VERSION = "3.1.3"
ICON = icon.png
DESCRIPTION = "HexaEdit CE v"$(VERSION)
COMPRESSED = YES
COMPRESSED_MODE = zx0
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz -DPROGRAM_VERSION=\"$(VERSION)\"
CXXFLAGS = -Wall -Wextra -Oz -DPROGRAM_VERSION=\"$(VERSION)\"

HAS_PRINTF = NO

# ----------------------------

include $(shell cedev-config --makefile)
