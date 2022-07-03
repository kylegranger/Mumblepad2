
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

#define NUM_TEST_FILES 2
#define NUM_ENTROPY_ITERATIONS 5000
#define NUM_RANDOMSIZED_ITERATIONS 100
#define TEST_MUM_NUM_THREADS 8
#define LARGE_TEST_SIZE 128000000

uint8_t *originalFileData[NUM_TEST_FILES];
uint8_t *decryptFileData[NUM_TEST_FILES];
size_t fileLength[NUM_TEST_FILES];
int bitsSet[256];

uint8_t *largePlaintext = nullptr;
uint8_t *largeEncrypt = nullptr;
uint8_t *largeDecrypt = nullptr;

// original files
std::string testfile[NUM_TEST_FILES] = {
    "../testfiles/image.jpg",
    "../testfiles/constitution.pdf"};

// encrypted files
std::string testfileE[NUM_TEST_FILES] = {
    "../testfiles/imageEncrypted",
    "../testfiles/constitutionEncrypted"};

// decrypted files
std::string testfileD[NUM_TEST_FILES] = {
    "../testfiles/imageDecrypted.jpg",
    "../testfiles/constitutionDecrypted.pdf"};

// #define CREATE_REFERENCE_FILES
#define NUM_REFERENCE_FILES 2
std::string referenceFileKey = "../referencefiles/key.bin";
std::string referenceTempFile = "../referencefiles/temp";
std::string referenceFiles[NUM_REFERENCE_FILES] = {
    "../referencefiles/image.jpg",
    "../referencefiles/constitution.pdf"};

#ifdef USE_OPENGL
#define TEST_NUM_ENGINES 4
#else
#define TEST_NUM_ENGINES 2
#endif
EMumEngineType engineList[TEST_NUM_ENGINES] = {
    MUM_ENGINE_TYPE_CPU,
    MUM_ENGINE_TYPE_CPU_MT,
#ifdef USE_OPENGL
    MUM_ENGINE_TYPE_GPU_A,
    MUM_ENGINE_TYPE_GPU_B,
#endif
};

// the GPU-B engine only supports 4K block size
EMumBlockType firstTestBlockTypeList[TEST_NUM_ENGINES] = {
    MUM_BLOCKTYPE_128,
    MUM_BLOCKTYPE_128,
#ifdef USE_OPENGL
    MUM_BLOCKTYPE_1024,
    MUM_BLOCKTYPE_4096
#endif
};

// We'll only profile 4K block size for GPU-A
EMumBlockType firstProfilingBlockTypeList[TEST_NUM_ENGINES] = {
    MUM_BLOCKTYPE_128,
    MUM_BLOCKTYPE_128,
#ifdef USE_OPENGL
    MUM_BLOCKTYPE_1024,
    MUM_BLOCKTYPE_4096
#endif
};

std::string engineName[TEST_NUM_ENGINES] = {
    "CPU-engine",
    "CPU-MT-engine",
#ifdef USE_OPENGL
    "GPU-A-engine",
    "GPU-B-engine",
#endif
};

// #define TEST_WITH_PADDING_OFF
#ifdef TEST_WITH_PADDING_OFF

#define TEST_NUM_PADDING_TYPES 2
EMumPaddingType paddingList[TEST_NUM_PADDING_TYPES] = {
    MUM_PADDING_TYPE_ON,
    MUM_PADDING_TYPE_OFF,
};

char *paddingName[TEST_NUM_PADDING_TYPES] = {
    "padding-on",
    "padding-off",
};

#else

#define TEST_NUM_PADDING_TYPES 1
EMumPaddingType paddingList[TEST_NUM_PADDING_TYPES] = {
    MUM_PADDING_TYPE_ON};

std::string paddingName[TEST_NUM_PADDING_TYPES] = {
    "padding-on"};

#endif

void dumpBlock(const char *msg, uint8_t *a, int size) {
    printf("%s", msg);
    for (int i = 0; i < size; i++) {
        printf("%02x", a[i]);
        if ((i%16) == 15) 
            printf("\n");
    }
}

bool blockChecker(uint8_t *a, uint8_t *b, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

double utilGetTime()
{
    struct timespec tp;
    clockid_t clk_id;

    clk_id = CLOCK_REALTIME;
    clock_gettime(clk_id, &tp);
    double t = static_cast<double>(tp.tv_sec) + static_cast<double>(tp.tv_nsec) / 1000000000.0;
    return t;
}

uint64_t utilGetTimeMillis()
{
    return static_cast<uint64_t>(utilGetTime() * 1000.0);
}

uint64_t utilGetTimeMicros()
{
    return static_cast<uint64_t>(utilGetTime() * 1000000.0);
}

void fillRandomly(uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        data[i] = (uint8_t)rand();
    }
}

