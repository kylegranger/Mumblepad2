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

#ifndef MUMPUBLIC_H
#define MUMPUBLIC_H

#include "stddef.h"
#include "stdint.h"

#define MUM_KEY_SIZE          4096
#define MUM_MAX_BLOCK_SIZE    4096
#define MUM_NUM_SUBKEYS       560
#define MUM_PRNG_SUBKEY_INDEX 304


typedef enum EMumEngineType {
    MUM_ENGINE_TYPE_NONE   = -1,
    MUM_ENGINE_TYPE_CPU    = 100,
    MUM_ENGINE_TYPE_CPU_MT = 101,
    MUM_ENGINE_TYPE_GPU_A  = 102,
    MUM_ENGINE_TYPE_GPU_B  = 103,
} EMumEngineType;

typedef enum EMumError {
    MUM_ERROR_OK = 0,
    MUM_ERROR_FILEIO_INPUT = -1001,
    MUM_ERROR_FILEIO_OUTPUT = -1002,
    MUM_ERROR_INVALID_ENCRYPT_SIZE = -1003,
    MUM_ERROR_INVALID_DECRYPT_SIZE = -1004,
    MUM_ERROR_INVALID_ENCRYPTED_BLOCK = -1005,
    MUM_ERROR_INVALID_BLOCK_SIZE = -1006,
    MUM_ERROR_INVALID_MAX_ENCRYPT_SIZE = -1007,
    MUM_ERROR_BUFFER_WAIT_ENCRYPT = -1008,
    MUM_ERROR_BUFFER_WAIT_DECRYPT = -1009,
    MUM_ERROR_RENDERER_NOT_MULTITHREADED = -1010,
    MUM_ERROR_MTRENDERER_NO_THREADS = -1011,
    MUM_ERROR_KEYFILE_READ = -1012,
    MUM_ERROR_KEYFILE_WRITE = -1013,
    MUM_ERROR_INVALID_FILE_EXTENSION = -1014,
    MUM_ERROR_SUBKEY_INDEX_OUTOFRANGE = -1015,
    MUM_ERROR_KEY_NOT_INITIALIZED = -1016,
    MUM_ERROR_LENGTH_TOO_SMALL = -1017,
    MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE = -1018,
    MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH = -1019,
    MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM = -1020,
    MUM_ERROR_KEYFILE_SMALL = -1021,
} EMumError;

typedef enum EMumBlockType {
    MUM_BLOCKTYPE_INVALID = 0,
    // maximum encrypt size is 112 bytes
    MUM_BLOCKTYPE_128 = 1,
    // maximum encrypt size is 240 bytes
    MUM_BLOCKTYPE_256 = 2,
    // maximum encrypt size is 496 bytes
    MUM_BLOCKTYPE_512 = 3,
    // maximum encrypt size is 1000 bytes
    MUM_BLOCKTYPE_1024 = 4,
    // maximum encrypt size is 2000 bytes
    MUM_BLOCKTYPE_2048 = 5,
    // maximum encrypt size is 4000 bytes
    MUM_BLOCKTYPE_4096 = 6
} EMumBlockType;

typedef enum EMumPaddingType {
    MUM_PADDING_TYPE_OFF = 0,
    MUM_PADDING_TYPE_ON = 1,
} EMumPaddingType;


extern void * MumCreateEngine(EMumEngineType engineType, EMumBlockType blockType, EMumPaddingType paddingType, uint32_t numThreads);
extern void MumDestroyEngine(void *me);
extern EMumError MumInitKey(void *me, uint8_t *key);
extern EMumError MumGetSubkey(void *me, uint32_t index, uint8_t *subkey);
extern EMumError MumLoadKey(void *me, const char *keyfile);
extern EMumError MumEncryptBlock(void *me, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum);
extern EMumError MumDecryptBlock(void *me, uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum);
extern EMumError MumEncrypt(void *me, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum);
extern EMumError MumDecrypt(void *me, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength);
extern EMumError MumEncryptFile(void *me, const char *srcfile, const char *dstfile);
extern EMumError MumDecryptFile(void *me, const char *srcfile, const char *dstfile);
extern EMumError MumPlaintextBlockSize(void *me, uint32_t *plaintextBlockSize);
extern EMumError MumEncryptedBlockSize(void *me, uint32_t *encryptedBlockSize);
extern EMumError MumEncryptedSize(void *me, uint32_t plaintextSize, uint32_t *encryptedSize);
// adds a file extension to a file based, based on the block size/type:
// .mu1 = 128-byte block
// .mu2 = 256-byte block
// .mu3 = 512-byte block
// .mu4 = 1024-byte block
// .mu5 = 2048-byte block
// .mu6 = 4096-byte block
extern EMumError MumCreateEncryptedFileName(EMumBlockType blockType, const char *plaintextname, char *encryptname, size_t outlength);
// returns original filename (stripped of Mumblepad extension), plus block type used.
extern EMumError MumGetInfoFromEncryptedFileName(const char *encryptname, EMumBlockType *blockType, char *decryptname, size_t outlength);
// just returns block type, if exists in file extension; returns invalid block if not found
extern EMumBlockType MumGetBlockTypeFromEncryptedFileName(const char *encryptname);


#endif

