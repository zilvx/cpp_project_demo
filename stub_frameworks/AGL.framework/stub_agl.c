/* Stub AGL framework — macOS 26 removed AGL, but Qt6 still links it.
   These symbols are never actually called by Qt6 on modern macOS. */
#include <stdint.h>
#include <CoreGraphics/CoreGraphics.h>

typedef uint32_t AGLPixelFormat;
typedef void*   AGLContext;
typedef void*   AGLPbuffer;
typedef void*   AGLRendererInfo;
typedef uint32_t AGLDevice;
typedef uint32_t GLint;
typedef uint32_t GLenum;
typedef uint32_t GLuint;
typedef unsigned char GLubyte;
typedef void*   GDHandle;
typedef struct OpaqueWindowPtr* WindowRef;
typedef void* HIViewRef;

AGLPixelFormat aglChoosePixelFormat(const GDHandle* gdevs, GLint ndev,
                                     const GLint* attribs) { return 0; }
AGLPixelFormat aglChoosePixelFormatCFM(const GDHandle* gdevs, GLint ndev,
                                        const GLint* attribs) { return 0; }
AGLPixelFormat aglCreatePixelFormat(const GLint* attribs) { return 0; }
AGLPixelFormat aglCreatePixelFormatCFM(const GLint* attribs) { return 0; }
GLint aglConfigure(AGLPixelFormat pix, GLint param, GLint val) { return 0; }
GLint aglConfigureCFM(AGLPixelFormat pix, GLint param, GLint val) { return 0; }
GLint aglCopyContext(AGLContext src, AGLContext dst, GLuint mask) { return 0; }
AGLContext aglCreateContext(AGLPixelFormat pix, AGLContext share) { return 0; }
AGLContext aglCreateContextCFM(AGLPixelFormat pix, AGLContext share) { return 0; }
GLint aglCreatePBuffer(GLint width, GLint height, GLenum target,
                        GLenum internalFormat, GLint max_level, AGLPbuffer* pbuffer) { return 0; }
GLint aglDescribePBuffer(AGLPbuffer pbuffer, GLint* width, GLint* height,
                          GLenum* target, GLenum* internalFormat,
                          GLint* max_level) { return 0; }
GLint aglDescribePixelFormat(AGLPixelFormat pix, GLint attrib, GLint* value) { return 0; }
GLint aglDescribeRenderer(AGLRendererInfo info, GLint prop, GLint* value) { return 0; }
GLint aglDestroyContext(AGLContext ctx) { return 0; }
GLint aglDestroyPBuffer(AGLPbuffer pbuffer) { return 0; }
void aglDestroyPixelFormat(AGLPixelFormat pix) {}
void aglDestroyRendererInfo(AGLRendererInfo info) {}
GLint aglDevicesOfPixelFormat(AGLPixelFormat pix, GLint* ndevs, AGLDevice* devs) { return 0; }
GLint aglDisplaysOfPixelFormat(AGLPixelFormat pix, GLint* ndevs, GDHandle* gdevs) { return 0; }
GLint aglDisable(AGLContext ctx, GLenum pname) { return 0; }
GLint aglEnable(AGLContext ctx, GLenum pname) { return 0; }
const GLubyte* aglErrorString(GLenum code) { return (const GLubyte*)"stub"; }
GLint aglGetCGLContext(AGLContext ctx, void** cgl_ctx) { return 0; }
GLint aglGetCGLPixelFormat(AGLPixelFormat pix, void** cgl_pix) { return 0; }
AGLContext aglGetCurrentContext(void) { return 0; }
GLint aglGetDrawable(AGLContext ctx, WindowRef* drawable) { return 0; }
GLint aglGetWindowRef(AGLContext ctx, WindowRef* window) { return 0; }
GLint aglGetHIViewRef(AGLContext ctx, HIViewRef* hiview) { return 0; }
GLenum aglGetError(void) { return 0; }
GLint aglGetInteger(AGLContext ctx, GLenum pname, GLint* params) { return 0; }
