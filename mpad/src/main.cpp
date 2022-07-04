//
// MIT License
//
// Copyright (c) 2022 Kyle Granger
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "assert.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mumpublic.h>


typedef struct TJob {
    bool doEncrypt;
    std::string infile;
    std::string outfile;
    std::string keyfile;
    EMumEngineType engineType;
    EMumBlockType blockType;
    uint32_t numThreads;
} TJob;

void printUsage() {
    printf("\nUsage of mpad:\n");
    printf("   mpad encrypt|decrypt [options]\n");
    printf("   Options:\n");
    printf("      -i <input-file> : required\n");
    printf("      -o <output-file> \n");
    printf("         is optional; if missing, and preferred, mpad will auto create file name\n");
    printf("         encrypt: appending mu1|mu2|mu3|mu4|mu5|mu6 extension to input file name\n");
    printf("         decrypt: strip the  mu1|mu2|mu3|mu4|mu5|mu6 extension from input file\n");
    printf("         (encrypted name), to derive original file name\n");
    printf("      -k <key-file> : required\n");
#ifdef USE_OPENGL
    printf("      -e <engine-type> :: [ cpu | mt | gl | gl8 ]\n");
    printf("         single-threaded, multi-threaded, OpenGL single stage, OpenGL with 8 stages\n");
#else
    printf("      -e <engine-type> :: [ cpu | mt ]\n");
#endif
    printf("         is optional, default is cpu\n");
    printf("      -b <block-size>  : [ 128 | 256 | 512 | 1024 | 2048 | 4096 ]\n");
    printf("         this is the block size\n");
    printf("         is optional for encrypt & decrypt, default is 128\n");
    printf("         if decrypt input file name has extension mu1|mu2|mu3|mu4|mu5|mu6, \n");
    printf("         the extension will determine the block size used for decryption.\n");
#ifdef USE_OPENGL
    printf("         gl8 only uses blocks of size 4096, and will override a block \n");
    printf("         size in the command line\n\n");
#endif
}

bool parseCommandLine(int argc, char *argv[], TJob &job) {
    if (argc < 2 || (argc%2 == 1)) {
        return false;
    }
    std::string operation = argv[1];
    if (operation.compare("encrypt") == 0) {
        job.doEncrypt = true;
    } else if (operation.compare("decrypt") == 0) {
        job.doEncrypt = false;
    } else {
        printf("must specify encrypt or decrypt as first command line parameter\n");
        return 0;
    }

    std::string engine;
    std::string block;
    job.engineType = MUM_ENGINE_TYPE_CPU;
    job.blockType = MUM_BLOCKTYPE_INVALID;
    job.numThreads = 8;

    // parse all the flags
    for (int i = 2; i < argc; i+= 2) {
        std::string flag = argv[i];
        if (flag.compare("-i") == 0) {
            job.infile = argv[i+1];
            printf("infile: %s\n", job.infile.c_str());
        } else if (flag.compare("-o") == 0) {
            job.outfile = argv[i+1];
            printf("outfile: %s\n", job.outfile.c_str());
        } else if (flag.compare("-k") == 0) {
            job.keyfile = argv[i+1];
            printf("keyfile: %s\n", job.keyfile.c_str());
        } else if (flag.compare("-e") == 0) {
            engine = argv[i+1];
            printf("engine: %s\n", engine.c_str());
        } else if (flag.compare("-b") == 0) {
            block = argv[i+1];
            printf("block: %s\n", block.c_str());
        } else {
            printf("unknown flag %s\n", flag.c_str());
            return false;
        }
    }

    // check if anything required is missing
    if (job.infile.length() == 0) {
        printf("no input file specified\n");
        return false;
    }
    if (job.keyfile.length() == 0) {
        printf("no keyfile file specified\n");
        return false;
    }


    // parse the strings now
    if (engine.length() > 0) {
        if (engine.compare("cpu") == 0) {
            job.engineType = MUM_ENGINE_TYPE_CPU;
            printf("engine type is MUM_ENGINE_TYPE_CPU\n");
        } else if (engine.compare("mt") == 0) {
            job.engineType = MUM_ENGINE_TYPE_CPU_MT;
            printf("engine type is MUM_ENGINE_TYPE_CPU_MT\n");
#ifdef USE_OPENGL
        } else if (engine.compare("gl") == 0) {
            job.engineType = MUM_ENGINE_TYPE_GPU_A;
            printf("engine type is MUM_ENGINE_TYPE_GPU_A\n");
        } else if (engine.compare("gl8") == 0) {
            job.engineType = MUM_ENGINE_TYPE_GPU_B;
            printf("engine type is MUM_ENGINE_TYPE_GPU_B\n");
#endif
        } else {
            printf("unknown engine type: %s\n", engine.c_str());
            return 0;
        }
    }
    if (block.length() > 0) {
        if (block.compare("128") == 0) {
            job.blockType = MUM_BLOCKTYPE_128;
        } else if (block.compare("256") == 0) {
            job.blockType = MUM_BLOCKTYPE_256;
        } else if (block.compare("512") == 0) {
            job.blockType = MUM_BLOCKTYPE_512;
        } else if (block.compare("1024") == 0) {
            job.blockType = MUM_BLOCKTYPE_1024;
        } else if (block.compare("2048") == 0) {
            job.blockType = MUM_BLOCKTYPE_2048;
        } else if (block.compare("4096") == 0) {
            job.blockType = MUM_BLOCKTYPE_4096;
        } else {
            printf("unknown block type: %s\n", block.c_str());
            return 0;
        }
    }

    // set auto generated output names
    if (job.outfile.length() == 0) {
        if (job.doEncrypt) {
            // auto generate output name based on input name and block size
            if (job.blockType == MUM_BLOCKTYPE_INVALID) {
                // set block type to default in none passed in
                job.blockType = MUM_BLOCKTYPE_128;
            }
            size_t len = job.infile.length() + 8;
            char *filename = (char*) malloc(len);
            MumCreateEncryptedFileName(job.blockType, job.infile.c_str(), filename, len);
            job.outfile = filename;
            printf("using output file %s\n", job.outfile.c_str());
        } else {
            // auto generate output name based on input name and block size
            size_t len = job.infile.length();
            char *filename = (char*) malloc(len);
            EMumBlockType blockType;
            auto error = MumGetInfoFromEncryptedFileName(job.infile.c_str(), &blockType, filename, len);
            if (error == MUM_ERROR_OK) {
                job.outfile = filename;
                job.blockType = blockType;
                free(filename);
                printf("using output file %s\n", job.outfile.c_str());
            } else {
                free(filename);
                printf("missing output file\n");
                return false;
            }
        }
    }

    // if blocktype still not set, & decrypt & .mu1/2/3/4/5/6 file extension,
    // use that for block type
    // else, use default 128
    if (job.blockType == MUM_BLOCKTYPE_INVALID) {
        if (!job.doEncrypt) {
            auto blockType = MumGetBlockTypeFromEncryptedFileName(job.infile.c_str());
            if (blockType == MUM_BLOCKTYPE_INVALID) {
                job.blockType = blockType;
            } else {
                job.blockType = MUM_BLOCKTYPE_128;
            }
        } else {
            job.blockType = MUM_BLOCKTYPE_128;
        }
    }

    // gl8 can only accept block size 4096
    if (job.engineType == MUM_ENGINE_TYPE_GPU_B) {
        job.blockType == MUM_BLOCKTYPE_4096;
    }

    return true;
}


