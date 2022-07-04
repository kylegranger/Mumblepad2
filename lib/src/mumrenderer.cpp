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

#include <assert.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include "mumrenderer.h"

CMumRenderer::CMumRenderer(TMumInfo *mumInfo)
{
    mMumInfo = mumInfo;
    mPrng = nullptr;
    switch (mMumInfo->blockType)
    {
    case MUM_BLOCKTYPE_4096:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R32;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R32 : MUM_BLOCK_SIZE_R32;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R32;
        mMumInfo->numRows = 32;
        packData = &CMumRenderer::PackDataR32;
        unpackData = &CMumRenderer::UnpackDataR32;
        break;

    case MUM_BLOCKTYPE_2048:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R16;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R16 : MUM_BLOCK_SIZE_R16;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R16;
        mMumInfo->numRows = 16;
        packData = &CMumRenderer::PackDataR16;
        unpackData = &CMumRenderer::UnpackDataR16;
        break;

    case MUM_BLOCKTYPE_1024:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R8;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R8 : MUM_BLOCK_SIZE_R8;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R8;
        mMumInfo->numRows = 8;
        packData = &CMumRenderer::PackDataR8;
        unpackData = &CMumRenderer::UnpackDataR8;
        break;

    case MUM_BLOCKTYPE_512:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R4;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R4 : MUM_BLOCK_SIZE_R4;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R4;
        mMumInfo->numRows = 4;
        packData = &CMumRenderer::PackDataR4;
        unpackData = &CMumRenderer::UnpackDataR4;
        break;

    case MUM_BLOCKTYPE_256:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R2;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R2 : MUM_BLOCK_SIZE_R2;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R2;
        mMumInfo->numRows = 2;
        packData = &CMumRenderer::PackDataR2;
        unpackData = &CMumRenderer::UnpackDataR2;
        break;

    case MUM_BLOCKTYPE_128:
        mMumInfo->encryptedBlockSize = MUM_BLOCK_SIZE_R1;
        mMumInfo->plaintextBlockSize = mMumInfo->paddingOn ? MUM_ENCRYPT_SIZE_R1 : MUM_BLOCK_SIZE_R1;
        mMumInfo->paddingSize = MUM_PADDING_SIZE_R1;
        mMumInfo->numRows = 1;
        packData = &CMumRenderer::PackDataR1;
        unpackData = &CMumRenderer::UnpackDataR1;
        break;
    }

    numEncryptedBlocks = 0;
    numDecryptedBlocks = 0;
    blockLatency = (mMumInfo->engineType < MUM_ENGINE_TYPE_GPU_B) ? 0 : 7;
}

CMumRenderer::~CMumRenderer()
{
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
}

EMumError CMumRenderer::Encrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum)
{
    uint32_t encryptSize = 0;
    EMumError error = MUM_ERROR_OK;
    uint32_t latency = 0;
    uint8_t dummy[MUM_MAX_BLOCK_SIZE];

    *outlength = 0;
    while (length > 0)
    {
        if (length >= mMumInfo->plaintextBlockSize)
            encryptSize = mMumInfo->plaintextBlockSize;
        else
        {
            encryptSize = length;
            memcpy(dummy, src, encryptSize);
            src = dummy;
        }
        length -= encryptSize;
        error = EncryptBlock(src, dst, encryptSize, seqNum++);
        if (error == MUM_ERROR_BUFFER_WAIT_ENCRYPT)
            latency++;
        else if (error != MUM_ERROR_OK)
            return error;
        else
        {
            dst += mMumInfo->encryptedBlockSize;
            *outlength += mMumInfo->encryptedBlockSize;
        }
        src += mMumInfo->plaintextBlockSize;
    }
    while (latency > 0)
    {
        uint8_t dummy[MUM_MAX_BLOCK_SIZE];
        memset(dummy, seqNum, MUM_MAX_BLOCK_SIZE);
        error = EncryptBlock(dummy, dst, encryptSize, seqNum++);
        if (error == MUM_ERROR_BUFFER_WAIT_ENCRYPT)
            continue;
        if (error != MUM_ERROR_OK)
            return error;
        dst += mMumInfo->encryptedBlockSize;
        *outlength += mMumInfo->encryptedBlockSize;
        latency--;
    }
    return error;
}