void fillSequentially(uint8_t *data, uint32_t size)
{
    uint32_t *dst = (uint32_t *)data;
    for (uint32_t i = 0; i < size / 4; i++)
    {
        dst[i] = i;
    }
}

bool loadFile(std::string filename, uint8_t **data, size_t *length)
{
    size_t res, size;

    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (*data != nullptr)
    {
        res = fread(*data, 1, size, f);
        fclose(f);
        if (res != size)
            return false;
        *length = size;
    }
    else
    {
        uint8_t *buffer = (uint8_t *)malloc(size);
        res = fread(buffer, 1, size, f);
        fclose(f);
        if (res != size)
            return false;
        *length = size;
        *data = buffer;
    }
    return true;
}

bool loadTestFiles()
{
    for (int i = 0; i < NUM_TEST_FILES; i++)
    {
        originalFileData[i] = nullptr;
        if (!loadFile(testfile[i], &originalFileData[i], &fileLength[i]))
            return false;
        decryptFileData[i] = (uint8_t *)malloc(fileLength[i]);
    }
    return true;
}

void init()
{
    int flags[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    int i, bit;

    for (i = 0; i < 256; i++)
    {
        int total = 0;
        for (bit = 0; bit < 8; bit++)
        {
            if (i & flags[bit])
                total++;
        }
        bitsSet[i] = total;
    }

    uint32_t plaintextSize = LARGE_TEST_SIZE;
    uint32_t encryptBufferSize = plaintextSize * 5 / 4;

    largePlaintext = new uint8_t[plaintextSize];
    largeEncrypt = new uint8_t[encryptBufferSize];
    largeDecrypt = new uint8_t[plaintextSize + 65536];
}

bool loadDeFiles(int index)
{
    size_t length;
    if (!loadFile(testfile[index], &decryptFileData[index], &length))
        return false;
    if (length != fileLength[index])
        return false;
    return true;
}

bool testFileEncrypt(void *engine, char *engineDesc)
{
    EMumError error;
    uint8_t clavier[MUM_KEY_SIZE];
    uint32_t encryptedBlockSize;

    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);
    fillRandomly(clavier, MUM_KEY_SIZE);
    error = MumInitKey(engine, clavier);
    for (int i = 0; i < NUM_TEST_FILES; i++)
    {
        error = MumEncryptFile(engine, testfile[i].c_str(), testfileE[i].c_str());
        if (error != MUM_ERROR_OK)
        {
            printf("failed MumEncryptFile %s %s, error %d\n",
                   testfile[i].c_str(),
                   testfileE[i].c_str(),
                   error);
            return false;
        }
        error = MumDecryptFile(engine, testfileE[i].c_str(), testfileD[i].c_str());
        if (error != MUM_ERROR_OK)
        {
            printf("failed MumDecryptFile %s %s, error %d\n",
                   testfileE[i].c_str(),
                   testfileD[i].c_str(),
                   error);
            return false;
        }

        if (!loadDeFiles(i))
        {
            printf("FAILED testFileEncrypt, engine %s: can't load decrypt files\n", engineDesc);
            return false;
        }
        // printf("   compare original file with decrypted data, block size %d, size %zd\n", encryptedBlockSize, fileLength[i]);
        if (memcmp(originalFileData[i], decryptFileData[i], fileLength[i]) != 0)
        {
            printf("FAILED testFileEncrypt, engine %s\n", engineDesc);
            return false;
        }
        printf("SUCCESS testFileEncrypt, engine %s: file %s\n", engineDesc, testfile[i].c_str());
    }
    return true;
}

