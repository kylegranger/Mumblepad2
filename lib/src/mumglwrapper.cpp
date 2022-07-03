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

#include <string.h>
#include <malloc.h>
//#include "../gl/gl3w.h"
#include <GL/glx.h>
#include "mumglwrapper.h"

#ifdef USE_OPENGL

struct graphics_context context;

extern GL3WglProc get_proc(const char *proc);

CMumGlWrapper::CMumGlWrapper()
{
}

bool CMumGlWrapper::Init()
{
    memset(this, 0, sizeof(CMumGlWrapper));

    if (!glfwInit())
    {
        printf("GLFW3: failed to initialize\n");
        return false;
    }
    // glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    context.window = glfwCreateWindow(320, 240, "mumblepad", NULL, NULL);
    glfwMakeContextCurrent(context.window);
    glfwSwapInterval(1);

    glActiveTexture = (PFNGLACTIVETEXTUREPROC)get_proc("glActiveTexture");
    glAttachShader = (PFNGLATTACHSHADERPROC)get_proc("glAttachShader");
    glBindBuffer = (PFNGLBINDBUFFERPROC)get_proc("glBindBuffer");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)get_proc("glBindFramebuffer");
    glBindTexture = (PFNGLBINDTEXTUREPROC)get_proc("glBindTexture");
    glBufferData = (PFNGLBUFFERDATAPROC)get_proc("glBufferData");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)get_proc("glCheckFramebufferStatus");
    glCompileShader = (PFNGLCOMPILESHADERPROC)get_proc("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)get_proc("glCreateProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)get_proc("glCreateShader");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)get_proc("glDeleteFramebuffers");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)get_proc("glDeleteProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)get_proc("glDeleteShader");
    glDeleteTextures = (PFNGLDELETETEXTURESPROC)get_proc("glDeleteTextures");
    glDisable = (PFNGLDISABLEPROC)get_proc("glDisable");
    glDrawBuffers = (PFNGLDRAWBUFFERSPROC)get_proc("glDrawBuffers");
    glDrawElements = (PFNGLDRAWELEMENTSPROC)get_proc("glDrawElements");
    glEnable = (PFNGLENABLEPROC)get_proc("glEnable");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)get_proc("glEnableVertexAttribArray");

    glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)get_proc("glTexSubImage2D");
    glGenTextures = (PFNGLGENTEXTURESPROC)get_proc("glGenTextures");

    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)get_proc("glGetProgramInfoLog");

    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)get_proc("glGenFramebuffers");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)get_proc("glFramebufferTexture2D");
    glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)get_proc("glFramebufferTexture");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)get_proc("glGetShaderiv");
    glGenBuffers = (PFNGLGENBUFFERSPROC)get_proc("glGenBuffers");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)get_proc("glGetShaderInfoLog");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)get_proc("glGetShaderiv");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)get_proc("glGetAttribLocation");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)get_proc("glGetProgramiv");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)get_proc("glGetUniformLocation");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)get_proc("glLinkProgram");
    glReadPixels = (PFNGLREADPIXELSPROC)get_proc("glReadPixels");
    glShaderSource = (PFNGLSHADERSOURCEPROC)get_proc("glShaderSource");
    glTexParameteri = (PFNGLTEXPARAMETERIPROC)get_proc("glTexParameteri");
    glTexImage2D = (PFNGLTEXIMAGE2DPROC)get_proc("glTexImage2D");
    glUniform1i = (PFNGLUNIFORM1IPROC)get_proc("glUniform1i");
    glUseProgram = (PFNGLUSEPROGRAMPROC)get_proc("glUseProgram");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)get_proc("glVertexAttribPointer");
    glViewport = (PFNGLVIEWPORTPROC)get_proc("glViewport");

    printf("gl3w: ok to go!\n");
    return true;
}


GLuint CMumGlWrapper::LoadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if (shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint CMumGlWrapper::LoadProgram(const char *vertShaderSrc, const char *fragShaderSrc)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, vertShaderSrc);
    if (vertexShader == 0)
        return 0;

    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    // Create the program object
    programObject = glCreateProgram();

    if (programObject == 0)
        return 0;

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

    // Free up no longer needed shader resources
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programObject;
}

#endif