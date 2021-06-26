# HexaEdit CE

HexaEdit CE is a on-calc hex editor for the TI-84 Plus CE.It can edit any kind of program or appvar as well as the calculator's RAM. It also provides a wide array of features, such as byte insertion and deletion, hexadecimal-to-decimal translation, and much more.

![screenshot](https://u.cubeupload.com/torontobay/v200.png)


To find out more about HexaEdit CE, please visit its Cemetech forum thread:

https://www.cemetech.net/forum/viewtopic.php?t=16759

## Branch "improve-core" Information

Updates to this branch will focus on improving the abilities and struture of the core editing functions. This is a developmental branch, so code stored here is **not** guaranteed to be stable.

## Installation

After extracting the HexaEdit_CE.zip file, send the following file to your calculator using a computer-calculator link program, such as TI-Connect CE or TILP:

* HEXAEDIT.8xp

**Important**

HexaEdit CE uses the C libraries from the LLVM toolchain. If you encounter a version error when you run HexaEdit, please send the libraries included with the Cemetech download to your calculator. Your other programs will still work with these libraries.


## Technical Info

* Platform: TI-84 Plus CE
* Language: C
* Latest Version: 2.1.0

## Change Log
| Release Number        | Git Branch   | Description
| --------------------- |:------------:| -----------
| v2.1.0                | improve-core | Replaced phrase search C algorithm with faster assembly algorithm. Removed user-specified phrase search range because the faster algorithm rendered it superfluous. Added [mode] key functionality to the main menu. Fixed main menu protected program bug.
| v2.0.2                | improve-core | Simplified editor API and Headless Start configuration.
| v2.0.1 *(unreleased)* | master       | Fixed Headless Start bug. Added phrase search to editor. Modified input functions.
| v2.0.0                | master       | Complete rewrite; Added Headless Start. "Undo" action now works for more than one change. Reworked GUI. Increased file search range to all file types.
| v1.2.1                | master       | Added [Enter] key support for selection and submitting actions; Upgraded the sprite viewer to handle any sprite size
| v1.2.0                | master       | Fixed "Goto" and decimal translation bugs; Added alpha-scrolling to main menu and editor; When the selected nibble is the second part of the byte, pressing the left arrow now relocates the selected nibble instead of moving the cursor; Fixed perpetual AM clock; Added lowercase letters to file search routine
| v1.1.0                | master       | Fixed main menu scrolling, key fall-throughs, and battery status indicator; Added file search, selective cursor delay, and recent files deletion
| v1.0.0                | master       | First release