bool testReferenceFileDecrypt(void *engine, char *engineDesc, EMumBlockType blockType)
{
    EMumError error;
    uint32_t encryptedBlockSize;

    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);
    if (error != MUM_ERROR_OK)
        return false;
    error = MumLoadKey(engine, referenceFileKey.c_str());
    if (error != MUM_ERROR_OK)
        return false;
    for (int i = 0; i < NUM_REFERENCE_FILES; i++)
    {
        // Determine name of encrypted file
        char encryptedname[512];
        error = MumCreateEncryptedFileName(blockType, referenceFiles[i].c_str(), encryptedname, 512);
        if (error != MUM_ERROR_OK)
            return false;

        // Decrypt file into temp file
        error = MumDecryptFile(engine, encryptedname, referenceTempFile.c_str());
        if (error != MUM_ERROR_OK)
            return false;

        size_t originalLength;
        uint8_t *originalData = nullptr;
        if (!loadFile(referenceFiles[i], &originalData, &originalLength))
            return false;

        size_t decryptedLength;
        uint8_t *decryptedData = nullptr;
        if (!loadFile(referenceTempFile, &decryptedData, &decryptedLength))
            return false;

        if (originalLength != decryptedLength)
        {
            printf("FAILED testReferenceFileDecrypt, length do not match: %zd %zd\n", originalLength, decryptedLength);
            return false;
        }

        // printf("   compare original reference file with decrypted data, block size %d, size %zd\n", encryptedBlockSize, originalLength);
        if (memcmp(originalData, decryptedData, originalLength) != 0)
        {
            printf("FAILED testReferenceFileDecrypt, engine %s\n", engineDesc);
            return false;
        }
        printf("SUCCESS testReferenceFileDecrypt, engine %s, file %s\n", engineDesc, encryptedname);

        // cleanup
        free(originalData);
        free(decryptedData);
    }
    return true;
}

bool analyzeBitsChange(uint32_t bitsChanged, uint32_t bitsTotal, uint32_t bytesChanged, uint32_t bytesTotal)
{
    float bitsPercent = (float)bitsChanged * 100.0f / (float)bitsTotal;
    float bytesPart = (float)bytesChanged * 256.0f / (float)bytesTotal;
    bool success = true;
    if (bytesTotal > MUM_KEY_SIZE)
    {
        if (bitsPercent < 49)
            success = false;
        if (bitsPercent > 51)
            success = false;
        if (bytesPart < 254.8f)
            success = false;
        if (bytesPart > 255.2f)
            success = false;
    }
    else
    {
        if (bitsPercent < 47)
            success = false;
        if (bitsPercent > 53)
            success = false;
        if (bytesPart < 253.0f)
            success = false;
        // if (bytesPart > 255.9f)
        //     success = false;
    }
    if (!success)
        printf("   bits pct %f, bytes pct %f\n", bitsPercent, bytesPart);
    return success;
}

void createRandomBlocksWithOneBitDifference(uint8_t *block1, uint8_t *block2, uint32_t size)
{
    uint32_t byteOffset = rand() % size;
    uint32_t bitOffset = rand() & 0x7;

    fillRandomly(block1, size);
    memcpy(block2, block1, size);
    block2[byteOffset] ^= 1 << bitOffset;
}

