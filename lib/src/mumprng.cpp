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
#include <stdio.h>
#include <stdlib.h>
#include "mumprng.h"



CMumPrng::CMumPrng(uint8_t *subkeyData)
{
    memcpy(mSubkeyData, subkeyData, MUM_PRNG_SUBKEY_SIZE);
    memset(mReadyData, 0, MUM_PRNG_SUBKEY_SIZE);
    mReadIndex = 0;
    Init();
    Regenerate();
}

CMumPrng::~CMumPrng()
{
}


static __inline void swap(uint8_t *a, uint8_t *b)
{
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

void CMumPrng::Init()
{
    mA = 0;
    mB = 0;

    // RC4 initialization
    for (int i = 0; i < 256; i++)
        mState[i] = i;

    // our subkey area is 64KB -- for the state initialization we will
    // use a 256-byte from there, 89 bytes before the end.
    uint8_t *prngKey = &mSubkeyData[MUM_PRNG_SUBKEY_SIZE - 256 - 89];
    uint32_t j = 0;
    for (int i = 0; i < 256; i++)
    {
        j = (j + mState[i] + prngKey[i]) & 255;
        swap(&mState[i], &mState[j]);
    }
}

void CMumPrng::Fetch(uint8_t *dst, uint32_t size)
{
    if (size > (MUM_PRNG_SUBKEY_SIZE - mReadIndex))
        Regenerate();
    memcpy(dst, mReadyData+mReadIndex,size);
    mReadIndex += size;
}


void CMumPrng::XorWithSubkey()
{
    uint32_t *src = (uint32_t*) mReadyData;
    uint32_t *sbk = (uint32_t*) mSubkeyData;
    for (uint32_t i = 0; i < MUM_PRNG_SUBKEY_SIZE / 4; i++)
        *src++ ^= *sbk++;
}

void CMumPrng::Regenerate()
{
    Generate(mReadyData, MUM_PRNG_SUBKEY_SIZE);
    // every 64KB of stream generated gets XOR's with the subkey.
    XorWithSubkey();
    mReadIndex = 0;
}

void CMumPrng::Generate(uint8_t *dst, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        // RC4 stream generation
        mA = (mA + 1) & 255;
        mB = (mB + mState[mA]) & 255;
        swap(&mState[mA], &mState[mB]);
        uint32_t c = (mState[mA] + mState[mA]) & 255;
        dst[i] = mState[c];
    }
}



