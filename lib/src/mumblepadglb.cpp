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

#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include "assert.h"
#include "../gl/gl3w.h"
#include "mumblepadglb.h"

#ifdef USE_OPENGL

static float wholeSquareVertices[] = {
    -1.0f,
    1.0f,
    0.0f,
    -1.0f,
    -1.0f,
    0.0f,
    1.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
};

static float wholeSquareUv[] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f};

static float wholeSquareUvOffset[] = {
    0.0f, 0.875f,
    0.0f, -0.125f,
    1.0f, -0.125f,
    1.0f, 0.875f};

static unsigned short wholeSquareIndices[] = {0, 1, 2, 0, 2, 3};

static char vertexShaderText[] =
    "attribute vec4 a_position;   \n"
    "attribute vec2 a_texCoord;   \n"
    "varying vec2 v_texCoord;     \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = a_position; \n"
    "   v_texCoord = a_texCoord;  \n"
    "}                            \n";

static char nullShaderText[] =
    "varying vec2 v_texCoord;\n\
uniform sampler2D source;\n\
void main(void)\n\
{\n\
    gl_FragColor  = texture2D(source, v_texCoord);\n\
}";

static char encryptDiffuseText[] =
    "varying vec2 v_texCoord;\n\
uniform sampler2D source;\n\
uniform sampler2D bitmasks;\n\
uniform sampler2D positionX;\n\
uniform sampler2D positionY;\n\
void main(void)\n\
{\n\
    vec4 posX  = texture2D(positionX, v_texCoord);\n\
    vec4 posY  = texture2D(positionY, v_texCoord);\n\
    vec4 src1  = texture2D(source, vec2(posX[0],posY[0]));\n\
    vec4 src2  = texture2D(source, vec2(posX[1],posY[1]));\n\
    vec4 src3  = texture2D(source, vec2(posX[2],posY[2]));\n\
    vec4 src4  = texture2D(source, vec2(posX[3],posY[3]));\n\
    float round = floor((v_texCoord[1]) * 8.0);\n\
    float offset = round/8.0 + 0.015625;\n\
    gl_FragColor[0] = texture2D(bitmasks,vec2(src1[0],offset))[0] + \
        texture2D(bitmasks,vec2(src2[2],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[3],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[1],offset+0.09375))[0];\n\
    gl_FragColor[1] = texture2D(bitmasks,vec2(src1[2],offset))[0] + \
        texture2D(bitmasks,vec2(src2[3],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[1],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[0],offset+0.09375))[0];\n\
    gl_FragColor[2] = texture2D(bitmasks,vec2(src1[3],offset))[0] + \
        texture2D(bitmasks,vec2(src2[1],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[0],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[2],offset+0.09375))[0];\n\
    gl_FragColor[3] = texture2D(bitmasks,vec2(src1[1],offset))[0] + \
        texture2D(bitmasks,vec2(src2[0],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[2],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[3],offset+0.09375))[0];\n\
}";

static char encryptConfuseText[] =
    "varying vec2 v_texCoord;\n\
uniform sampler2D source;\n\
uniform sampler2D lutKey;\n\
uniform sampler2D lutXor;\n\
uniform sampler2D lutPermute;\n\
void main(void)\n\
{\n\
    vec4 clav = texture2D(lutKey, v_texCoord);\n\
    vec4 src  = texture2D(source, v_texCoord);\n\
    vec4 xorKey;\n\
    xorKey[0] = texture2D(lutXor,vec2(src.r,clav.r))[0];\n\
    xorKey[1] = texture2D(lutXor,vec2(src.g,clav.g))[0];\n\
    xorKey[2] = texture2D(lutXor,vec2(src.b,clav.b))[0];\n\
    xorKey[3] = texture2D(lutXor,vec2(src.a,clav.a))[0];\n\
    gl_FragColor[0] = texture2D(lutPermute,vec2(xorKey[0],v_texCoord[1]))[0];\n\
    gl_FragColor[1] = texture2D(lutPermute,vec2(xorKey[1],v_texCoord[1]))[0];\n\
    gl_FragColor[2] = texture2D(lutPermute,vec2(xorKey[2],v_texCoord[1]))[0];\n\
    gl_FragColor[3] = texture2D(lutPermute,vec2(xorKey[3],v_texCoord[1]))[0];\n\
}";

