# ----------------------------
# Makefile Options
# ----------------------------

NAME = HEXAEDIT
ICON = icon.png
DESCRIPTION = "HexaEdit CE v3.1.1"
COMPRESSED = YES
COMPRESSED_MODE = zx0
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz -DPROGRAM_VERSION=\"3.1.1\"
CXXFLAGS = -Wall -Wextra -Oz -DPROGRAM_VERSION=\"3.1.1\"

HAS_PRINTF = NO

# ----------------------------

include $(shell cedev-config --makefile)