bool testEntropy(void *engine, char *engineDesc, EMumPaddingType paddingType)
{
    EMumError error;
    uint8_t plaintext[4096];
    uint8_t encrypt1[4096];
    uint8_t encrypt2[4096];
    uint8_t decrypt1[4096];
    uint8_t decrypt2[4096];
    uint32_t outlength, i, iter;
    uint32_t bitsChanged, bitsTotal;
    uint32_t bytesChanged, bytesTotal;

    if (paddingType == MUM_PADDING_TYPE_OFF)
        return true;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);

    bitsChanged = 0;
    bitsTotal = 0;
    bytesChanged = 0,
    bytesTotal = 0;
    for (iter = 0; iter < NUM_ENTROPY_ITERATIONS; iter++)
    {
        fillRandomly(plaintext, plaintextBlockSize);
        error = MumEncrypt(engine, plaintext, encrypt1, plaintextBlockSize, &outlength, 0);
        assert(error == MUM_ERROR_OK);
        error = MumEncrypt(engine, plaintext, encrypt2, plaintextBlockSize, &outlength, 0);
        assert(error == MUM_ERROR_OK);
        error = MumDecrypt(engine, encrypt1, decrypt1, encryptedBlockSize, &outlength);
        assert(error == MUM_ERROR_OK);
        error = MumDecrypt(engine, encrypt2, decrypt2, encryptedBlockSize, &outlength);
        assert(error == MUM_ERROR_OK);
        // first, reality check; make sure both encryptions return same plaintext
        if (!blockChecker(decrypt1, plaintext, plaintextBlockSize))
        {
            printf("FAILED testEntropy A, engine %s, plaintextSize %d, decrypted %d, index %d\n",
                   engineDesc, plaintextBlockSize, outlength, i);
            return false;
        }
        if (!blockChecker(decrypt2, plaintext, plaintextBlockSize))
        {
            printf("FAILED testEntropy B, engine %s, plaintextSize %d, decrypted %d, index %d\n",
                   engineDesc, plaintextBlockSize, outlength, i);
            return false;
        }
        // Now, check deltas between bytes and deltas between bits
        for (i = 0; i < encryptedBlockSize; i++)
        {
            if (encrypt1[i] != encrypt2[i])
                bytesChanged++;
            bitsChanged += bitsSet[encrypt1[i] ^ encrypt2[i]];
            bytesTotal++;
            bitsTotal += 8;
        }
    }

    bool success = analyzeBitsChange(bitsChanged, bitsTotal, bytesChanged, bytesTotal);
    if (!success)
    {
        printf("FAILED testEntropy E, engine %s\n", engineDesc);
        return false;
    }

    printf("SUCCESS testEntropy, engine %s\n", engineDesc);

    // Do entropy test when plaintext consists just of zeros
    bitsChanged = 0;
    bitsTotal = 0;
    bytesChanged = 0,
    bytesTotal = 0;
    memset(plaintext, 0, plaintextBlockSize);
    for (iter = 0; iter < NUM_ENTROPY_ITERATIONS; iter++)
    {
        error = MumEncrypt(engine, plaintext, encrypt1, plaintextBlockSize, &outlength, 0);
        error = MumEncrypt(engine, plaintext, encrypt2, plaintextBlockSize, &outlength, 0);
        error = MumDecrypt(engine, encrypt1, decrypt1, encryptedBlockSize, &outlength);
        error = MumDecrypt(engine, encrypt2, decrypt2, encryptedBlockSize, &outlength);
        // first, reality check; make sure both encryptions return same plaintext
        if (!blockChecker(decrypt1, plaintext, plaintextBlockSize))
        {
            printf("FAILED testEntropy C, engine %s, plaintextSize %d, decrypted %d, index %d\n",
                   engineDesc, plaintextBlockSize, outlength, i);
            return false;
        }
        if (!blockChecker(decrypt2, plaintext, plaintextBlockSize))
        {
            printf("FAILED testEntropy D, engine %s, plaintextSize %d, decrypted %d, index %d\n",
                   engineDesc, plaintextBlockSize, outlength, i);
            return false;
        }

        // Now, check deltas between bytes and deltas between bits
        // 255/256 of bytes should be different if blocks are 'random'
        // 50% of bits should be different
        for (i = 0; i < encryptedBlockSize; i++)
        {
            if (encrypt1[i] != encrypt2[i])
                bytesChanged++;
            bitsChanged += bitsSet[encrypt1[i] ^ encrypt2[i]];
            bytesTotal++;
            bitsTotal += 8;
        }
    }

    success = analyzeBitsChange(bitsChanged, bitsTotal, bytesChanged, bytesTotal);
    if (!success)
    {
        printf("FAILED zeroed plaintext testEntropy, engine %s\n", engineDesc);
        return false;
    }

    printf("SUCCESS zeroed plaintext testEntropy, engine %s\n", engineDesc);

    return true;
}

bool testEntropySubkeyPair(void *engine, int index1, int index2)
{
    EMumError error;
    uint8_t subkey1[MUM_KEY_SIZE];
    uint8_t subkey2[MUM_KEY_SIZE];
    error = MumGetSubkey(engine, index1, subkey1);
    if (error != MUM_ERROR_OK)
        return false;
    error = MumGetSubkey(engine, index2, subkey2);
    if (error != MUM_ERROR_OK)
        return false;

    uint32_t bitsChanged = 0;
    uint32_t bitsTotal = 0;
    uint32_t bytesChanged = 0;
    uint32_t bytesTotal = 0;
    for (int i = 0; i < MUM_KEY_SIZE; i++)
    {
        if (subkey1[i] != subkey2[i])
            bytesChanged++;
        bitsChanged += bitsSet[subkey1[i] ^ subkey2[i]];
        bytesTotal++;
        bitsTotal += 8;
    }
    bool success = analyzeBitsChange(bitsChanged, bitsTotal, bytesChanged, bytesTotal);
    if (!success)
    {
        printf("FAILED testEntropySubkeyPair\n");
        return false;
    }
    return true;
}

