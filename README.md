# Mumblepad2

This project is the spiritual successor to Mumblepad.

- now ported to Linux
- MIT license
- cmake build system
- demo app that encrypts and decrypts files


## Mumblepad Block Cipher

This is a symmetric-key block cipher, with a key size 4096 bytes, 32768 bits.

The main features:
- a relatively large 4096-byte key.
- padding of plaintext blocks with random number bytes, so different encrypted blocks contain same plaintext.
- no requirement of a block cipher mode, thus parallelizable.
- implementation on GPU, in addition to CPU.
- may work with 6 different block sizes, ranging from 128 to 4096 bytes.
- the multi-threaded implementation encrypts or decrypts @ 210MB/s on laptop.

The specification may be found in the doc folder.

Encryption and decryption, runs on either CPU or GPU, may run multi-threaded on CPU.  There are eight rounds, two passes -- diffuse, confuse -- per round. There are six different block sizes (mutually exclusive): 128, 256, 512, 1024, 2048, 4096 bytes.

Runs on GPU with OpenGL or OpenGL ES 2.0; encryption and decryption operations implemented in fragment shaders.

Encrypted blocks containing same plaintext are different, due to small amount of per-block random number padding. Encrypted block also contain a 16-bit length, 16-bit sequence number, and 32-bit checksum.  Resulting plaintext is 87.5% to 97.65% of total bytes, depending on block size.

The multi-threaded implementation can encrypt or decrypt 210MB per second on an HP ZBook 17 (Gen1).




## Future directions
- port back to Windows, using cmake system.
- investigate WebGPU render shaders or compute shaders, running at full GPU speed in a browser
- further optimizations, especially multi-threaded and GPU implementations.

# Building

From the root directory

```
cmake -B build
cd build
make
```

The default build type is Release.  To build debug, run this instead:

    cmake -B build -DCMAKE_BUILD_TYPE=Debug

The binaries are copied into the `out` folder at the project root. You will find:
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

`mpad` is a demo application that encrypts and decrypts files using a given key.


```
Usage of mpad:
   mpad encrypt|decrypt [options]
   Options:
      -i <input-file> : required
      -o <output-file> 
         is optional; if missing, mpad will auto create the output file name
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

Note:  the `gl` and `gl8` engines are not available if the library and utilities are built without the `USE_OPENGL` compile flag.

Examples:

Defaults to block size 128. Auto generates output file name
```
$ out/mpad encrypt -k key.bin -i test.jpg
keyfile: key.bin
infile: test.jpg
using output file test.jpg.mu1
-rw-rw-r-- 1 ader ader 727168 Jul  4 13:02 test.jpg.mu1
```

Using a larger block results is a slighly smaller file because there are less padding bytes per block.
```
$ out/mpad encrypt -k key.bin -i test.jpg -b 2048
keyfile: key.bin
infile: test.jpg
block: 2048
using output file test.jpg.mu5
-rw-rw-r-- 1 ader ader 653312 Jul  4 13:04 test.jpg.mu5
```

Decrypt goes like this:
```
out/mpad decrypt -k key.bin -i test.jpg.mu5
```

You will get the same result regardless of which render engine you use.  You could encrypt with with the gl engine and decrypt with the mt engine -- the resulting decrypted file will still match the original.


# test

The test application runs various block checking scenarios, and well as profiling each of the render engines.  It must be run from the same directory as the testfiles (i.e., the root).  After a successful build, the executable will be located in the `out` directory

To run it:

    out/test







