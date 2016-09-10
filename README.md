Text Source using Pango for OBS Studio
======================================

This plugin provides a text source for OBS Studio. The text is layed out
and rendered using Pango.

Current Features
----------------

* Text alignment
* Text color
  * Colors for top and bottom of the text to create gradients
* Outline
  * Configurable width of the outline
  * Configurable color of the outline
* Drop Shadow
  * Configurable offset of the outline from the text
  * Configurable color for the drop shadow
* Custom text width
  * Word wrapping

Planned Features
----------------

* Reading the text from a file
* Chat log mode (Last 6 lines from file)

Build
-----

You can either build the plugin as a standalone project or integrate it
into the build of OBS Studio.

Building it as a standalone project follows the standard cmake approach.
Create a new directory and run `cmake ... <path_to_source>` for whichever
build system you use. You may have to set the `CMAKE_INCLUDE_PATH`
environment variable to the location of libobs's header files if cmake
does not find them automatically. You may also have to set the
`CMAKE_LIBRARY_PATH` environment variable to the location of the libobs
binary if cmake does not find it automatically.

To integrate the plugin into the OBS Studio build put the source into a
subdirectory of the `plugins` folder of OBS Studio and add it to the
CMakeLists.txt.