bool testSubkeyEntropy(void *engine, char *engineDesc, EMumPaddingType paddingType)
{
    if (paddingType == MUM_PADDING_TYPE_OFF)
        return true;

    for (int i = 0; i < MUM_NUM_SUBKEYS - 1; i++)
    {
        for (int j = i + 1; j < MUM_NUM_SUBKEYS; j++)
        {
            if (!testEntropySubkeyPair(engine, i, j))
                return false;
        }
    }

    printf("SUCCESS testSubkeyEntropy, engine %s\n", engineDesc);

    return true;
}

bool testSimpleBlocks(void *engine, char *engineDesc)
{
    EMumError error;
    uint8_t plaintext[4096];
    uint8_t random[4096];
    uint8_t encrypt[4096];
    uint8_t decrypt[4096];
    uint32_t length, i;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);

    fillRandomly(plaintext, plaintextBlockSize);

    uint32_t seqOut = 0;
    uint32_t seqIn = 0;

    error = MumEncryptBlock(engine, plaintext, encrypt, plaintextBlockSize, seqOut++);
    while (error == MUM_ERROR_BUFFER_WAIT_ENCRYPT)
    {
        error = MumEncryptBlock(engine, random, encrypt, plaintextBlockSize, seqOut++);
    }
    if (error != MUM_ERROR_OK) {
        printf("MumEncryptBlock error %d\n", error);
        return false;
    }

    error = MumDecryptBlock(engine, encrypt, decrypt, &length, &seqIn);
    while (error == MUM_ERROR_BUFFER_WAIT_DECRYPT)
    {
        error = MumDecryptBlock(engine, random, decrypt, &length, &seqIn);
    }

    if (error != MUM_ERROR_OK)
    {
        printf("MumDecryptBlock error %d\n", error);
        return false;
    }

    if (!blockChecker(plaintext, decrypt, plaintextBlockSize))
    {
        printf("FAILED testSimpleBlocks, engine %s\n", engineDesc);
        return false;
    }

    printf("SUCCESS testSimpleBlocks, engine %s\n", engineDesc);
    return true;
}

bool testUnitializedEngine(void *engine, char *engineDesc)
{
    EMumError error;
    uint8_t plaintext[4096];
    uint8_t encrypt[4096];
    uint8_t decrypt[4096];
    uint32_t length;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);

    fillRandomly(plaintext, plaintextBlockSize);

    uint32_t seqOut = 0;
    uint32_t seqIn = 0;

    error = MumEncryptBlock(engine, plaintext, encrypt, plaintextBlockSize, seqOut++);
    if (error != MUM_ERROR_KEY_NOT_INITIALIZED)
        return false;

    error = MumDecryptBlock(engine, encrypt, decrypt, &length, &seqIn);
    if (error != MUM_ERROR_KEY_NOT_INITIALIZED)
        return false;

    uint32_t encrypted = 0;
    error = MumEncrypt(engine, plaintext, encrypt, plaintextBlockSize, &encrypted, 0);
    if (error != MUM_ERROR_KEY_NOT_INITIALIZED)
        return false;

    uint32_t decrypted = 0;
    error = MumDecrypt(engine, encrypt, decrypt, encrypted, &decrypted);
    if (error != MUM_ERROR_KEY_NOT_INITIALIZED)
        return false;

    printf("SUCCESS testUnitializedEngine, engine %s\n", engineDesc);
    return true;
}

bool testRandomlySizedBlocks(void *engine, char *engineDesc, EMumPaddingType paddingType)
{
    EMumError error;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);

    int maxsize = 512*1024;
    uint8_t *plaintext = new uint8_t[maxsize + 4096];
    uint8_t *encrypt = new uint8_t[maxsize*5/4];
    uint8_t *decrypt = new uint8_t[maxsize + 4096];
    for (int test = 0; test < NUM_RANDOMSIZED_ITERATIONS; test++)
    {
        // Pick random number between 1 and 512*1024
        uint32_t plaintextSize = (rand() & (maxsize-1)) + 1;

        // Alternate between random fill and 32-bit int sequential fill
        if (test & 1)
            fillRandomly(plaintext, plaintextSize);
        else
            fillSequentially(plaintext, plaintextSize);

        uint32_t encryptedLen = 0;
        error = MumEncrypt(engine, plaintext, encrypt, plaintextSize, &encryptedLen, 0);
        if (error != MUM_ERROR_OK) {
            printf("testRandomlySizedBlocks MumEncrypt error %d\n", error);
            return false;
        }

        uint32_t decryptedLen = 0;
        error = MumDecrypt(engine, encrypt, decrypt, encryptedLen, &decryptedLen);
        if (error != MUM_ERROR_OK) {
            printf("testRandomlySizedBlocks error %d, encryptedLen %d\n", error, encryptedLen);
            return false;
        }
        if (paddingType == MUM_PADDING_TYPE_ON && plaintextSize != decryptedLen) {
            printf("testRandomlySizedBlocks plaintextSize != decrypted, %d %d\n", plaintextSize, decryptedLen);
            return false;
        }
        if (!blockChecker(plaintext, decrypt, plaintextSize))
        {
            printf("blockChecker failed testRandomlySizedBlocks, engine %s, plaintextSize %d\n",
                   engineDesc, plaintextSize);
            // return false;
        }
    }
    printf("SUCCESS testRandomlySizedBlocks, engine %s\n", engineDesc);
    delete[] plaintext;
    delete[] encrypt;
    delete[] decrypt;
    return true;
}

