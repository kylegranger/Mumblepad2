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

#ifndef MUMDEFINES_H
#define MUMDEFINES_H

#include "mumpublic.h"

#define MUM_NUM_ROUNDS 8
#define MUM_KEY_MASK        4095
// 8 subkeys, 1 per round for confusion pass, XOR
// 8 subkeyss, 1 per round for 3-bit tables, diffusion pass bitmasks
// 8 * 32 subkeys, 1 per row and per round confusion pass
// 8 * 4 subkeys, 1 per color channel and per round for diffusion pass.
// number of color channels in RGBA pixel
#define MUN_NUM_POSITIONS 4
#define MUM_NUM_CYCLES 7
#define MUM_CYCLE_INDEX_INCREMENT 3
#define MUM_CYCLE_OFFSET_INCREMENT 5
#define MUM_BLOCK_INFO_SIZE    8

#define MUM_LENGTH_LENGTH_MASK     0x00001fff
#define MUM_LENGTH_BLOCKTYPE_MASK  0x0000e000
#define MUM_LENGTH_BLOCKTYPE_SHIFT 13

#define MUM_BLOCK_SIZE_R32     4096
#define MUM_ENCRYPT_SIZE_R32   4000
#define MUM_BLOCK_SIZE_A_R32   2472
#define MUM_BLOCK_SIZE_B_R32   1528
#define MUM_PADDING_SIZE_R32   88


#define MUM_BLOCK_SIZE_R16     2048
#define MUM_ENCRYPT_SIZE_R16   2000
#define MUM_BLOCK_SIZE_A_R16   1236
#define MUM_BLOCK_SIZE_B_R16   764
#define MUM_PADDING_SIZE_R16   40

#define MUM_BLOCK_SIZE_R8     1024
#define MUM_ENCRYPT_SIZE_R8   1000
#define MUM_BLOCK_SIZE_A_R8   618
#define MUM_BLOCK_SIZE_B_R8   382
#define MUM_PADDING_SIZE_R8   16

#define MUM_BLOCK_SIZE_R4     512
#define MUM_ENCRYPT_SIZE_R4   492
#define MUM_BLOCK_SIZE_A_R4   304
#define MUM_BLOCK_SIZE_B_R4   188
#define MUM_PADDING_SIZE_R4   12

#define MUM_BLOCK_SIZE_R2     256
#define MUM_ENCRYPT_SIZE_R2   240
#define MUM_BLOCK_SIZE_A_R2   148
#define MUM_BLOCK_SIZE_B_R2   92
#define MUM_PADDING_SIZE_R2   8

#define MUM_BLOCK_SIZE_R1     128
#define MUM_ENCRYPT_SIZE_R1   112
#define MUM_BLOCK_SIZE_A_R1   72
#define MUM_BLOCK_SIZE_B_R1   40
#define MUM_PADDING_SIZE_R1   8

#define MUM_CELLS_X             32
#define MUM_CELLS_MAX_Y         32
// for gpub 32*8
#define MUM_CELLS_YB        256
#define MUM_CELL_SIZE       4
#define MUM_NUM_POSITIONS   4
// padding is 4096-4000-8
// #define MUM_PADDING_SIZE    88

#define MUM_MASK_TABLE_ROWS 32

#define MUM_NUM_3BIT_VALUES      8
#define MUM_NUM_8BIT_VALUES    256
#define MUM_MAX_10BIT_VALUES  1024

// total = 4096 bytes
// payload = 4000
// padding = 88 (32/12/12/32)
// info = 8
typedef struct TMumBlockR32
{
    uint8_t paddingA[32];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R32];
    uint8_t paddingB[12];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[12];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R32];
    uint8_t paddingD[32];
} TMumBlockR32;

// total = 2048 bytes
// payload = 2000
// padding = 40 (16/4/4/16)
// info = 8
typedef struct TMumBlockR16
{
    uint8_t paddingA[16];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R16];
    uint8_t paddingB[4];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[4];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R16];
    uint8_t paddingD[16];
} TMumBlockR16;

// total = 1024 bytes
// payload = 1000
// padding = 16 (4/4/4/4)
// info = 8
typedef struct TMumBlockR8
{
    uint8_t paddingA[4];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R8];
    uint8_t paddingB[4];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[4];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R8];
    uint8_t paddingD[4];
} TMumBlockR8;

// total = 512 bytes
// payload = 496
// padding = 8 (2/2/2/2)
// info = 8
typedef struct TMumBlockR4
{
    uint8_t paddingA[2];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R4];
    uint8_t paddingB[4];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[4];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R4];
    uint8_t paddingD[2];
} TMumBlockR4;

// total = 256 bytes
// payload = 240
// padding = 8 (2/2/2/2)
// info = 8
typedef struct TMumBlockR2
{
    uint8_t paddingA[2];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R2];
    uint8_t paddingB[2];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[2];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R2];
    uint8_t paddingD[2];
} TMumBlockR2;

// total = 112 bytes
// payload = 112
// padding = 8 (2/2/2/2)
// info = 8
typedef struct TMumBlockR1
{
    uint8_t paddingA[2];
    uint8_t dataA[MUM_BLOCK_SIZE_A_R1];
    uint8_t paddingB[2];
    uint8_t checksum[4];
    uint8_t length[2];
    uint8_t seqnum[2];
    uint8_t paddingC[2];
    uint8_t dataB[MUM_BLOCK_SIZE_B_R1];
    uint8_t paddingD[2];
} TMumBlockR1;

typedef struct TMumInfo 
{
    EMumEngineType engineType;
    EMumBlockType blockType;
    bool paddingOn;
    bool keyInitialized;
    uint32_t numRows;
    uint32_t plaintextBlockSize;
    uint32_t encryptedBlockSize;
    uint32_t paddingSize;
    uint32_t numRoundsPerBlock;

    uint8_t key[MUM_KEY_SIZE];
    uint8_t subkeys[MUM_NUM_SUBKEYS][MUM_KEY_SIZE];

    // permutation tables
    uint32_t permuteTables3bit[MUM_NUM_ROUNDS][MUM_NUM_3BIT_VALUES];
    uint32_t permuteTables8bit[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_NUM_8BIT_VALUES];
    uint32_t permuteTables8bitI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_NUM_8BIT_VALUES];
    uint32_t permuteTables10bit[MUM_NUM_ROUNDS][MUN_NUM_POSITIONS][MUM_MAX_10BIT_VALUES];


    // tables derived from permuation tables
    uint32_t bitmasks[MUM_NUM_ROUNDS][4];
    uint32_t positionTables5bitX[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint32_t positionTables5bitY[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint32_t positionTables5bitXI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint32_t positionTables5bitYI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];

    // precomputed texture data, 8-bit unsigned
    uint8_t bitmaskTextureData[MUM_NUM_ROUNDS][MUM_MASK_TABLE_ROWS*MUM_NUM_8BIT_VALUES];
    uint8_t permuteTextureData[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_NUM_8BIT_VALUES];
    uint8_t permuteTextureDataI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_NUM_8BIT_VALUES];
    uint8_t positionTextureDataX[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t positionTextureDataY[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t positionTextureDataYB[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t positionTextureDataXI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t positionTextureDataYI[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t positionTextureDataYIB[MUM_NUM_ROUNDS][MUM_CELLS_MAX_Y][MUM_CELLS_X][MUM_NUM_POSITIONS];
    uint8_t xorTextureData[MUM_NUM_8BIT_VALUES*MUM_NUM_8BIT_VALUES];
} TMumInfo;


#endif

