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

#include "mumblepad.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

CMumblepad::CMumblepad(TMumInfo *mumInfo) : CMumRenderer(mumInfo)
{
    mMumInfo = mumInfo;
}

CMumblepad::~CMumblepad()
{
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
}

void CMumblepad::InitKey()
{
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
    mPrng = new CMumPrng(mMumInfo->subkeys[MUM_PRNG_SUBKEY_INDEX]);
}

void CMumblepad::EncryptUpload(uint8_t *data)
{
    memcpy(mPingPongBlock[0], data, mMumInfo->encryptedBlockSize);
}

void CMumblepad::DecryptUpload(uint8_t *data)
{
    EncryptUpload(data);
}

void CMumblepad::EncryptDiffuse(uint32_t round)
{
    uint32_t x, y, srcPosX1, srcPosY1, srcPosX2, srcPosY2, srcPosX3, srcPosY3, srcPosX4, srcPosY4;
    uint32_t maskA, maskB, maskC, maskD;
    uint8_t *mappedSrc1, *mappedSrc2, *mappedSrc3, *mappedSrc4;
    uint8_t *src;
    uint8_t *dst;
    uint32_t numRows = mMumInfo->numRows;

    // first pass for encrypt
    // source = 0, destination = 1
    src = mPingPongBlock[0];
    dst = mPingPongBlock[1];

    maskA = mMumInfo->bitmasks[round][0];
    maskB = mMumInfo->bitmasks[round][1];
    maskC = mMumInfo->bitmasks[round][2];
    maskD = mMumInfo->bitmasks[round][3];
    for (y = 0; y < numRows; y++)
    {
        for (x = 0; x < MUM_CELLS_X; x++)
        {
            srcPosX1 = mMumInfo->positionTables5bitX[round][y][x][0];
            srcPosY1 = mMumInfo->positionTables5bitY[round][y][x][0];
            mappedSrc1 = src + srcPosX1 * MUM_CELL_SIZE + srcPosY1 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX2 = mMumInfo->positionTables5bitX[round][y][x][1];
            srcPosY2 = mMumInfo->positionTables5bitY[round][y][x][1];
            mappedSrc2 = src + srcPosX2 * MUM_CELL_SIZE + srcPosY2 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX3 = mMumInfo->positionTables5bitX[round][y][x][2];
            srcPosY3 = mMumInfo->positionTables5bitY[round][y][x][2];
            mappedSrc3 = src + srcPosX3 * MUM_CELL_SIZE + srcPosY3 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX4 = mMumInfo->positionTables5bitX[round][y][x][3];
            srcPosY4 = mMumInfo->positionTables5bitY[round][y][x][3];
            mappedSrc4 = src + srcPosX4 * MUM_CELL_SIZE + srcPosY4 * MUM_CELLS_X * MUM_CELL_SIZE;
            dst[0] = (mappedSrc1[0] & maskA) + (mappedSrc2[2] & maskB) + (mappedSrc3[3] & maskC) + (mappedSrc4[1] & maskD);
            dst[1] = (mappedSrc1[2] & maskA) + (mappedSrc2[3] & maskB) + (mappedSrc3[1] & maskC) + (mappedSrc4[0] & maskD);
            dst[2] = (mappedSrc1[3] & maskA) + (mappedSrc2[1] & maskB) + (mappedSrc3[0] & maskC) + (mappedSrc4[2] & maskD);
            dst[3] = (mappedSrc1[1] & maskA) + (mappedSrc2[0] & maskB) + (mappedSrc3[2] & maskC) + (mappedSrc4[3] & maskD);
            dst += 4;
        }
    }
}

void CMumblepad::EncryptConfuse(uint32_t round)
{
    uint32_t x, y;
    uint8_t *clav;
    uint8_t *src;
    uint8_t *dst;
    uint32_t *prm;
    uint32_t numRows = mMumInfo->numRows;

    // second pass for encrypt
    // source = 1, destination = 0
    src = mPingPongBlock[1];
    dst = mPingPongBlock[0];
    clav = mMumInfo->subkeys[round];

    for (y = 0; y < numRows; y++)
    {
        prm = mMumInfo->permuteTables8bit[round][y];
        for (x = 0; x < MUM_CELLS_X; x++)
        {
            *dst++ = (uint8_t)prm[(uint8_t)(*src++ ^ *clav++)];
            *dst++ = (uint8_t)prm[(uint8_t)(*src++ ^ *clav++)];
            *dst++ = (uint8_t)prm[(uint8_t)(*src++ ^ *clav++)];
            *dst++ = (uint8_t)prm[(uint8_t)(*src++ ^ *clav++)];
        }
    }
}

