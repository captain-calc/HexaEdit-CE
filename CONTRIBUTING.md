# Contributing

## Terminology

In header file documentation:

**"private":** Denotes functions that should only be used within the corresponding C file. These functions should be marked `static`.

**"public":** Denotes functions that can be called from other files.


## Code Style Guide

**Line Length:** Every line of documentation and code should not exceed 80 characters in length.

**File Header:** Every file should have a header block containing the name(s) of the file's author(s), the date the file was last modified, the file's name, and a brief explanation of the file's purpose, in that order.

**Header File Documentation:** Aside from the file header, every header file should have include guards, clearly marked sections, and function documentation. Blocks of defines, "private" function declarations, and "public" function declarations should be clearly delineated.

**Function Documentation:** Every function defined in a C or ASM file should have a corresponding function declaration in a header file with the same name as the source file. The only exception is `main.c`. Each function declaration should be documented. The function documentation should follow this template:

`Description: (A short description of what the function does and any specific advice about using the function)
Pre:          (Preconditions: What should be true before the function executes to ensure a successful execution. This field includes parameter restrictions.)
Post:          (Postconditions: What is true after the function executes (return values, modified parameters, etc.))`

The function declaration should start on the line immediately after the end of the documentation.

**"Private" Function Naming:** "Private" functions should be written in all lowercase letters and in snake case.

**"Public" Function Naming:** "Public" functions should be prefaced with the name of the file that they were declared in. The rest of the function name should be in camel case. For example, a function declared in editor_actions.h that writes a nibble should be named `editoract_WriteNibble()`.

**Comment Quality:** Code comments should be concise and clear. They should also be pertinent to the code that they accompany. Avoid "conversational" comments.

**Code Block Indentation:** All code blocks should be indented with 2 (two) spaces. Do not use tabs because they are rendered differently across text editors and will mess up any formatting.

**Function Spacing:** Each C function in a source file should have two lines between it and any subsequent function. Functions in header files should always have two lines after the end of the function declaration and the start of the documentation for the next function.

## Bug Reports

If you have found a bug, please either raise an issue in HexaEdit's Github repository or make a post on HexaEdit's Cemetech forum thread. Provide as much information as you can in your report, such as what you did immediately before you encountered the bug, how the program responded, and so on. If the program displayed an error message, please include the message in your description. If the bug did not trigger an error message, please mention the lack of notification.

## Pull Requests

Before requesting a pull request merge, please ensure that your code changes follow the code style guide outlined above. Pull requests that do not measure up to HexaEdit's code standards will not be merged.

