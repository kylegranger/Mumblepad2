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

#ifndef MUMENGINE_H
#define MUMENGINE_H

#include "mumdefines.h"
#include "mumprng.h"
#include "mumrenderer.h"
#ifdef USE_MUM_OPENGL
#include "mumglwrapper.h"
#endif

class CMumEngine
{
public:
    CMumEngine(EMumEngineType engineType, EMumBlockType blockType, EMumPaddingType paddingType, uint32_t numThreads);
    ~CMumEngine();
    EMumError InitKey(uint8_t *key);
    EMumError LoadKey(const char *keyfile);
    EMumError GetSubkey(uint32_t index, uint8_t *subkey);
    uint32_t PlaintextBlockSize();
    uint32_t EncryptedBlockSize();
    uint32_t EncryptedSize(uint32_t plaintextSize);
    EMumError EncryptBlock(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum);
    EMumError DecryptBlock(uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum);

    EMumError EncryptFile(const char *srcfile, const char *dstfile);
    EMumError DecryptFile(const char *srcfile, const char *dstfile);
    EMumError Encrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum);
    EMumError Decrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength);

private:
    TMumInfo mMumInfo;
    CMumRenderer *mMumRenderer;
    uint32_t GetSubkeyInteger(uint8_t *subkey, uint32_t offset);
    void InitXorTextureData();
    void CreatePermuteTable(uint8_t *subkey, uint32_t numEntries, uint32_t *outTable);
    void CreatePrimeCycleWithOffset(uint32_t primeIndex, uint32_t offset, uint8_t *outCycle);

#ifdef USE_MUM_OPENGL
    static CMumGlWrapper *mMumGlWrapper;
#endif

    void InitSubkeys();
    void InitPermuteTables();
    void InitPositionTables();
    void InitBitmasks();
};


#endif

