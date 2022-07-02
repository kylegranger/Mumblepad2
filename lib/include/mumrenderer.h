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


#ifndef MUMRENDERER_H
#define MUMRENDERER_H

#include "mumdefines.h"
#include "mumprng.h"

class CMumRenderer {

public:

    CMumRenderer(TMumInfo *mumInfo);
    virtual ~CMumRenderer();

    virtual EMumError EncryptBlock(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum);
    virtual EMumError DecryptBlock(uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum);
    virtual EMumError Encrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum);
    virtual EMumError Decrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength);


    virtual void EncryptDiffuse(uint32_t round) = 0;
    virtual void EncryptConfuse(uint32_t round) = 0;
    virtual void DecryptConfuse(uint32_t round) = 0;
    virtual void DecryptDiffuse(uint32_t round) = 0;
    virtual void EncryptUpload(uint8_t *data) = 0;
    virtual void EncryptDownload(uint8_t *data) = 0;
    virtual void DecryptUpload(uint8_t *data) = 0;
    virtual void DecryptDownload(uint8_t *data) = 0;
    virtual void InitKey() = 0;

    void ResetEncryption() { numEncryptedBlocks = 0; }
    void ResetDecryption() { numDecryptedBlocks = 0; }
protected:
    TMumInfo *mMumInfo;
    CMumPrng *mPrng;
    int64_t numEncryptedBlocks;
    int64_t numDecryptedBlocks;
    int64_t blockLatency;
    uint8_t  mPackedData[MUM_MAX_BLOCK_SIZE];
    uint8_t mPingPongBlock[2][MUM_MAX_BLOCK_SIZE];
    uint8_t mPadding[MUM_PADDING_SIZE_R32];


    uint32_t ComputeChecksum(uint8_t *data, uint32_t size);
    void SetPadding(uint8_t *src, uint32_t length);

    EMumError(CMumRenderer::*packData)(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError(CMumRenderer::*unpackData)(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError PackDataR32(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError PackDataR16(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError PackDataR8(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError PackDataR4(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError PackDataR2(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError PackDataR1(uint8_t *unpackedData, uint32_t length, uint32_t seqnum);
    EMumError UnpackDataR32(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError UnpackDataR16(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError UnpackDataR8(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError UnpackDataR4(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError UnpackDataR2(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);
    EMumError UnpackDataR1(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum);

};


#endif

