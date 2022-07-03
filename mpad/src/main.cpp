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
    printf("Usage of mpad:\n");
    printf("   mpad encrypt|decrypt [options]\n");
    printf("   Options:\n");
    printf("      -i <input-file> \n");
    printf("      -o <input-file> \n");
    printf("      -k <key-file> \n");
    printf("      -e cpu | mt | gl \n");
    printf("         single-threaded, multi-threaded, or OpenGL\n");
    printf("         is optional, default is cpu\n");
    printf("      -b 128 | 256 | 512 | 1024 | 2048 | 4096 \n");
    printf("         this is the block size\n");
    printf("         is optional for encrypt, default is 128\n");
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

    std::string engine = "cpu";
    std::string block = "128";
    job.numThreads = 8;

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

    if (job.infile.length() == 0) {
        printf("no input file specified\n");
        return false;
    }
    if (job.outfile.length() == 0) {
        printf("no output file specified\n");
        return false;
    }
    if (job.keyfile.length() == 0) {
        printf("no keyfile file specified\n");
        return false;
    }

    if (engine.compare("cpu") == 0) {
        job.engineType = MUM_ENGINE_TYPE_CPU;
        printf("engine type is MUM_ENGINE_TYPE_CPU\n");
    } else if (engine.compare("mp") == 0) {
        job.engineType = MUM_ENGINE_TYPE_CPU_MT;
        printf("engine type is MUM_ENGINE_TYPE_CPU_MT\n");
    } else if (engine.compare("gl") == 0) {
        job.engineType = MUM_ENGINE_TYPE_GPU_A;
        printf("engine type is MUM_ENGINE_TYPE_GPU_A\n");
    } else {
        printf("unknown engine type: %s\n", engine.c_str());
        return 0;
    }
    if (block.compare("128") == 0) {
        job.blockType = MUM_BLOCKTYPE_128;
        printf("block type is MUM_BLOCKTYPE_128\n");
    } else if (block.compare("256") == 0) {
        job.blockType = MUM_BLOCKTYPE_256;
        printf("block type is MUM_BLOCKTYPE_256\n");
    } else if (block.compare("512") == 0) {
        job.blockType = MUM_BLOCKTYPE_512;
        printf("block type is MUM_BLOCKTYPE_512\n");
    } else if (block.compare("1024") == 0) {
        job.blockType = MUM_BLOCKTYPE_1024;
        printf("block type is MUM_BLOCKTYPE_1024\n");
    } else if (block.compare("2048") == 0) {
        job.blockType = MUM_BLOCKTYPE_2048;
        printf("block type is MUM_BLOCKTYPE_2048\n");
    } else if (block.compare("4096") == 0) {
        job.blockType = MUM_BLOCKTYPE_4096;
        printf("block type is MUM_BLOCKTYPE_4096\n");
    } else {
        printf("unknown block type: %s\n", block.c_str());
        return 0;
    }
    return true;
}


bool encrypt(TJob job) {
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

    error = MumEncryptFile(engine, job.infile.c_str(), job.outfile.c_str());
    if (error != MUM_ERROR_OK)
    {
        printf("failed MumEncryptFile %s %s, error %d\n",
            job.infile.c_str(),
            job.outfile.c_str(),
            error);
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
        printf("failed MumEncryptFile %s %s, error %d\n",
            job.infile.c_str(),
            job.outfile.c_str(),
            error);
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
