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

#ifndef MUMPRNG_H
#define MUMPRNG_H

#include "mumdefines.h"

#define MUM_PRNG_SUBKEY_SIZE   (MUM_KEY_SIZE*16)
#define MUM_PRNG_SEED1 0xb11924e1
#define MUM_PRNG_SEED2 0x6d73e55f


class CMumPrng
{
public:
    CMumPrng(uint8_t *subkeyData);
    ~CMumPrng();
    void Fetch(uint8_t *dst, uint32_t size);

private:
    void Init();
    void Regenerate();
    void Generate(uint8_t *dst, uint32_t size);
    void XorWithSubkey();
    uint8_t mState[256];
    uint32_t mA;
    uint32_t mB;
    uint32_t mReadIndex;
    uint8_t mSubkeyData[MUM_PRNG_SUBKEY_SIZE];
    uint8_t mReadyData[MUM_PRNG_SUBKEY_SIZE];


} ;

#endif

