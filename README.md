# DD-Command-On-Linux
dd is a command-line utility used to copy data to and from files. Since Linux treats many external devices (including USB drives) as files, this makes dd very powerful. For example, the tool can be used to create a backup image of your hard drive and store it as a file which can be uploaded to cloud storage. dd could also be used to directly clone one drive to another, write a bootable iso image to a USB drive, and much more. implementation will copy data from an input file to an output file in a manner specified by its arguments.\n
-i <file>: input file (defaults to stdin)\n
-o <file>: output file (defaults to stdout)\n
File is created if this file if does not already exist.\n
-b <size>: block size, the number of bytes copied at a time (defaults to 512)\n
-c <count>: total number of blocks copied (defaults to the entire file)\n
-p <count>: number of blocks to skip at the start of the input file (defaults to 0)\n
-k <count>: number of blocks to skip at the start of the output file (defaults to 0)\n
Any other arguments will exit with code 1. getopt will automatically print an error message for you.\n