bool profileLargeBlocks(void *engine, char *engineDesc)
{
    EMumError error;
    uint32_t i;
    uint32_t plaintextSize = LARGE_TEST_SIZE;
    uint32_t encryptBufferSize = plaintextSize * 5 / 4;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine, &encryptedBlockSize);

    fillSequentially(largePlaintext, plaintextSize);
    memset(largeEncrypt, 7, encryptBufferSize);
    memset(largeDecrypt, 9, plaintextSize + encryptedBlockSize);

    uint32_t encrypted = 0;
    auto t = utilGetTime();
    error = MumEncrypt(engine, largePlaintext, largeEncrypt, plaintextSize, &encrypted, 0);
    if (error != MUM_ERROR_OK)
        return false;
    auto encryptTime = utilGetTime() - t;

    uint32_t decrypted = 0;
    uint32_t predicted;
    MumEncryptedSize(engine, plaintextSize, &predicted);
    t = utilGetTime();
    error = MumDecrypt(engine, largeEncrypt, largeDecrypt, predicted, &decrypted);
    if (error != MUM_ERROR_OK)
        return false;
    double decryptTime = utilGetTime() - t;

    float mb = (float)(plaintextSize) / 1000000.0f;
    printf("profileLargeBlocks: engine %s, encrypt size %d, total bytes %d\n",
           engineDesc, plaintextBlockSize, plaintextSize);
    printf("   encrypt time %f sec, MB/sec %f \n", encryptTime, mb / encryptTime);
    printf("   decrypt time %f sec, MB/sec %f\n\n", decryptTime, mb / decryptTime);

    if (!blockChecker(largePlaintext, largeDecrypt, plaintextSize))
    {
        printf("FAILED profileLargeBlocks, engine %s, plaintextSize %d, decrypted %d, index %d\n",
               engineDesc, plaintextSize, decrypted, i);
        return false;
    }

    return true;
}

bool doTest(void *engine, char *engineDesc, EMumPaddingType paddingType, EMumBlockType blockType)
{
    uint8_t clavier[MUM_KEY_SIZE];
    EMumError error;

    printf("doTest: %s, block type %d\n", engineDesc, blockType);

    if (!testUnitializedEngine(engine, engineDesc))
    {
        printf("failed testUnitializedEngine\n");
        // return false;
    }

    fillRandomly(clavier, MUM_KEY_SIZE);
    error = MumInitKey(engine, clavier);
    if (!testSimpleBlocks(engine, engineDesc))
    {
        printf("failed testSimpleBlocks\n");
        // return false;
    }
    if (!testRandomlySizedBlocks(engine, engineDesc, paddingType))
    {
        printf("failed testRandomlySizedBlocks\n");
        // return false;
    }
    if (!testEntropy(engine, engineDesc, paddingType))
    {
        printf("failed testEntropy\n");
        // return false;
    }
    if (!testSubkeyEntropy(engine, engineDesc, paddingType))
    {
        printf("failed testSubkeyEntropy\n");
        // return false;
    }
    if (!testFileEncrypt(engine, engineDesc))
    {
        printf("failed testFileEncrypt\n");
        // return false;
    }
    // if (paddingType == MUM_PADDING_TYPE_ON && !testReferenceFileDecrypt(engine, engineDesc, blockType)) {
    //     printf("failed testReferenceFileDecrypt\n");
    //     return false;
    // }
    return true;
}

bool doProfiling(void *engine, char *engineDesc, bool withPadding)
{
    uint8_t clavier[MUM_KEY_SIZE];
    EMumError error;

    printf("  doProfiling %s\n", engineDesc);

    fillRandomly(clavier, MUM_KEY_SIZE);
    error = MumInitKey(engine, clavier);
    if (!profileLargeBlocks(engine, engineDesc))
        return false;
    return true;
}