bool encrypt(TJob job) {
    void *engine = MumCreateEngine(job.engineType, job.blockType, MUM_PADDING_TYPE_ON, job.numThreads);
    auto error = MumLoadKey(engine, job.keyfile.c_str());
    if (error != MUM_ERROR_OK)
    {
        if (error == MUM_ERROR_KEYFILE_SMALL) {
            printf("key file %s was too small, smaller than 4096 bytes\n",job.keyfile.c_str());
        } else {
            printf("could not load key file %s\n",job.keyfile.c_str());
        }
        MumDestroyEngine(engine);
        return false;
    }

    error = MumEncryptFile(engine, job.infile.c_str(), job.outfile.c_str());
    if (error != MUM_ERROR_OK)
    {
        if (error == MUM_ERROR_FILEIO_INPUT) {
            printf("could not open input file %s for encryption\n", job.infile.c_str());
        } else if (error == MUM_ERROR_FILEIO_OUTPUT) {
            printf("could not open output file %s for encryption\n", job.outfile.c_str());
        } else {
            printf("could not perform encryption\n");
        }
        MumDestroyEngine(engine);
        return false;
    }
    MumDestroyEngine(engine);
    return true;
}

bool decrypt(TJob job) {
    void *engine = MumCreateEngine(job.engineType, job.blockType, MUM_PADDING_TYPE_ON, job.numThreads);
    auto error = MumLoadKey(engine, job.keyfile.c_str());
    if (error != MUM_ERROR_OK)
    {
        printf("failed MumLoadKey %s, error %d\n",
            job.keyfile.c_str(),
            error);
        MumDestroyEngine(engine);
        return false;
    }

    error = MumDecryptFile(engine, job.infile.c_str(), job.outfile.c_str());
    if (error != MUM_ERROR_OK)
    {
        if (error == MUM_ERROR_FILEIO_INPUT) {
            printf("could not open input file %s for decryption\n", job.infile.c_str());
        } else if (error == MUM_ERROR_FILEIO_OUTPUT) {
            printf("could not open output file %s for decryption\n", job.outfile.c_str());
        } else {
            printf("could not perform decryption\n");
        }
        MumDestroyEngine(engine);
        return false;
    }
    MumDestroyEngine(engine);
    return true;
}

int main(int argc, char *argv[])
{
    TJob job;
    if (!parseCommandLine(argc, argv, job)) {
        printUsage();
        return 0;
    }

    if (job.doEncrypt) {
        encrypt(job);
    } else {
        decrypt(job);
    }

    return 1;
}
