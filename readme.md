# Cacodemon345's UEFI-DOOM (forked from warfish's DOOM repo)

A port of DOOM to UEFI systems.
Tested with: QEMU with OVMF.

# Building
Prerequisites:
1. EDK II: https://github.com/tianocore/edk2
2. Visual Studio 2015 toolset
3. EDK II LibC (EADK): https://github.com/tianocore/edk2-libc

Instructions:
1. Install the Visual Studio 2015 toolset.
2. Clone the EDK II and EDK II LibC repo.
3. Follow the instuctions for setting up the workspace. You also can just drop everything from the EDK II LibC repo to the main EDK II workspace.
4. Edit the AppPkg.dsc; add the following line at the end of [LibraryClasses] list:
	`SerialPortLib|PcAtChipsetPkg/Library/SerialIoLib/SerialIoLib.inf`
   Also add the path to the inf file of this project in the [Components] list like this:
	`AppPkg/Applications/uefidoom/doom.inf`
5. Edit the StdLib.inc file inside StdLib directory; change these lines:
	```MSFT:*_VS2015_*_CC_FLAGS          = /Wv:11
	   MSFT:*_VS2015x86_*_CC_FLAGS       = /Wv:11
	   MSFT:*_VS2015xASL_*_CC_FLAGS      = /Wv:11
	   MSFT:*_VS2015x86xASL_*_CC_FLAGS   = /Wv:11```
into:
	```MSFT:*_VS2015_*_CC_FLAGS          = /Wv:11 /W0
   	   MSFT:*_VS2015x86_*_CC_FLAGS       = /Wv:11 /W0
   	   MSFT:*_VS2015xASL_*_CC_FLAGS      = /Wv:11 /W0
           MSFT:*_VS2015x86xASL_*_CC_FLAGS   = /Wv:11 /W0```
    This will disable the warnings (don't worry, the project will build and run).

6. Move the "uefidoom" folder from this project inside Applications.
7. Download the AudioDxe source code from http://github.com/Goldfish64/AudioPkg. Disable the warnings there too.
8. Open a command prompt, type `edksetup.bat` (assuming you set up the environment properly) and then type `build -t VS2015 -b DEBUG -a X64`
9. If everything goes well, look for a file called "doom.efi" inside this path: 
`path\to\edk2\Build\AppPkg\DEBUG_VS2015\X64\`
10. Now just copy `doom.wad` (the Registered DOOM 1 1.9 IWAD file) into it.
This should make sure everything is ready for playing.
# Running:
Make sure you have a UEFI environment in your real hardware. Also, make sure the executable resides inside a FAT32 partition and that the DOOM 1 IWAD file resides in the same directory as the executable.
You will also need UEFI command-line shell for that (binaries are available online).
Steps:
1. Build the UEFI Shell package (ShellPkg) using `build -t VS2015 -a X64 -p ShellPkg/ShellPkg.dsc -b DEBUG` (run edksetup.bat before that, once again assuming the environment is properly set). If everything goes well, look for `Shell.efi` inside `path\to\edk2\Build\DEBUG_VS2015\X64\ShellPkg\Application\Shell\Shell\OUTPUT`
Alternative: Look for a EFI Shell binary online.
2. Copy the shell binary into a FAT32-formatted USB key; make a folder named EFI, make another folder inside it named BOOT,  and copy the shell binary into it.
3. Rename it into `BOOTX64.efi`
4. Boot your UEFI computer into the UEFI Shell environment.
5. Change to the directory where the "doom.efi" exec is stored (change the current filesystem drive to the one that contains the exec e.g `FS0:`)
6. Type "doom" to start DOOM.
# Bugs:
1. The "numsprites" variable in the sprite loading function will be decreased by 3 to allow it to load the sprites.
2. ~~Game is very slow.~~
3. ~~TICRATE constant remains 65 from the original DOOM UEFI port by warfish.~~
4. ~~In-game input doesn't work?~~
# Planned:
1. Audio support (using GoldFish64's AudioDxe driver).
Help is accepted! Please let me know of any problems. Thanks.
