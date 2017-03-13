
<h1 align='center'>SC4-HSM User Manual</h1>

-> Spark Innovations Inc. <-

-> Version 0.3 <-

![SC4-HSM](sc4-hsm1.jpg =640x)

## System description

The SC4-HSM is a prototype USB Hardware Secure Module based on the [STM32F405RG][1] microprocessor.  The features of this processor include:

+ 1 MB flash memory
+ 192 kB SRAM
+ A hardware random-number generator (HWRNG)
+ Hardware memory readout protection (RDP)

In addition, the SC4-HSM features a built-in 128x32 pixel monochrome OLED display, a tri-color LED, and two user pushbuttons.  It connects to a host processor via a USB2 port.

The HWRNG and RDP features make the SC4-HSM suitable as a secure key generation and storage device, which is its intended purpose.  However, it is a fully general-purpose user-programmable peripheral.  All of the prototype software currently running on it is open-source.  This reflects our belief that the only way to build a secure system is for every component, including the software, to be fully open to inspection.

Software that has been successfully run on the SC4-HSM to date includes

* [TweetNaCl](https://tweetnacl.cr.yp.to), a cryptographic library by Dan Bernstein
* [MicroPython](https://micropython.org)
* [TinyScheme](http://tinyscheme.sourceforge.net/home.html)

[1]: http://www.st.com/content/st_com/en/products/microcontrollers/stm32-32-bit-arm-cortex-mcus/stm32f4-series/stm32f405-415/stm32f405rg.html

## Getting started

The SC4-HSM comes pre-programmed with demo software.  The hardware presents itself as a USB Serial device, so all you need to communicate with it is a terminal emulator capable of connecting to such a device.  The SC4-HSM has been tested on Linux and OS X.  It has not been tested on Microsoft Windows, but there is no reason why that shouldn't work.

To run the pre-flashed demo, simply plug the unit into a USB port and connect to it using a terminal emulator.  The details of how to do this depend on your operating system.  On Linux, the device will show up as /dev/ttyACM0.  On a Mac, it will show up as /dev/cu.usbmodem80171 (the number may be different).  On Windows it will show up as a USB serial device named "STM32 Virtual Com Port."

The SC4-HSM software comes with a little terminal emulator called "term" which you can use to communicate with the SC4-HSM.  The source code is in the tools directory.  To build it, just cd to the tools directory and type 'make'.

Note that on Linux the SC4-HSM by default is accessible only by the root user, so you will need to use [sudo](https://en.wikipedia.org/wiki/Sudo) or [setuid](https://en.wikipedia.org/wiki/Setuid) to obtain the necessary privileges.

### New - FIDO U2F token

The SC4-HSM can function as a FIDO-U2F token.  To enable this functionality, simply plug the unit in and press either of the two user buttons.  To return to the normal demo mode (where you can access the device through a virtual comm port) press the button again.

## Programming the SC4-HSM

The SC4-HSM comes with demo firmware pre-loaded into the flash.  But the HSM is designed for you to build and upload firmware yourself.  The best place to get the latest firmware is our [git repository](https://github.com/Spark-Innovations/sc4-hsm) but you can also download a tarball [here](sc4-hsm.tgz).  The git repo will generally be more up to date than the tarball, but the tarball includes a pre-built binary that you can use without having to install the gnu-arm compiler toolchain (see the section entitled "Building the firmware" below).

In order to load new firmware into the SC4-HSM you will need a copy of [dfu-util](http://dfu-util.sourceforge.net).

To load new firmware you will need to get the SC4-HSM in to DFU (Device Firmware Update) mode.  To do this, hold down the DFU button on the bottom of the board (see figure 1) while resetting the device, either by pressing the RESET button at the top of the board, or by plugging the device into a USB port while holding down the DFU button.

![SC4-HSM](sc4-hsm2.jpg =640x)

-> Figure 1: The SC4-HSM with the Reset, DFU and user button locations labelled <-

If you have successfully put the device in DFU mode, the display will be blank.

Note that you can gain physical access to the reset and DFU buttons either by opening the case, or by using a small object like a paperclip (I use a thumb tack with the tip cut off with a pair of wire cutters) inserted through the small access holes in the case.  The case is made of 3-D printed plastic, and is designed to come apart easily.  The case snaps together, but it is a pretty loose fit so use caution when opening and closing it.

Once the unit is in DFU mode, all you need to do to upload the firmware is to run the "hsm" script in the "tools" directory.

Once the firmware is uploaded, run it by pressing the Reset button again (without holding down the DFU button, obviously) or unplug the device and plug it back in again.

Note that the "hsm" script will automatically detect if the HSM is in DFU mode.  If it is, it will upload the latest firmware.  If it is not, it will run the "term" program.  So in general all you need to do to interact with the HSM in whatever mode it is in it to run the "hsm" script.

## Building the firmware

In order to build the firmware for the SC4-HSM you will need the [Gnu ARM Embedded tool chain](https://launchpad.net/gcc-arm-embedded).  You can also use the [Gnu ARM Eclipse IDE](http://gnuarmeclipse.github.io), but I personally don't recommend this.  IMHO it adds a lot of overhead for not a lot of value, but reasonable people can (and do) disagree.

If you are running on a unix system then to build the demo software:

1.  Edit the Makefile to change the CROSS_COMPILE variable points to your gcc-arm installation

2.  make

3.  Upload the new firmware by following the instructions in the previous section.

That's it!

# Demo code

The demo firmware includes a copy of TweetNaCl and a very (very!) minimal UI.  There are also several "fun" demos to showcase the display and some of the other capabilities of the SC4-HSM.  Just for fun, I even threw in a copy of TinyScheme.  To explore the demo code, connect to the SC4-HSM using a terminal emulator (see the "Getting started" section above) and type "?" followed by a carriage return to get a list of commands.

*NEW* There is now a [FIDO U2F](https://en.wikipedia.org/wiki/Universal_2nd_Factor) functionality for the SC4-HSM.  The code for this lives in the git repository in the u2f branch.  To access it, just check out the u2f branch, run "make", and then upload the resulting firmware.  Press the "reset" button and then either one of the user buttons to get into U2F mode.  (We are working on getting the device to present itself as a virtual com port and a U2F HID device at the same time, but that is not yet working.)

## Cryptgraphy and security

The SC4-HSM is designed to be a secure cryptographic device.  As such, the generation, secure storage, and secure use of cryptographic keys is of paramount importance.  At the moment, the SC4-HSM only supports the TweetNaCl library, but in principle it could run any cryptographic code, subject to the constraints of the on-board storage.  In particular, TweetNaCl only supports Curve25519 and not RSA for public key encryption.  It also uses Salsa20 for symmetric encryption rather than AES.  The STM32F415 processor on the SC4-HSM has built-in hardware support for AES, but this is not supported by the demo firmware.

## Key generation

Cryptographic keys are generated using the STM32F415's built-in hardware random number generator (HWRNG).  Entropy is collected and whitened by running the output of the HWRNG through the SHA512 hash function with a 4:1 margin of safety, i.e. the whitener extracts one bit of output entropy for every four bits of input.  This is probably severe overkill, and it was done mainly to simplify the code: the whitener operates on 32-word input blocks from the HWRNG and compresses them to 32-byte output blocks.

Keys are stored in a 16kB block of flash storage set aside for this purpose at address 0x80080000.  For the demo code only ten keys are supported, and only one key is active at any one time.  This is mainly because I have not yet gotten around to writing code for securely parsing complex user input, so all of the commands use a single input character to specify a key.

The SC4-HSM uses a key convention originally introduced for the [SC4 secure communications application](https://github.com/Spark-Innovations/SC4).  It uses a 32-byte random seed to derive a single ECC key that can be used securely for both encryption and signing.  If you want to know more about how that works, let me know and I'll write it up in more detail.

## Memory readout protection

When the SC4-HSM is in DFU mode, its memory can be read from as well as written to.  Because keys are stored in ordinary flash, this means that an attacker with physical access to the device can read your keys.

There are two possible mitigations for this.  The first is to encrypt keys with a pass phrase.  This is not currently implemented, but it is not difficult.  A future version of the demo code will include this feature.  (Note, however, that because the pass phrase must be entered on the host device, this is not necessarily secure either.   However, it isn't useless either because an attacker would need to both compromise your pass phrase *and* obtain physical possession of the device.)

The second mitigation is that the STM32F415 has a hardware memory readout protection (RDP) feature which can disable access to the memory in DFU mode.  There are three levels of readout protection:

RDP Level 0 is the default level in which memory can be read in DFU mode.

RDP Level 1 is a reversible protection level that does the following:

1. Prevents memory from being read in DFU mode
2. Fully erases the flash on any attempt to write to it
3. Fully erases the flash when the RDP level is set back to 0

RDP Level 2 is a permanent protection that disables DFU mode entirely.  It also disables support for hardware debugging.  RDP level 2 is by far the most secure.  It will protect your keys against all but the most sophisticated adversaries.  Getting around RDP level 2 requires decapping the chip.  However, it is PERMANENT.  Once you enter RDP level 2 you can never go back, which means you cannot load new code.  Because of this, RDP level 2 is not directly supported by the development firmware.  If you want to invoke RDP level 2 you will need to modify the firmware to do this.  It isn't difficult.  If you can't figure out how to do it, you shouldn't be doing it.

To enable RDP level 1, simply connect to the HSM and type R.

To disable RDP level 1, do the following:

1. Put device in DFU mode by holding down the DFU button and pressing RESET

2. Type: dfu-util -s 0:force:unprotect -a 0 -D path-to/src/build/firmware.dfu

3. Wait 1 minute while the chip performs a full flash erase.

4. Put device in DFU mode again as in step 1

5. Reload the firmware according to the instructions above in the section entitled "Programming the SC4-HSM" (i.e. type: dfu-util -a 0 -D build/firmware.dfu)

Note that in step 3, dfu-util  will return *before* the flash erase is complete.  There is no feedback on when the flash erase is complete.  You just have to time it.  If you stop the process before it finishes, then attempts to load the firmware will fail with the message "ERASE_PAGE not correctly executed".  If that happens, just repeat the above procedure and give it a little more time.

NOTE: Readout protection will prevent an adversary from *reading* your keys if you lose physical possession of the device, but it will (obviously) not prevent them from *using* your keys unless they are protected by a pass-phrase.  This feature will be released in a future version of the firmware.

## A tour of the source code

One of the most important considerations for any secure device is the provenance of the code that is running on it.  It is distressingly easy for vulnerabilities to hide even in open code, and the more code there is, the more places there are for things to go wrong.  Accordingly, we have made a significant effort to distill the code running on the SC4-HSM down to the bare essentials, and to document its provenance.

The code is divided up into five subdirectories according to where it came from and whether or not it has been modified from its original source.

The code is subject to various licensing agreements depending on its source, but it is all under some variation of an open-source license.  Some of the code (like TweetNaCl) is in the public domain.  If you need detailed information about code licensing please [contact us](mailto:support@spark-innovations.com).

### /stm

The /stm directory contains unmodified code provided by [STMicroelectronics](http://www.st.com), the manufacturer of the STM32F415.  The code in this directory was extracted from the [STM32Cube HAL][1] (Hardware Abstraction Layer) library.  The original library as provided by STMicroelectronics covers the entire family of STM32 processors, so for any given application it consists of mostly dead code (to the tune of about 1.3 GB!)  The extracted code is 5 MB.

[1]: http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-embedded-software/stm32cubef4.html

### /lib

The /lib directory contains hardware device drivers from vendors other than STMicroelectronics, or code that was originally provided by STMicroelectronics but modified by Spark Innovations for the SC4-HSM.  In particular, this directory contains:

+ Drivers from [Adafruit](https://www.adafruit.com) for the OLED display, plus some miscellaneous display utilities
+ Drivers for the USB port
+ Configuration files and drivers for the SC4-HSM hardware (e.g. pin assignments, etc.)

### /stubs

The /stubs directory contains stubs for certain system calls that are needed by other components of the software.  The code in this directory was adapted from the [newlib](https://sourceware.org/newlib/) embedded library.

### /tinyscheme

This is simply a copy of [TinyScheme](http://tinyscheme.sourceforge.net/home.html) modified to run on the SC4-HSM.  Note that at the moment this is bit of a parlor trick than a useful feature.  Getting TinyScheme to run required quite a bit of hacking and slashing, and a lot of useful functionality was eliminated in the process.  In particular, the SC4-HSM doesn't have an operating system, so it doesn't have a file system, so nothing that requires opening a file will work.  This got to be particularly challenging because the way the TinyScheme REPL works is by loading a pseudo-file from stdin.  If you look at the code, you will find it chock-full of Horrible Hacks.  The current version of this code is not recommended for mission-critical applications.

### /sc4

The /sc4 directory contains original code written specifically for the SC4-HSM by Spark Innovations.  The TweetNaCl library lives here too for historical reasons even though it was obviously written by a third party.

