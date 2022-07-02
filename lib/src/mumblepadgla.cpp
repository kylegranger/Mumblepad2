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
#include "malloc.h"
#include "assert.h"
#include "../gl/gl3w.h"
#include "mumblepadgla.h"

#ifdef USE_MUM_OPENGL

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
    gl_FragColor[0] = texture2D(bitmasks,vec2(src1[0],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[2],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[3],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[1],0.875))[0];\n\
    gl_FragColor[1] = texture2D(bitmasks,vec2(src1[2],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[3],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[1],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[0],0.875))[0];\n\
    gl_FragColor[2] = texture2D(bitmasks,vec2(src1[3],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[1],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[0],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[2],0.875))[0];\n\
    gl_FragColor[3] = texture2D(bitmasks,vec2(src1[1],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[0],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[2],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[3],0.875))[0];\n\
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
    gl_FragColor[0] = texture2D(bitmasks,vec2(src1[0],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[3],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[2],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[1],0.875))[0];\n\
    gl_FragColor[1] = texture2D(bitmasks,vec2(src1[3],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[2],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[1],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[0],0.875))[0];\n\
    gl_FragColor[2] = texture2D(bitmasks,vec2(src1[1],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[0],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[3],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[2],0.875))[0];\n\
    gl_FragColor[3] = texture2D(bitmasks,vec2(src1[2],0.125))[0] + \
        texture2D(bitmasks,vec2(src2[1],0.375))[0] + \
        texture2D(bitmasks,vec2(src3[0],0.625))[0] + \
        texture2D(bitmasks,vec2(src4[3],0.875))[0];\n\
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
    vec4 xorKey;\n\
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

CMumblepadGla::CMumblepadGla(TMumInfo *mumInfo, CMumGlWrapper *mumGlWrapper) : CMumRenderer(mumInfo)
{
    uint32_t round;
    mMumInfo = mumInfo;
    mGlw = mumGlWrapper;
    mEncryptDiffuseProgram = -1;
    mEncryptConfuseProgram = -1;
    mDecryptDiffuseProgram = -1;
    mDecryptConfuseProgram = -1;

    mPingPongTexture[0] = -1;
    mPingPongFBuffer[0] = -1;
    mPingPongTexture[1] = -1;
    mPingPongFBuffer[1] = -1;

    for (round = 0; round < MUM_NUM_ROUNDS; round++)
    {
        mLutTextureKey[round] = -1;
        mLutTexturePermute[round] = -1;
        mLutTexturePermuteI[round] = -1;
        mLutTextureBitmask[round] = -1;
        mPositionTexturesX[round] = -1;
        mPositionTexturesY[round] = -1;
        mPositionTexturesXI[round] = -1;
        mPositionTexturesYI[round] = -1;
    }
    mLutTextureXor = -1;
    if (!InitTextures())
        assert(0);
}

void CMumblepadGla::DeleteFrameBuffers()
{
    glDeleteTextures(2, mPingPongTexture);
    glDeleteFramebuffers(2, mPingPongFBuffer);
}

void CMumblepadGla::DeleteLutTextures()
{
    uint32_t round;

    glDeleteTextures(1, &mLutTextureXor);

    for (round = 0; round < MUM_NUM_ROUNDS; round++)
    {
        glDeleteTextures(1, &mLutTextureKey[round]);
        glDeleteTextures(1, &mLutTextureBitmask[round]);
        glDeleteTextures(1, &mPositionTexturesX[round]);
        glDeleteTextures(1, &mPositionTexturesY[round]);
        glDeleteTextures(1, &mPositionTexturesXI[round]);
        glDeleteTextures(1, &mPositionTexturesYI[round]);
        glDeleteTextures(1, &mLutTexturePermute[round]);
        glDeleteTextures(1, &mLutTexturePermuteI[round]);
    }
}

CMumblepadGla::~CMumblepadGla()
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

void CMumblepadGla::EncryptUpload(uint8_t *data)
{
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                    mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void CMumblepadGla::DecryptUpload(uint8_t *data)
{
    EncryptUpload(data);
}

bool CMumblepadGla::CreateFrameBuffers()
{
    GLenum result;
    glGenTextures(2, mPingPongTexture);
    glGenFramebuffers(2, mPingPongFBuffer);

    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);

    result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool CMumblepadGla::CreateLutTextures()
{
    uint32_t round;

    glGenTextures(1, &mLutTextureXor);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, MUM_NUM_8BIT_VALUES, MUM_NUM_8BIT_VALUES, 0, GL_RED,
                 GL_UNSIGNED_BYTE, mMumInfo->xorTextureData);

    for (round = 0; round < MUM_NUM_ROUNDS; round++)
    {
        glGenTextures(1, &mLutTextureKey[round]);
        glBindTexture(GL_TEXTURE_2D, mLutTextureKey[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mLutTextureBitmask[round]);
        glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, MUM_NUM_8BIT_VALUES, MUM_MASK_TABLE_ROWS, 0, GL_RED,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mPositionTexturesX[round]);
        glBindTexture(GL_TEXTURE_2D, mPositionTexturesX[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mPositionTexturesY[round]);
        glBindTexture(GL_TEXTURE_2D, mPositionTexturesY[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mPositionTexturesXI[round]);
        glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mPositionTexturesYI[round]);
        glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MUM_CELLS_X, mMumInfo->numRows, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mLutTexturePermute[round]);
        glBindTexture(GL_TEXTURE_2D, mLutTexturePermute[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, MUM_NUM_8BIT_VALUES, mMumInfo->numRows, 0, GL_RED,
                     GL_UNSIGNED_BYTE, NULL);

        glGenTextures(1, &mLutTexturePermuteI[round]);
        glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI[round]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, MUM_NUM_8BIT_VALUES, mMumInfo->numRows, 0, GL_RED,
                     GL_UNSIGNED_BYTE, NULL);
    }
    return true;
}

