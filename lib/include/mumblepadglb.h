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


#ifndef __MUMBLEPADGLB_H
#define __MUMBLEPADGLB_H

#include "mumrenderer.h"

#ifdef USE_MUM_OPENGL

#include "mumglwrapper.h"


class CMumblepadGlb : public CMumRenderer 
{
public:
    CMumblepadGlb(TMumInfo *mumInfo, CMumGlWrapper *mumGlWrapper);
    ~CMumblepadGlb();
    virtual void EncryptDiffuse(uint32_t round);
    virtual void EncryptConfuse(uint32_t round);
    virtual void DecryptConfuse(uint32_t round);
    virtual void DecryptDiffuse(uint32_t round);
    virtual void EncryptUpload(uint8_t *data);
    virtual void EncryptDownload(uint8_t *data);
    virtual void DecryptUpload(uint8_t *data);
    virtual void DecryptDownload(uint8_t *data);
    virtual void InitKey();
private:
    TMumInfo *mMumInfo;
    CMumGlWrapper *mGlw;
    bool InitTextures();
    bool CreateLutTextures();
    bool CreateFrameBuffers();
    bool CreateShaders();
    void DeleteFrameBuffers();
    void DeleteLutTextures();
    void WriteTextures();
    GLuint CreateShader(const char *vertShaderSrc, const char *fragShaderSrc);

    uint32_t mEncryptDiffuseProgram;
    uint32_t mEncryptConfuseProgram;
    uint32_t mDecryptDiffuseProgram;
    uint32_t mDecryptConfuseProgram;

    // textures
    GLuint mLutTextureKey;
    GLuint mLutTextureKeyI;
    GLuint mLutTexturePermute;
    GLuint mLutTexturePermuteI;
    GLuint mLutTextureXor;
    GLuint mLutTextureBitmask;
    GLuint mLutTextureBitmaskI;
    GLuint mPingPongTexture[2];
    GLuint mPingPongFBuffer[2];
    GLuint mPositionTexturesX;
    GLuint mPositionTexturesY;
    GLuint mPositionTexturesXI;
    GLuint mPositionTexturesYI;

};

#endif

#endif
