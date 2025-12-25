#include "debug_rendering.h"
#include "surface.h"

#include "GLFW/glfw3.h"

namespace {
    GLenum pickGLFormat(PixelFormat pixelFormat) {
        switch (pixelFormat) {
            case PixelFormat::BGRA8888: return GL_BGRA;
            case PixelFormat::BGRX8888: return GL_BGRA;
            case PixelFormat::BGR888:   return GL_BGR;

            case PixelFormat::BGRA5551: Assert(false, "Not supported yet") return 0;
            case PixelFormat::BGR555: Assert(false, "Not supported yet") return 0;

            case PixelFormat::Unknown: [[fallthrough]];
            case PixelFormat::SENTINEL: [[fallthrough]];
            default:
                Assert(false, "invalid pixel format");
                return false;
        }
    }

    GLint pickGLInternalFormat(PixelFormat pixelFormat) {
        switch (pixelFormat) {
            case PixelFormat::BGRA8888: return GL_RGBA8;
            case PixelFormat::BGRX8888: return GL_RGBA8;
            case PixelFormat::BGR888:   return GL_RGB8;

            case PixelFormat::BGRA5551: Assert(false, "Not supported yet") return 0;
            case PixelFormat::BGR555: Assert(false, "Not supported yet") return 0;

            case PixelFormat::Unknown: [[fallthrough]];
            case PixelFormat::SENTINEL: [[fallthrough]];
            default:
                Assert(false, "invalid pixel format");
                return false;
        }
    }
}

void debug_immPreviewSurface(const Surface& surface) {
    i32 bytesPerPixel = pixelFormatBytesPerPixel(surface.pixelFormat);

    glfwInit();
    defer { glfwTerminate(); };
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* win = glfwCreateWindow(surface.width, surface.height,
                                        "Surface Preview", nullptr, nullptr);
    if (!win) return;
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // allow tightly packed rows
    if (surface.pitch != surface.width * bytesPerPixel) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, surface.pitch / bytesPerPixel);
    }

    const GLint internalFmt = pickGLInternalFormat(surface.pixelFormat);
    const GLenum fmt = pickGLFormat(surface.pixelFormat);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, surface.width, surface.height,
                0, fmt, GL_UNSIGNED_BYTE, surface.data);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        glViewport(0, 0, surface.width, surface.height);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
            glTexCoord2f(1.f, 0.f); glVertex2f( 1.f, -1.f);
            glTexCoord2f(0.f, 1.f); glVertex2f(-1.f,  1.f);
            glTexCoord2f(1.f, 1.f); glVertex2f( 1.f,  1.f);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(win);
    }

    glDeleteTextures(1, &tex);
    glfwDestroyWindow(win);
}