EMumError CMumRenderer::Decrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength)
{
    uint32_t seqnum = 0;
    EMumError error = MUM_ERROR_OK;
    uint32_t latency = 0;
    uint8_t *firstBlock = src;

    if ((length % mMumInfo->encryptedBlockSize) != 0)
        return MUM_ERROR_INVALID_DECRYPT_SIZE;

    *outlength = 0;
    while (length)
    {
        uint32_t encryptSize = 0;
        EMumError error = DecryptBlock(src, dst, &encryptSize, &seqnum);
        if (error == MUM_ERROR_BUFFER_WAIT_DECRYPT)
            latency++;
        else if (error != MUM_ERROR_OK)
            return error;
        else
        {
            dst += encryptSize;
            *outlength += encryptSize;
        }
        src += mMumInfo->encryptedBlockSize;
        length -= mMumInfo->encryptedBlockSize;
    }
    while (latency > 0)
    {
        uint32_t encryptSize = 0;
        error = DecryptBlock(firstBlock, dst, &encryptSize, &seqnum);
        if (error == MUM_ERROR_BUFFER_WAIT_DECRYPT)
            continue;
        if (error != MUM_ERROR_OK)
            return error;
        dst += encryptSize;
        *outlength += encryptSize;
        latency--;
    }
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::EncryptBlock(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum)
{
    if (mMumInfo->paddingOn)
    {
        SetPadding(src, length);
        EMumError error = (this->*packData)(src, length, seqnum);
        if (error != MUM_ERROR_OK)
            return error;
        EncryptUpload(mPackedData);
    }
    else
    {
        EncryptUpload(src);
    }

    for (uint32_t r = 0; r < mMumInfo->numRoundsPerBlock; r++)
    {
        EncryptDiffuse(r);
        EncryptConfuse(r);
    }

    EncryptDownload(dst);

    numEncryptedBlocks++;
    if (numEncryptedBlocks <= blockLatency)
        return MUM_ERROR_BUFFER_WAIT_ENCRYPT;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::DecryptBlock(uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum)
{
    DecryptUpload(src);
    for (int r = mMumInfo->numRoundsPerBlock - 1; r >= 0; r--)
    {
        DecryptConfuse((uint32_t)r);
        DecryptDiffuse((uint32_t)r);
    }
    numDecryptedBlocks++;
    if (numDecryptedBlocks <= blockLatency)
        return MUM_ERROR_BUFFER_WAIT_DECRYPT;
    if (mMumInfo->paddingOn)
    {
        DecryptDownload(mPackedData);
        EMumError error = (this->*unpackData)(dst, length, seqnum);
        if (error != MUM_ERROR_OK)
            return error;
    }
    else
    {
        *length = mMumInfo->plaintextBlockSize;
        DecryptDownload(dst);
    }
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR32(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR32 *block = (TMumBlockR32 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R32)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R32);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 32);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R32);
    unpackedData += MUM_BLOCK_SIZE_A_R32;

    // second part of paddingxx
    memcpy(block->paddingB, &mPadding[32], 12);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_4096 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[44], 12);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R32);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[56], 32);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR32(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR32 *block = (TMumBlockR32 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R32);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R32, block->dataB, MUM_BLOCK_SIZE_B_R32);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_4096)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R32)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R32);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR16(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR16 *block = (TMumBlockR16 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R16)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R16);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 16);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R16);
    unpackedData += MUM_BLOCK_SIZE_A_R16;

    // second part of padding
    memcpy(block->paddingB, &mPadding[16], 4);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_2048 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[20], 4);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R16);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[24], 16);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR16(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR16 *block = (TMumBlockR16 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R16);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R16, block->dataB, MUM_BLOCK_SIZE_B_R16);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_2048)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R16)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R16);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR8(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR8 *block = (TMumBlockR8 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R8)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R8);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 4);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R8);
    unpackedData += MUM_BLOCK_SIZE_A_R8;

    // second part of padding
    memcpy(block->paddingB, &mPadding[4], 4);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_1024 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[8], 4);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R8);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[12], 4);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR8(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR8 *block = (TMumBlockR8 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R8);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R8, block->dataB, MUM_BLOCK_SIZE_B_R8);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_1024)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R8)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R8);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR4(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR4 *block = (TMumBlockR4 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R4)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R4);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 2);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R4);
    unpackedData += MUM_BLOCK_SIZE_A_R4;

    // second part of padding
    memcpy(block->paddingB, &mPadding[2], 2);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_512 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[4], 2);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R4);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[6], 2);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR4(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR4 *block = (TMumBlockR4 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R4);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R4, block->dataB, MUM_BLOCK_SIZE_B_R4);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_512)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R4)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R4);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR2(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR2 *block = (TMumBlockR2 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R2)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R2);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 2);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R2);
    unpackedData += MUM_BLOCK_SIZE_A_R2;

    // second part of padding
    memcpy(block->paddingB, &mPadding[2], 2);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_256 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[4], 2);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R2);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[6], 2);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR2(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR2 *block = (TMumBlockR2 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R2);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R2, block->dataB, MUM_BLOCK_SIZE_B_R2);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_256)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R2)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R2);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::PackDataR1(uint8_t *unpackedData, uint32_t length, uint32_t seqnum)
{
    TMumBlockR1 *block = (TMumBlockR1 *)mPackedData;

    if (length > MUM_ENCRYPT_SIZE_R1)
        return MUM_ERROR_INVALID_ENCRYPT_SIZE;

    uint32_t checksum = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R1);

    // first third of padding
    memcpy(block->paddingA, &mPadding[0], 2);

    // part A of data
    memcpy(block->dataA, unpackedData, MUM_BLOCK_SIZE_A_R1);
    unpackedData += MUM_BLOCK_SIZE_A_R1;

    // second part of padding
    memcpy(block->paddingB, &mPadding[2], 2);

    block->checksum[0] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[1] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[2] = (uint8_t)checksum;
    checksum >>= 8;
    block->checksum[3] = (uint8_t)checksum;
    length += (MUM_BLOCKTYPE_128 << MUM_LENGTH_BLOCKTYPE_SHIFT);
    block->length[0] = (uint8_t)(length % 256);
    block->length[1] = (uint8_t)(length / 256);
    block->seqnum[0] = (uint8_t)(seqnum % 256);
    block->seqnum[1] = (uint8_t)(seqnum / 256);

    // third part of padding
    memcpy(block->paddingC, &mPadding[4], 2);

    // second part of data
    memcpy(block->dataB, unpackedData, MUM_BLOCK_SIZE_B_R1);

    // fourth part of padding
    memcpy(block->paddingD, &mPadding[6], 2);
    return MUM_ERROR_OK;
}

