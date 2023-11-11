# bFLT-objdump
Disassembler for bFLT (binary flat) files.
Prints headers, and decompresses compressed sections.

### Dependencies

- Zlib 

### References:
- https://www.halolinux.us/embedded-development/bflt-file-format.html
- https://github.com/torvalds/linux/blob/master/fs/binfmt\_flat.c

## Building
```
mkdir build
cd build
cmake ..
make
```
## Install
```
sudo make install
```
## Usage
```
./bflt-objdump [-h] [-r] [-d] [-o output_file] input_file
   -h - Print header
   -r - Print reloc entries
   -d - Print instructions
   -D - Print instructions and data
```
