# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        ?= HEXAEDIT
COMPRESSED  ?= NO
ICON        ?= icon.png
DESCRIPTION ?= "HexaEdit CE"

# ----------------------------
# Other Options (Advanced)
# ----------------------------


ARCHIVED            ?= YES
SRCDIR              ?= src
OBJDIR              ?= obj
BINDIR              ?= bin

include $(CEDEV)/include/.makefile