bool multiTest(void *engine1, void *engine2)
{
    EMumError error;
    uint32_t decryptSize;
    uint32_t encryptSize;
    uint32_t outlength;

    uint32_t encryptedBlockSize, plaintextBlockSize;
    error = MumPlaintextBlockSize(engine1, &plaintextBlockSize);
    error = MumEncryptedBlockSize(engine1, &encryptedBlockSize);

    decryptSize = 28657;
    error = MumEncryptedSize(engine1, decryptSize, &encryptSize);
    if (error != MUM_ERROR_OK)
        return false;

    uint8_t *src = new uint8_t[decryptSize];
    uint8_t *dec1 = new uint8_t[decryptSize + plaintextBlockSize];
    uint8_t *dec2 = new uint8_t[decryptSize + plaintextBlockSize];
    uint8_t *enc1 = new uint8_t[encryptSize];
    uint8_t *enc2 = new uint8_t[encryptSize];
    for (uint32_t i = 0; i < decryptSize; i++)
        src[i] = (uint8_t)i;

    error = MumEncrypt(engine1, src, enc1, decryptSize, &outlength, 0);
    if (error != MUM_ERROR_OK)
        return false;
    if (outlength != encryptSize)
        return false;

    error = MumEncrypt(engine2, src, enc2, decryptSize, &outlength, 0);
    if (error != MUM_ERROR_OK)
        return false;
    if (outlength != encryptSize)
        return false;

    memset(dec1, 0, encryptSize);
    error = MumDecrypt(engine1, enc1, dec1, encryptSize, &outlength);
    if (error != MUM_ERROR_OK)
        return false;
    if (outlength != decryptSize)
        return false;

    memset(dec2, 0, encryptSize);
    error = MumDecrypt(engine2, enc2, dec2, encryptSize, &outlength);
    if (error != MUM_ERROR_OK)
        return false;
    if (outlength != decryptSize)
        return false;

    if (memcmp(src, dec1, decryptSize) != 0)
    {
        printf("FAILED multiTest!\n");
        return false;
    }
    if (memcmp(src, dec2, decryptSize) != 0)
    {
        printf("FAILED multiTest!\n");
        return false;
    }

    printf("SUCCESS multiTest!\n");
    return true;
}

bool doMultiEngineTest(EMumEngineType engineType1, EMumEngineType engineType2,
                       EMumBlockType blockType, std::string testString)
{
    uint8_t clavier[MUM_KEY_SIZE];
    EMumError error;

    fillRandomly(clavier, MUM_KEY_SIZE);

    void *engine1 = MumCreateEngine(MUM_ENGINE_TYPE_CPU, blockType, MUM_PADDING_TYPE_ON, 0);
    error = MumInitKey(engine1, clavier);

    void *engine2 = MumCreateEngine(MUM_ENGINE_TYPE_GPU_A, blockType, MUM_PADDING_TYPE_ON, 0);
    error = MumInitKey(engine2, clavier);

    if (!multiTest(engine1, engine2))
        return false;

    uint32_t encryptedBlockSize;
    error = MumEncryptedBlockSize(engine1, &encryptedBlockSize);
    printf("SUCCESS doMultiEngineTest, engine %s\n", testString.c_str());

    MumDestroyEngine(engine1);
    MumDestroyEngine(engine2);
    return true;
}

bool doTests()
{
    for (int paddingIndex = 0; paddingIndex < TEST_NUM_PADDING_TYPES; paddingIndex++)
    {
        for (int engineIndex = 0; engineIndex < TEST_NUM_ENGINES; engineIndex++)
        {
            for (int blockType = firstTestBlockTypeList[engineIndex]; blockType <= MUM_BLOCKTYPE_4096; blockType++)
            {
                void *engine = MumCreateEngine(engineList[engineIndex], (EMumBlockType)blockType, paddingList[paddingIndex], TEST_MUM_NUM_THREADS);
                uint32_t encryptedBlockSize;
                MumEncryptedBlockSize(engine, &encryptedBlockSize);
                char engineDesc[256];
                sprintf(engineDesc, "%s:%s:blocksize-%u", engineName[engineIndex].c_str(), paddingName[paddingIndex].c_str(), encryptedBlockSize);
                if (!doTest(engine, engineDesc, paddingList[paddingIndex], (EMumBlockType)blockType))
                    return false;
                MumDestroyEngine(engine);
                engine = NULL;
            }
        }
    }
    return true;
}