static char decryptDiffuseText[] =
    "varying vec2 v_texCoord;\n\
uniform sampler2D source;\n\
uniform sampler2D bitmasks;\n\
uniform sampler2D positionX;\n\
uniform sampler2D positionY;\n\
void main(void)\n\
{\n\
    vec4 posX  = texture2D(positionX, v_texCoord);\n\
    vec4 posY  = texture2D(positionY, v_texCoord);\n\
    vec4 src1  = texture2D(source, vec2(posX[0],posY[0]));\n\
    vec4 src2  = texture2D(source, vec2(posX[1],posY[1]));\n\
    vec4 src3  = texture2D(source, vec2(posX[2],posY[2]));\n\
    vec4 src4  = texture2D(source, vec2(posX[3],posY[3]));\n\
    float round = floor((v_texCoord[1]) * 8.0);\n\
    float offset = round/8.0 + 0.015625;\n\
    gl_FragColor[0] = texture2D(bitmasks,vec2(src1[0],offset))[0] + \
        texture2D(bitmasks,vec2(src2[3],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[2],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[1],offset+0.09375))[0];\n\
    gl_FragColor[1] = texture2D(bitmasks,vec2(src1[3],offset))[0] + \
        texture2D(bitmasks,vec2(src2[2],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[1],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[0],offset+0.09375))[0];\n\
    gl_FragColor[2] = texture2D(bitmasks,vec2(src1[1],offset))[0] + \
        texture2D(bitmasks,vec2(src2[0],offset+0.031255))[0] + \
        texture2D(bitmasks,vec2(src3[3],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[2],offset+0.09375))[0];\n\
    gl_FragColor[3] = texture2D(bitmasks,vec2(src1[2],offset))[0] + \
        texture2D(bitmasks,vec2(src2[1],offset+0.03125))[0] + \
        texture2D(bitmasks,vec2(src3[0],offset+0.0625))[0] + \
        texture2D(bitmasks,vec2(src4[3],offset+0.09375))[0];\n\
}";

static char decryptConfuseText[] =
    "varying vec2 v_texCoord;\n\
uniform sampler2D source;\n\
uniform sampler2D lutKey;\n\
uniform sampler2D lutXor;\n\
uniform sampler2D lutPermute;\n\
void main(void)\n\
{\n\
    vec4 clav = texture2D(lutKey, v_texCoord);\n\
    vec4 src  = texture2D(source, v_texCoord);\n\
    vec4 prm;\n\
    prm[0] = texture2D(lutPermute,vec2(src[0],v_texCoord[1]))[0];\n\
    prm[1] = texture2D(lutPermute,vec2(src[1],v_texCoord[1]))[0];\n\
    prm[2] = texture2D(lutPermute,vec2(src[2],v_texCoord[1]))[0];\n\
    prm[3] = texture2D(lutPermute,vec2(src[3],v_texCoord[1]))[0];\n\
    gl_FragColor[0] = texture2D(lutXor,vec2(prm.r,clav.r))[0];\n\
    gl_FragColor[1] = texture2D(lutXor,vec2(prm.g,clav.g))[0];\n\
    gl_FragColor[2] = texture2D(lutXor,vec2(prm.b,clav.b))[0];\n\
    gl_FragColor[3] = texture2D(lutXor,vec2(prm.a,clav.a))[0];\n\
}";

