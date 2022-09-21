This quick-and-dirity tool was created to assist me in generating Gcode for creating PCBs using my 3D printer. 

The tool is used in combination with EAGLE (9.7v) and FlatCAM (8.5v) software. It is compatible with Ubuntu 20 and Windows 7.

## Workflow

1. Create and design your PCB in EAGLE.
   (Make sure to set the ground plane polygom to layer tTest or similar before exporting CAM to prevent it from being etched.)
2. Generate gerber and excellon files using the CAM processer in EAGLE. Note: disable 'cutouts' in the profile section.
3. Open this tool and choose your project.
4. Generate the commands for FlatCAM.
5. Paste these commands in the console of FlatCAM (these commands are automatically copied to your clipboard).
6. Hit Enter and let FlatCAM do its thing.
7. Now modify the Gcode output using this tool.
8. Your Gcode files are now ready to use!

## Build

1. Copy and rename `local_settings.h.example` to `local_settings.h` and uncomment/edit the values if needed.

### Linux

1. Run `make`.

### Windows

1. Install Cygwin and make sure it's in your path.
2. Run `make_script.cmd` file.