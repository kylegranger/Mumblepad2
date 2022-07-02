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

#include "mumpublic.h"
#include "mumengine.h"
#include "stdio.h"
#include <string.h>
#include "assert.h"

EMumError MumEncryptedSize(void *mev, uint32_t plaintextSize, uint32_t *encryptedSize)
{
    CMumEngine *me = NULL;

    *encryptedSize = 0;
    me = (CMumEngine *)mev;
    *encryptedSize = me->EncryptedSize(plaintextSize);

    return MUM_ERROR_OK;
}

void MumDestroyEngine(void *mev)
{
    CMumEngine *me = (CMumEngine *)mev;
    delete me;
}

EMumError MumPlaintextBlockSize(void *mev, uint32_t *plaintextBlockSize)
{
    CMumEngine *me = (CMumEngine *)mev;
    *plaintextBlockSize = me->PlaintextBlockSize();
    return MUM_ERROR_OK;
}

EMumError MumEncryptedBlockSize(void *mev, uint32_t *encryptedBlockSize)
{
    CMumEngine *me = (CMumEngine *)mev;
    *encryptedBlockSize = me->EncryptedBlockSize();
    return MUM_ERROR_OK;
}

EMumError MumInitKey(void *mev, uint8_t *key)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->InitKey(key);
}

EMumError MumLoadKey(void *mev, const char *keyfile)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->LoadKey(keyfile);
}

EMumError MumEncryptFile(void *mev, const char *srcfile, const char *dstfile)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->EncryptFile(srcfile, dstfile);
}

EMumError MumDecryptFile(void *mev, const char *srcfile, const char *dstfile)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->DecryptFile(srcfile, dstfile);
}

EMumError MumEncrypt(void *mev, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->Encrypt(src, dst, length, outlength, seqNum);
}

EMumError MumDecrypt(void *mev, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->Decrypt(src, dst, length, outlength);
}

EMumError MumEncryptBlock(void *mev, uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->EncryptBlock(src, dst, length, seqnum);
}

EMumError MumDecryptBlock(void *mev, uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->DecryptBlock(src, dst, length, seqnum);
}

EMumError MumGetSubkey(void *mev, uint32_t index, uint8_t *subkey)
{
    CMumEngine *me = (CMumEngine *)mev;
    return me->GetSubkey(index, subkey);
}

void *MumCreateEngine(EMumEngineType engineType, EMumBlockType blockType, EMumPaddingType paddingType, uint32_t numThreads)
{
    if (engineType < MUM_ENGINE_TYPE_CPU)
        return NULL;
#ifdef USE_MUM_OPENGL
    if (engineType > MUM_ENGINE_TYPE_GPU_B)
        return NULL;
#else
    if (engineType > MUM_ENGINE_TYPE_CPU)
        return NULL;
#endif
    CMumEngine *me = new CMumEngine(engineType, blockType, paddingType, numThreads);
    return me;
}

EMumError MumCreateEncryptedFileName(EMumBlockType blockType, const char *infilename, char *outfilename, size_t outlength)
{
    sprintf(outfilename, "%s.mu%d", infilename, blockType);
    return MUM_ERROR_OK;
}

EMumError MumGetInfoFromEncryptedFileName(const char *infilename, EMumBlockType *blockType, char *outfilename, size_t outlength)
{
    size_t len = strlen(infilename);
    if (len < 4)
        return MUM_ERROR_INVALID_FILE_EXTENSION;

    char ext[8];
    memcpy(ext, &infilename[len - 4], 4);
    ext[4] = 0;

    if (!strcmp(ext, ".mu1"))
        *blockType = MUM_BLOCKTYPE_128;
    else if (!strcmp(ext, ".mu2"))
        *blockType = MUM_BLOCKTYPE_256;
    else if (!strcmp(ext, ".mu3"))
        *blockType = MUM_BLOCKTYPE_512;
    else if (!strcmp(ext, ".mu4"))
        *blockType = MUM_BLOCKTYPE_1024;
    else if (!strcmp(ext, ".mu5"))
        *blockType = MUM_BLOCKTYPE_2048;
    else if (!strcmp(ext, ".mu6"))
        *blockType = MUM_BLOCKTYPE_4096;
    else
        return MUM_ERROR_INVALID_FILE_EXTENSION;

    if (outlength < len - 3)
        return MUM_ERROR_LENGTH_TOO_SMALL;

    memcpy(outfilename, infilename, len - 4);
    outfilename[len - 4] = 0;
    return MUM_ERROR_OK;
}