CMumblepadGlb::CMumblepadGlb(TMumInfo *mumInfo, CMumGlWrapper *gelbGlWrapper) : CMumRenderer(mumInfo)
{
    mMumInfo = mumInfo;
    mGlw = gelbGlWrapper;
    mEncryptDiffuseProgram = -1;
    mEncryptConfuseProgram = -1;
    mDecryptDiffuseProgram = -1;
    mDecryptConfuseProgram = -1;

    mPingPongTexture[0] = -1;
    mPingPongFBuffer[0] = -1;
    mPingPongTexture[1] = -1;
    mPingPongFBuffer[1] = -1;

    mLutTextureKey = -1;
    mLutTextureKeyI = -1;
    mLutTextureXor = -1;
    if (!InitTextures())
        assert(0);
}

void CMumblepadGlb::DeleteFrameBuffers()
{
    glDeleteTextures(2, mPingPongTexture);
    glDeleteFramebuffers(2, mPingPongFBuffer);
}

void CMumblepadGlb::DeleteLutTextures()
{
    glDeleteTextures(1, &mLutTextureKey);
    glDeleteTextures(1, &mLutTextureKeyI);
    glDeleteTextures(1, &mLutTextureXor);

    glDeleteTextures(1, &mLutTextureBitmask);
    glDeleteTextures(1, &mLutTextureBitmaskI);
    glDeleteTextures(1, &mPositionTexturesX);
    glDeleteTextures(1, &mPositionTexturesY);
    glDeleteTextures(1, &mPositionTexturesXI);
    glDeleteTextures(1, &mPositionTexturesYI);
    glDeleteTextures(1, &mLutTexturePermute);
    glDeleteTextures(1, &mLutTexturePermuteI);
}

CMumblepadGlb::~CMumblepadGlb()
{
    glDeleteProgram(mEncryptDiffuseProgram);
    glDeleteProgram(mEncryptConfuseProgram);
    glDeleteProgram(mDecryptDiffuseProgram);
    glDeleteProgram(mDecryptConfuseProgram);
    DeleteFrameBuffers();
    DeleteLutTextures();
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
}

void CMumblepadGlb::EncryptUpload(uint8_t *data)
{
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                    MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void CMumblepadGlb::DecryptUpload(uint8_t *data)
{
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 7 * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                    MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

bool CMumblepadGlb::CreateFrameBuffers()
{
    GLenum result;
    glGenTextures(2, mPingPongTexture);
    glGenFramebuffers(2, mPingPongFBuffer);

    uint32_t size = MUM_CELLS_X * MUM_CELLS_YB * 4;
    uint8_t *black = new uint8_t[size];
    memset(black, 0, size);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[0], 0);

    result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);
    delete[] black;
    result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool CMumblepadGlb::CreateLutTextures()
{
    glGenTextures(1, &mLutTextureKey);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKey);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mLutTextureKeyI);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKeyI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mLutTextureXor);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, MUM_NUM_8BIT_VALUES, MUM_NUM_8BIT_VALUES, 0, GL_RED,
                 GL_UNSIGNED_BYTE, mMumInfo->xorTextureData);

    glGenTextures(1, &mLutTextureBitmask);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, MUM_NUM_8BIT_VALUES, MUM_MASK_TABLE_ROWS * MUM_NUM_ROUNDS, 0, GL_RED,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mLutTextureBitmaskI);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmaskI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, MUM_NUM_8BIT_VALUES, MUM_MASK_TABLE_ROWS * MUM_NUM_ROUNDS, 0, GL_RED,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mPositionTexturesX);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mPositionTexturesY);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mPositionTexturesXI);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mPositionTexturesYI);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, MUM_CELLS_YB, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mLutTexturePermute);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermute);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, MUM_NUM_8BIT_VALUES, MUM_CELLS_YB, 0, GL_RED,
                 GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &mLutTexturePermuteI);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, MUM_NUM_8BIT_VALUES, MUM_CELLS_YB, 0, GL_RED,
                 GL_UNSIGNED_BYTE, NULL);
    return true;
}