void CMumblepad::DecryptConfuse(uint32_t round)
{
    uint32_t x, y;
    uint8_t *src, *dst, *clav;
    uint32_t *prm;
    uint32_t numRows = mMumInfo->numRows;

    // first pass for decrypt
    // source = 0, destination = 1
    src = mPingPongBlock[0];
    dst = mPingPongBlock[1];

    clav = mMumInfo->subkeys[round];
    for (y = 0; y < numRows; y++)
    {
        prm = mMumInfo->permuteTables8bitI[round][y];
        for (x = 0; x < MUM_CELLS_X; x++)
        {
            *dst++ = (uint8_t)prm[*src++] ^ *clav++;
            *dst++ = (uint8_t)prm[*src++] ^ *clav++;
            *dst++ = (uint8_t)prm[*src++] ^ *clav++;
            *dst++ = (uint8_t)prm[*src++] ^ *clav++;
        }
    }
}

void CMumblepad::DecryptDiffuse(uint32_t round)
{
    uint32_t x, y, srcPosX1, srcPosY1, srcPosX2, srcPosY2, srcPosX3, srcPosY3, srcPosX4, srcPosY4;
    uint32_t maskA, maskB, maskC, maskD;
    uint8_t *src;
    uint8_t *dst;
    uint8_t *mappedSrc1;
    uint8_t *mappedSrc2;
    uint8_t *mappedSrc3;
    uint8_t *mappedSrc4;
    uint32_t numRows = mMumInfo->numRows;

    // second pass for decrypt
    // source = 1, destination = 0
    src = mPingPongBlock[1];
    dst = mPingPongBlock[0];

    maskA = mMumInfo->bitmasks[round][0];
    maskB = mMumInfo->bitmasks[round][1];
    maskC = mMumInfo->bitmasks[round][2];
    maskD = mMumInfo->bitmasks[round][3];
    for (y = 0; y < numRows; y++)
    {
        for (x = 0; x < MUM_CELLS_X; x++)
        {
            srcPosX1 = mMumInfo->positionTables5bitXI[round][y][x][0];
            srcPosY1 = mMumInfo->positionTables5bitYI[round][y][x][0];
            mappedSrc1 = src + srcPosX1 * MUM_CELL_SIZE + srcPosY1 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX2 = mMumInfo->positionTables5bitXI[round][y][x][1];
            srcPosY2 = mMumInfo->positionTables5bitYI[round][y][x][1];
            mappedSrc2 = src + srcPosX2 * MUM_CELL_SIZE + srcPosY2 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX3 = mMumInfo->positionTables5bitXI[round][y][x][2];
            srcPosY3 = mMumInfo->positionTables5bitYI[round][y][x][2];
            mappedSrc3 = src + srcPosX3 * MUM_CELL_SIZE + srcPosY3 * MUM_CELLS_X * MUM_CELL_SIZE;
            srcPosX4 = mMumInfo->positionTables5bitXI[round][y][x][3];
            srcPosY4 = mMumInfo->positionTables5bitYI[round][y][x][3];
            mappedSrc4 = src + srcPosX4 * MUM_CELL_SIZE + srcPosY4 * MUM_CELLS_X * MUM_CELL_SIZE;
            *dst++ = (mappedSrc1[0] & maskA) + (mappedSrc2[3] & maskB) + (mappedSrc3[2] & maskC) + (mappedSrc4[1] & maskD);
            *dst++ = (mappedSrc1[3] & maskA) + (mappedSrc2[2] & maskB) + (mappedSrc3[1] & maskC) + (mappedSrc4[0] & maskD);
            *dst++ = (mappedSrc1[1] & maskA) + (mappedSrc2[0] & maskB) + (mappedSrc3[3] & maskC) + (mappedSrc4[2] & maskD);
            *dst++ = (mappedSrc1[2] & maskA) + (mappedSrc2[1] & maskB) + (mappedSrc3[0] & maskC) + (mappedSrc4[3] & maskD);
        }
    }
}

void CMumblepad::EncryptDownload(uint8_t *data)
{
    memcpy(data, mPingPongBlock[0], mMumInfo->encryptedBlockSize);
}
void CMumblepad::DecryptDownload(uint8_t *data)
{
    EncryptDownload(data);
}