EMumError CMumRenderer::UnpackDataR1(uint8_t *unpackedData, uint32_t *length, uint32_t *seqnum)
{
    TMumBlockR1 *block = (TMumBlockR1 *)mPackedData;

    memcpy(unpackedData, block->dataA, MUM_BLOCK_SIZE_A_R1);
    memcpy(unpackedData + MUM_BLOCK_SIZE_A_R1, block->dataB, MUM_BLOCK_SIZE_B_R1);

    uint32_t lengthField = block->length[0];
    lengthField += block->length[1] << 8;
    if (((lengthField & MUM_LENGTH_BLOCKTYPE_MASK) >> MUM_LENGTH_BLOCKTYPE_SHIFT) != MUM_BLOCKTYPE_128)
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_BLOCKTYPE;
    *length = (lengthField & MUM_LENGTH_LENGTH_MASK);
    if (*length > MUM_ENCRYPT_SIZE_R1)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_LENGTH;
    }

    uint32_t checksumA = block->checksum[0];
    checksumA += block->checksum[1] << 8;
    checksumA += block->checksum[2] << 16;
    checksumA += block->checksum[3] << 24;
    uint32_t checksumB = ComputeChecksum(unpackedData, MUM_ENCRYPT_SIZE_R1);
    if (checksumA != checksumB)
    {
        *length = 0;
        return MUM_ERROR_INVALID_ENCRYPTED_BLOCK_CHECKSUM;
    }

    *seqnum = block->seqnum[0];
    *seqnum += block->seqnum[1] << 8;
    return MUM_ERROR_OK;
}

uint32_t CMumRenderer::ComputeChecksum(uint8_t *data, uint32_t size)
{
    uint32_t checksum = 0;
    uint32_t *iptr = (uint32_t *)data;
    for (uint32_t i = 0; i < size / 4; i++)
        checksum += *iptr++;
    return checksum;
}

void CMumRenderer::SetPadding(uint8_t *src, uint32_t length)
{
    mPrng->Fetch(mPadding, mMumInfo->paddingSize);
    if (length < mMumInfo->plaintextBlockSize)
        mPrng->Fetch(&src[length], mMumInfo->plaintextBlockSize - length);
}