void CMumblepadGlb::InitKey()
{
    WriteTextures();
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
    mPrng = new CMumPrng(mMumInfo->subkeys[MUM_PRNG_SUBKEY_INDEX]);
}

void CMumblepadGlb::WriteTextures()
{
    uint32_t r;
    uint32_t indicesB[8] = {6, 5, 4, 3, 2, 1, 0, 7};

    for (r = 0; r < MUM_NUM_ROUNDS; r++)
    {
        glBindTexture(GL_TEXTURE_2D, mLutTextureKey);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->subkeys[r]);

        glBindTexture(GL_TEXTURE_2D, mLutTextureKeyI);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->subkeys[indicesB[r]]);

        glBindTexture(GL_TEXTURE_2D, mLutTexturePermute);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_NUM_8BIT_VALUES,
                        MUM_CELLS_MAX_Y, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->permuteTextureData[r]);

        glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_NUM_8BIT_VALUES,
                        MUM_CELLS_MAX_Y, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->permuteTextureDataI[indicesB[r]]);

        glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_NUM_8BIT_VALUES,
                        MUM_CELLS_MAX_Y, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->bitmaskTextureData[r]);

        glBindTexture(GL_TEXTURE_2D, mLutTextureBitmaskI);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (7 - r) * MUM_CELLS_MAX_Y, MUM_NUM_8BIT_VALUES,
                        MUM_CELLS_MAX_Y, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->bitmaskTextureData[r]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesX);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataX[r]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesY);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, r * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataYB[r]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (7 - r) * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataXI[r]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (7 - r) * MUM_CELLS_MAX_Y, MUM_CELLS_X,
                        MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataYIB[r]);
    }
}

GLuint CMumblepadGlb::CreateShader(const char *vertShaderSrc, const char *fragShaderSrc)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    vertexShader = mGlw->LoadShader(GL_VERTEX_SHADER, vertShaderSrc);
    if (vertexShader == 0)
    {
        assert(0);
        return 0;
    }

    fragmentShader = mGlw->LoadShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        assert(0);
        return 0;
    }

    programObject = glCreateProgram();
    if (programObject == 0)
    {
        assert(0);
        return 0;
    }

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programObject;
}

bool CMumblepadGlb::CreateShaders()
{
    mEncryptDiffuseProgram = CreateShader(vertexShaderText, encryptDiffuseText);
    mEncryptConfuseProgram = CreateShader(vertexShaderText, encryptConfuseText);
    mDecryptDiffuseProgram = CreateShader(vertexShaderText, decryptDiffuseText);
    mDecryptConfuseProgram = CreateShader(vertexShaderText, decryptConfuseText);
    return 1;
}

bool CMumblepadGlb::InitTextures()
{
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    if (!CreateLutTextures())
        return false;
    if (!CreateFrameBuffers())
        return false;
    if (!CreateShaders())
        return false;
    return true;
}

