==============================
| Program: HexaEdit CE
| Version: 2.1.0 CE
| Author: Captain Calc
| Release Date: 06/26/2021
==============================


Installation
------------------------
After extracting the HexaEdit_CE.zip file, send the following file to your calculator using a computer-calculator link program, such as TI-Connect CE or TILP:

* HEXAEDIT.8xp


Controls
-------------------------

  Main Menu Controls:
    [2nd], [Enter]...........Select
    [alpha]..................Activate accelerated scrolling
    Arrow Keys...............[Up] / [Down]:    File select
                             [Left] / [Right]: Change file table
    [y=].....................Open RAM Editor
    [window].................Open ROM Viewer
    [zoom]...................File search (see section below for controls)
    [trace]..................Phrase search range menu
    [mode]...................Open RAM Editor at selected file's VAT pointer
    1 - 4....................Jump to table (Recent Files = 1, Appvars = 2, etc.)

  Main Menu File Search:
    [2nd], [Enter]...........Submit search
    [alpha]..................Switch between uppercase letters, lowercase letters, and numbers
    [del]....................Delete last character
    [clear]..................Exit without submitting search

  Editor Controls:
    [2nd], [Enter]...........Toggle multi-bye selection/Select
    [alpha]..................Activate accelerated scrolling
    [0 - 9] and [A - F]......Write hexadecimal digit
    [y=].....................Open phrase search
    [window].................Insert bytes (only in File Editor)
    [zoom]...................Activate Goto function
    [trace]..................Undo previous action
    [graph]..................Exit editor
    [stat]...................Open image viewer

    When multi-byte selection is on, use the arrow keys to select bytes.

    The image viewer will select the first two bytes at the current cursor location to determine the size of the image. It constructs the image using the data starting at the current cursor postion + 3 and ends at the current cursor postion + 3 + size of image. If the size is too large, the viewer will not display an image. If the image is below a certain size, the image viewer will scale it up.

    Starting in version 2.0.0, the "Undo" action can now undo more than one change. The number of undos that can be made varies, but, if the editor cannot create an undo action for a change, it will not execute the desired change. In order to continue editing the file, you must save your changes and re-open it. This event will not occur until roughly 5000 edits have been made.


Phrase Search
-------------------------
    The phrase search function enables the user to search for either 16-byte ASCII phrases or 8-byte hexadecimal phrases. When the keymap indicator displays a lowercase "x", then each charcter entered will be treated as a hexadecimal nibble (half-byte). When the indicator displays an uppercase or lowercase "a", then each character entered will be treated as an ASCII character (1-byte).

    Other programs can set the phrase search range by writing a custom range to the HexaEdit configuration appvar. The default search range covers the largest memory area on the calculator (ROM, approximately 4,000,000 bytes). The new algorithm introduced in version 2.1.0 takes about 15 seconds to search all ROM ( ~2 seconds to search all RAM).


Headless Start
-------------------------
    "Headless Start" allows the user to make HexaEdit open an editor in RAM, ROM, or a file without opening the main menu first. In order to do this, the user must specify what type of editor he wants and where to open it in a special appvar called HEXAHSCA (HEXAedit Headless Start Configuration Appvar). The configuration data's format is one byte (general configuration) followed by one or two sets of configuration data, as shown below.

 * General Configuration
 * ===============================
 * Color Theme/Editor Type  1
 *
 *
 * Color Theme Override (7 bytes)
 * ===============================
 * Background Color           1
 * Bar Color                  1
 * Bar Text Color             1
 * Table BG Color             1
 * Table Text Color           1
 * Selected Table Text Color  1
 * Cursor Color               1
 * ===============================
 *
 *
 * RAM Editor/ROM Viewer (6 bytes)
 * ===============================
 * Primary Cursor Offset	3
 * Secondary Cursor Offset	3
 * ===============================
 *
 *
 * File Editor (17 bytes)
 * ===============================
 * File Name                10
 * File Type                1
 * Primary Cursor Offset	3
 * Secondary Cursor Offset	3
 * ===============================
 *
 *
 * Configuration Data Notes
 * =================================
 *
 * The Color Theme/Editor Type byte looks like this:
 *
 * 0000 0000
 * ^      ^
 * |      |
 * |      * The two least significant bytes specify the editor type (File = 0, RAM = 1, ROM = 2)
 * |
 * * The most significant byte should be set to specify a color override. It should be set to 0 if
 *   you do not want to change the color scheme.
 *
 * If you want to override the color scheme and open a file editor, for example, the byte would look
 * like: 1000 0010.
 *
 *
 * You may notice that the values cursor position in the editor are offsets instead of memory pointers.
 * This is because HexaEdit does not edit the specified file directly but, rather, a copy of it.
 * HexaEdit does not create this copy until after it reads out the configuration data and creates the
 * necessary memory pointers out of the file offsets.

The general order in which the data should be written is as follows:
  General Configuration Byte
  Color Theme Override Structure (optional)
  File Editor or RAM Editor or ROM Viewer


Bug Reports
-------------------------
If you find a bug in HexaEdit, please make a post in HexaEdit's forum thread on www.cemetech.net, PM me at my profile page on Cemetech, or open an issue in the Github repository for this program.

https://cemetech.net/forum


Source Code
--------------------------
The source code for HexaEdit CE can be found here:
https://github.com/Captain-Calc/HexaEdit-CE


Change Log
--------------------------
v2.1.0 - Replaced phrase search C algorithm with faster assembly algorithm. Removed
         user-specified phrase search range because the faster algorithm rendered it
         superfluous. Added [mode] key functionality to the main menu. Fixed main menu
         protected program bug.

v2.0.2 - Simplified editor API and Headless Start configuration.

v2.0.1 - Fixed Headless Start bug. Added phrase search to editor. Modified input functions.

v2.0.0 - Complete rewrite; Added Headless Start and expanded file search range to cover
         all file types. "Undo" action now works for more than one change. Reworked GUI.
	 
v1.2.1 - Added [Enter] key support for selection and submitting actions; Upgraded the sprite
         viewer to handle any sprite size
	 
v1.2.0 - Fixed "Goto" and decimal translation bugs; Added alpha-scrolling to main menu and
         editor; When the selected nibble is the second part of the byte, pressing the left
         arrow now relocates the selected nibble instead of moving the cursor; Fixed perpetual
         AM clock; Added lowercase letters to file search routine
	 
v1.1.0 - Fixed main menu scrolling, key fall-throughs, and battery status indicator;
         Added file search, selective cursor delay, and recent files deletion
	 
v1.0.0 - First release