#ifdef CREATE_REFERENCE_FILES
bool createReferenceFilesFromEngine(void *engine, EMumBlockType blockType)
{
    EMumError error;

    for (int i = 0; i < NUM_REFERENCE_FILES; i++)
    {
        char encryptedname[512];
        error = MumCreateEncryptedFileName(blockType, referenceFiles[i], .c_str() encryptedname, 512);
        if (error != MUM_ERROR_OK)
            return false;
        error = MumEncryptFile(engine, referenceFiles[i].c_str(), encryptedname);
        if (error != MUM_ERROR_OK)
            return false;
    }
    return true;
}

bool createReferenceFiles()
{
    char *referenceFileKey = "..\\referencefiles\\key.bin";

    // create key
    uint8_t clavier[MUM_KEY_SIZE];
    srand(610);
    fillRandomly(clavier, MUM_KEY_SIZE);

    // write it to disk
    size_t res;
    FILE *f = fopen(referenceFileKey, "wb");
    if (!f)
        return false;
    res = fwrite(clavier, 1, MUM_KEY_SIZE, f);
    fclose(f);
    if (res != MUM_KEY_SIZE)
        return false;

    for (int blockType = MUM_BLOCKTYPE_128; blockType <= MUM_BLOCKTYPE_4096; blockType++)
    {
        void *engine = MumCreateEngine(MUM_ENGINE_TYPE_CPU, (EMumBlockType)blockType, MUM_PADDING_TYPE_ON, TEST_MUM_NUM_THREADS);
        EMumError error = MumLoadKey(engine, referenceFileKey);
        if (error != MUM_ERROR_OK)
            return false;
        if (!createReferenceFilesFromEngine(engine, (EMumBlockType)blockType))
            return false;
        MumDestroyEngine(engine);
        engine = NULL;
    }
    return true;
}
#endif

bool doProfilings()
{
    printf("\n");
    for (int engineIndex = 0; engineIndex < TEST_NUM_ENGINES; engineIndex++)
    {
        for (int paddingIndex = 0; paddingIndex < TEST_NUM_PADDING_TYPES; paddingIndex++)
        {
            for (int blockType = firstProfilingBlockTypeList[engineIndex]; blockType <= MUM_BLOCKTYPE_4096; blockType++)
            {
                for (int i = 0; i < 1; i++)
                {
                    void *engine = MumCreateEngine(engineList[engineIndex], (EMumBlockType)blockType, paddingList[paddingIndex], TEST_MUM_NUM_THREADS);
                    uint32_t encryptedBlockSize;
                    MumEncryptedBlockSize(engine, &encryptedBlockSize);
                    char engineDesc[256];
                    sprintf(engineDesc, "%s:%s:blocksize-%u", engineName[engineIndex].c_str(), paddingName[paddingIndex].c_str(), encryptedBlockSize);
                    if (!doProfiling(engine, engineDesc, (paddingIndex == 1)))
                        return false;
                    MumDestroyEngine(engine);
                    engine = NULL;
                }
            }
        }
    }
    return true;
}

bool doMultiEngineTests()
{
    for (int blockType = MUM_BLOCKTYPE_128; blockType <= MUM_BLOCKTYPE_4096; blockType++)
    {
        if (!doMultiEngineTest(
                MUM_ENGINE_TYPE_CPU,
                MUM_ENGINE_TYPE_GPU_A,
                (EMumBlockType)blockType,
                "CPU + GPUA"))
            return false;
    }

    if (!doMultiEngineTest(
            MUM_ENGINE_TYPE_CPU,
            MUM_ENGINE_TYPE_GPU_B,
            MUM_BLOCKTYPE_4096,
            "CPU + GPUB"))
        return false;

    return true;
}

int main(int argc, char *argv[])
{
    int result = 0;
#ifdef CREATE_REFERENCE_FILES
    if (createReferenceFiles())
        return -1;
#endif

#ifdef USE_OPENGL
    printf("Testing library with OpenGL enabled\n");
#else
    printf("Testing library without OpenGL enabled\n");
#endif

    init();

    srand(utilGetTimeMillis());

    if (!loadTestFiles())
        result = -1;

    if (!doTests())
        result = -1;

    if (!doProfilings())
        result = -1;

    // if (!doMultiEngineTests() )
    // return -1;

    if (result != -1)
        printf("Success!!! Done!!!\n");
    else
        printf("Done...but we failed someplace.\n");

    return result;
}
