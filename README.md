# Mumblepad2

The project is the spiritual successor to Mumblepad.

- now ported to Linux
- MIT license
- cmake build system
- crc32 for checksum
- demo app that encrypts and decrypts files

# Building

From the root directory

```
cmake -B build
cd build
make
```

The default build type is Release.  To build debug, run this instead:

    cmake -B build -DCMAKE_BUILD_TYPE=Debug

The binaries are copied into the out folder at the project root. You will find:
- libmumblepad.a
- mpad
- test

mpad and test are both executables, and are described below.

## OpenGl
To build with OpenGL, you will need the standard development libraries along with GLFW

    apt-get install libglfw3-dev

And then

    cmake -B build -DUSE_OPENGL=On

# mpad

This is a demo application that encrypts and decrypts files using a given key.

And the moment

Usage
```
Usage of mpad:
   mpad encrypt|decrypt [options]
   Options:
      -i <input-file> : required
      -o <output-file> 
         is optional; if missing, and preferred, mpad will auto create file name
         by appending mu1|mu2|mu3|mu4|mu5|mu6 extension to input file name
      -k <key-file> : required
      -e <engine-type> :: [ cpu | mt | gl | gl8 ]
         single-threaded, multi-threaded, OpenGL single stage, OpenGL with 8 stages
         is optional, default is cpu
      -b <block-size>  : [ 128 | 256 | 512 | 1024 | 2048 | 4096 ]
         this is the block size
         is optional for encrypt & decrypt, default is 128
         if decrypt input file name has extension mu1|mu2|mu3|mu4|mu5|mu6, 
         the extension will determine the block size used for decryption.
         gl8 only uses blocks of size 4096, and will override a block 
         size in the command line

```

Examples:

Defaults to block size 128. Auto generates output file name
```
$ mpad/mpad encrypt -k key.bin -i test.jpg
keyfile: key.bin
infile: test.jpg
using output file test.jpg.mu1
-rw-rw-r-- 1 ader ader 727168 Jul  4 13:02 test.jpg.mu1
```

Using a larger block results is a slighly smaller file becuase there are less padding bytes per block.
```
$ mpad/mpad encrypt -k key.bin -i test.jpg -b 2048
keyfile: key.bin
infile: test.jpg
block: 2048
using output file test.jpg.mu5
-rw-rw-r-- 1 ader ader 653312 Jul  4 13:04 test.jpg.mu5
```

Decrypt goes like this
```
mpad/mpad decrypt -k key.bin -i test.jpg.mu5
```

You will get the same result regardless of engine uses.  You could encrypt with with the gl engine and decrypt with the mt engine -- the resulting decrypted file will still match the original.

```
$ mpad/mpad encrypt -k key.bin -i test.jpg -b 1024 -e gl
```


# test

The test application runs various block checking scenarios, and well as profiling each of the render engines.  It must be run from the same directory as the testfiles (i.e., the root).  After successfully building, the executable will be located in the out directory

To run it:

    out/test