void CMumblepadGla::InitKey()
{
    WriteTextures();
    if (mPrng != nullptr)
    {
        delete mPrng;
        mPrng = nullptr;
    }
    mPrng = new CMumPrng(mMumInfo->subkeys[MUM_PRNG_SUBKEY_INDEX]);
}

void CMumblepadGla::WriteTextures()
{
    uint32_t round;

    for (round = 0; round < MUM_NUM_ROUNDS; round++)
    {
        glBindTexture(GL_TEXTURE_2D, mLutTextureKey[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                        mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->subkeys[round]);

        glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_NUM_8BIT_VALUES, MUM_MASK_TABLE_ROWS,
                        GL_RED, GL_UNSIGNED_BYTE, mMumInfo->bitmaskTextureData[round]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesX[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                        mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataX[round]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesY[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                        mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataY[round]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                        mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataXI[round]);

        glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_CELLS_X,
                        mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, mMumInfo->positionTextureDataYI[round]);

        glBindTexture(GL_TEXTURE_2D, mLutTexturePermute[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_NUM_8BIT_VALUES,
                        mMumInfo->numRows, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->permuteTextureData[round]);

        glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI[round]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MUM_NUM_8BIT_VALUES,
                        mMumInfo->numRows, GL_RED, GL_UNSIGNED_BYTE, mMumInfo->permuteTextureDataI[round]);
    }
}

GLuint CMumblepadGla::CreateShader(const char *vertShaderSrc, const char *fragShaderSrc)
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

bool CMumblepadGla::CreateShaders()
{
    printf("do encryptDiffuseText\n");
    mEncryptDiffuseProgram = CreateShader(vertexShaderText, encryptDiffuseText);
    printf("do encryptConfuseText\n");
    mEncryptConfuseProgram = CreateShader(vertexShaderText, encryptConfuseText);
    printf("do decryptDiffuseText\n");
    mDecryptDiffuseProgram = CreateShader(vertexShaderText, decryptDiffuseText);
    printf("do decryptConfuseText\n");
    mDecryptConfuseProgram = CreateShader(vertexShaderText, decryptConfuseText);
    return true;
}

bool CMumblepadGla::InitTextures()
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

void CMumblepadGla::EncryptDiffuse(uint32_t round)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    //
    // destination is ping pong 1
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);
    glViewport(0, 0, MUM_CELLS_X, mMumInfo->numRows);

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

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesX[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesY[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGla::EncryptConfuse(uint32_t round)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // destination is ping pong 0
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[0], 0);
    glViewport(0, 0, MUM_CELLS_X, mMumInfo->numRows);

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
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUv);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    // source is ping pong 1
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[1]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKey[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermute[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGla::DecryptDiffuse(uint32_t round)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // second pass for decrypt
    // destination ping pong is 0
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[0], 0);
    glViewport(0, 0, MUM_CELLS_X, mMumInfo->numRows);

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

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureBitmask[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesXI[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mPositionTexturesYI[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGla::DecryptConfuse(uint32_t round)
{
    uint32_t location;
    uint32_t positionLoc;
    uint32_t texCoordLoc;

    // first pass for decrypt
    // destination ping pong is 1
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPingPongTexture[1], 0);
    glViewport(0, 0, MUM_CELLS_X, mMumInfo->numRows);

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
                          GL_FALSE, 2 * sizeof(GLfloat), wholeSquareUv);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // first pass for decrypt
    // source ping pong is 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mLutTextureKey[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mLutTextureXor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mLutTexturePermuteI[round]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, wholeSquareIndices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGla::EncryptDownload(uint8_t *data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mPingPongFBuffer[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mPingPongTexture[0]);
    glReadPixels(0, 0, MUM_CELLS_X, mMumInfo->numRows, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMumblepadGla::DecryptDownload(uint8_t *data)
{
    EncryptDownload(data);
}

#endif
