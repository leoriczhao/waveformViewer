#include "glad.h"
#include <GL/glx.h>

PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;

int gladLoadGL(void) {
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glGenVertexArrays");
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glXGetProcAddress((const GLubyte*)"glBindVertexArray");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glDeleteVertexArrays");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenBuffers");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindBuffer");
    glad_glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferData");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteBuffers");
    glad_glCreateShader = (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte*)"glCreateShader");
    glad_glShaderSource = (PFNGLSHADERSOURCEPROC)glXGetProcAddress((const GLubyte*)"glShaderSource");
    glad_glCompileShader = (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte*)"glCompileShader");
    glad_glDeleteShader = (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte*)"glDeleteShader");
    glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glCreateProgram");
    glad_glAttachShader = (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glAttachShader");
    glad_glLinkProgram = (PFNGLLINKPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glLinkProgram");
    glad_glUseProgram = (PFNGLUSEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glUseProgram");
    glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glDeleteProgram");
    glad_glGetShaderiv = (PFNGLGETSHADERIVPROC)glXGetProcAddress((const GLubyte*)"glGetShaderiv");
    glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
    glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glXGetProcAddress((const GLubyte*)"glGetProgramiv");
    glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetProgramInfoLog");
    glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetUniformLocation");
    glad_glUniform2f = (PFNGLUNIFORM2FPROC)glXGetProcAddress((const GLubyte*)"glUniform2f");
    glad_glUniform4f = (PFNGLUNIFORM4FPROC)glXGetProcAddress((const GLubyte*)"glUniform4f");
    glad_glUniform1i = (PFNGLUNIFORM1IPROC)glXGetProcAddress((const GLubyte*)"glUniform1i");
    glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glXGetProcAddress((const GLubyte*)"glVertexAttribPointer");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glXGetProcAddress((const GLubyte*)"glEnableVertexAttribArray");
    
    return glad_glGenVertexArrays != NULL;
}

