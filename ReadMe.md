### HexaEdit CE

HexaEdit CE is a on-calc hex editor for the TI-84 Plus CE.It can edit any kind of program or appvar as well as the calculator's RAM. It also provides a wide array of features, such as byte insertion and deletion, hexadecimal-to-decimal translation, and much more.

![screenshot](http://www.cubeupload.com/im/torontobay/v120apng.png)


To find out more about HexaEdit CE, please visit its Cemetech forum thread:

https://www.cemetech.net/forum/viewtopic.php?t=16759

#### Installation

After extracting the HexaEdit_CE.zip file, send the following file to your calculator using a computer-calculator link program, such as TI-Connect CE or TILP:

* HEXAEDIT.8xp

#### Building

As of July 4, 2020, this repository does not include two files, namely, *easter_eggs.h* and *easter_eggs.c*. These files will be added to the repository by July 18, 2020. In their place are two dummy files that allow the program to be built, but without the easter eggs.

#### Technical Info

* Platform: TI-84 Plus CE
* Language: C
* Latest Version: 1.2.0

#### Change Log

v1.2.0 - Fixed "Goto" and decimal translation bugs; Added alpha-scrolling to main menu and
         editor; When the selected nibble is the second part of the byte, pressing the left
         arrow now relocates the selected nibble instead of moving the cursor; Fixed perpetual
         AM clock; Added lowercase letters to file search routine
v1.1.0 - Fixed main menu scrolling, key fall-throughs, and battery status indicator;
         Added file search, selective cursor delay, and recent files deletion
v1.0.0 - First release