void CMumblepadGlb::EncryptDiffuse(uint32_t /*round*/)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // first pass for encrypt
    // destination is ping pong 1
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);
    glViewport(0, 0, MUM_CELLS_X, MUM_CELLS_YB);

    glUseProgram(mEncryptDiffuseProgram);
    location = glGetUniformLocation(mEncryptDiffuseProgram, "source");
    glUniform1i(location, 0);
    location = glGetUniformLocation(mEncryptDiffuseProgram, "bitmasks");
    glUniform1i(location, 1);
    location = glGetUniformLocation(mEncryptDiffuseProgram, "positionX");
    glUniform1i(location, 2);
    location = glGetUniformLocation(mEncryptDiffuseProgram, "positionY");
    glUniform1i(location, 3);

    // precompute!
    positionLoc = glGetAttribLocation(mEncryptDiffuseProgram, "a_position");
    texCoordLoc = glGetAttribLocation(mEncryptDiffuseProgram, "a_texCoord");

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT,
                          GL_FALSE, 3 * sizeof(GLfloat), wholeSquareVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUv);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    // source is ping pong 0
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGlb::EncryptConfuse(uint32_t /*round*/)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // destination is ping pong 0
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[0], 0);
    glViewport(0, 0, MUM_CELLS_X, MUM_CELLS_YB);

    glUseProgram(mEncryptConfuseProgram);
    location = glGetUniformLocation(mEncryptConfuseProgram, "source");
    glUniform1i(location, 0);
    location = glGetUniformLocation(mEncryptConfuseProgram, "lutKey");
    glUniform1i(location, 1);
    location = glGetUniformLocation(mEncryptConfuseProgram, "lutXor");
    glUniform1i(location, 2);
    location = glGetUniformLocation(mEncryptConfuseProgram, "lutPermute");
    glUniform1i(location, 3);

    // precompute!
    positionLoc = glGetAttribLocation(mEncryptConfuseProgram, "a_position");
    texCoordLoc = glGetAttribLocation(mEncryptConfuseProgram, "a_texCoord");

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT,
                          GL_FALSE, 3 * sizeof(GLfloat), wholeSquareVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUvOffset);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    // source is ping pong 1
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKey);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermute);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGlb::DecryptDiffuse(uint32_t /*round*/)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // second pass for decrypt
    // destination ping pong is 0
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[0], 0);
    glViewport(0, 0, MUM_CELLS_X, MUM_CELLS_YB);

    glUseProgram(mDecryptDiffuseProgram);
    location = glGetUniformLocation(mDecryptDiffuseProgram, "source");
    glUniform1i(location, 0);
    location = glGetUniformLocation(mDecryptDiffuseProgram, "bitmasks");
    glUniform1i(location, 1);
    location = glGetUniformLocation(mDecryptDiffuseProgram, "positionX");
    glUniform1i(location, 2);
    location = glGetUniformLocation(mDecryptDiffuseProgram, "positionY");
    glUniform1i(location, 3);

    // precompute!
    positionLoc = glGetAttribLocation(mDecryptDiffuseProgram, "a_position");
    texCoordLoc = glGetAttribLocation(mDecryptDiffuseProgram, "a_texCoord");

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT,
                          GL_FALSE, 3 * sizeof(GLfloat), wholeSquareVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUv);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // second pass for decrypt
    // source ping pong is 1
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmaskI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGlb::DecryptConfuse(uint32_t /*round*/)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // first pass for decrypt
    // destination ping pong is 1
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);
    glViewport(0, 0, MUM_CELLS_X, MUM_CELLS_YB);

    glUseProgram(mDecryptConfuseProgram);

    // precompute!
    location = glGetUniformLocation(mDecryptConfuseProgram, "source");
    glUniform1i(location, 0);
    location = glGetUniformLocation(mDecryptConfuseProgram, "lutKey");
    glUniform1i(location, 1);
    location = glGetUniformLocation(mDecryptConfuseProgram, "lutXor");
    glUniform1i(location, 2);
    location = glGetUniformLocation(mDecryptConfuseProgram, "lutPermute");
    glUniform1i(location, 3);

    // precompute!
    positionLoc = glGetAttribLocation(mDecryptConfuseProgram, "a_position");
    texCoordLoc = glGetAttribLocation(mDecryptConfuseProgram, "a_texCoord");

    glVertexAttribPointer(positionLoc, 3, GL_FLOAT,
                          GL_FALSE, 3 * sizeof(GLfloat), wholeSquareVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUvOffset);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // first pass for decrypt
    // source ping pong is 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKeyI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glFinish();
}

void CMumblepadGlb::EncryptDownload(uint8_t *data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glReadPixels(0, 0, MUM_CELLS_X, MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGlb::DecryptDownload(uint8_t *data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glReadPixels(0, 7 * MUM_CELLS_MAX_Y, MUM_CELLS_X, MUM_CELLS_MAX_Y, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif
