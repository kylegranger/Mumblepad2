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

#ifndef __MUMGLWRAPPER_H
#define __MUMGLWRAPPER_H

#include "../gl/gl3w.h"
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

//#include "../gl3w/GL/glcorearb.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"
#include "GLES2/gl2platform.h"

#include "mumrenderer.h"

#ifdef USE_OPENGL

#define ESUTIL_API __cdecl
#define ESCALLBACK __cdecl

#define ES_WINDOW_RGB 0
#define ES_WINDOW_ALPHA 1
#define ES_WINDOW_DEPTH 2
#define ES_WINDOW_STENCIL 4
#define ES_WINDOW_MULTISAMPLE 8

struct graphics_context
{
    GLFWwindow *window;
    GLuint program;
    GLint uniform_angle;
    GLuint vbo_point;
    GLuint vao_point;
    GLuint vbo_uv;
    GLuint vao_uv;
    double angle;
    long framecount;
    double lastframe;
};

typedef void(APIENTRYP PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void(APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef void(APIENTRYP PFNGLDISABLEPROC)(GLenum cap);
typedef void(APIENTRYP PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void(APIENTRYP PFNGLENABLEPROC)(GLenum cap);
typedef void(APIENTRYP PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef void(APIENTRYP PFNGLREADPIXELSPROC)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void(APIENTRYP PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void(APIENTRYP PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
typedef void(APIENTRYP PFNGLTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void(APIENTRYP PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);

class CMumGlWrapper
{
public:
    CMumGlWrapper();
    ~CMumGlWrapper();
    bool Init();
    GLuint LoadShader(GLenum type, const char *shaderSrc);

    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLBINDTEXTUREPROC glBindTexture;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLGENTEXTURESPROC glGenTextures;
    PFNGLTEXIMAGE2DPROC glTexImage2D;
    PFNGLREADPIXELSPROC glReadPixels;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLDELETETEXTURESPROC glDeleteTextures;
    PFNGLDRAWELEMENTSPROC glDrawElements;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;
    PFNGLDISABLEPROC glDisable;
    PFNGLENABLEPROC glEnable;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
    PFNGLTEXPARAMETERIPROC glTexParameteri;
    PFNGLVIEWPORTPROC glViewport;

private:
    GLuint LoadProgram(const char *vertShaderSrc, const char *fragShaderSrc);
};

#endif

#endif