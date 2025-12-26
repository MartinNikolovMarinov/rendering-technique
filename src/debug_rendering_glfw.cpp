#include "debug_rendering.h"
#include "surface.h"

#include "GLFW/glfw3.h"

namespace {

bool g_glfwInitialized = false;

static void glfwErrorCallback(int code, const char* desc);

constexpr GLenum pickGLFormat(PixelFormat pixelFormat);
constexpr GLint pickGLInternalFormat(PixelFormat pixelFormat);
constexpr GLenum pickGLType(PixelFormat pixelFormat);

} // namespace

bool initializeDebugRendering() {
    if (g_glfwInitialized) {
        return true;
    }

    glfwSetErrorCallback(glfwErrorCallback);

    if (glfwInit() == GLFW_FALSE) {
        Assert(false, "Failed to initialize GLFW!");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    g_glfwInitialized = true;
    return true;
}

void shutdownDebugRendering() {
    if (!g_glfwInitialized) {
        return;
    }

    glfwTerminate();
    g_glfwInitialized = false;
}

void debug_immPreviewSurface(const Surface& surface) {
    Panic(g_glfwInitialized, "GLFW is not initalized");

    GLFWwindow* win = glfwCreateWindow(surface.width * surface.bpp(), surface.height * surface.bpp(),
                                        "Surface Preview", nullptr, nullptr);
    if (!win) return;
    defer { glfwDestroyWindow(win); };
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    defer { glDeleteTextures(1, &tex); };

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // allow tightly packed rows

    const GLint internalFmt = pickGLInternalFormat(surface.pixelFormat);
    const GLenum fmt = pickGLFormat(surface.pixelFormat);
    const GLenum type = pickGLType(surface.pixelFormat);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, surface.width, surface.height,
                0, fmt, type, surface.data);

    // Choose texture coordinates based on surface origin (default GL origin is bottom-left).
    float u0 = 0.f, u1 = 1.f, v0 = 0.f, v1 = 1.f;
    switch (surface.origin) {
        case Origin::BottomLeft:
            break;
        case Origin::BottomRight:
            std::swap(u0, u1);
            break;
        case Origin::TopLeft:
            std::swap(v0, v1);
            break;
        case Origin::TopRight:
            std::swap(u0, u1);
            std::swap(v0, v1);
            break;
        case Origin::Undefined: [[fallthrough]];
        case Origin::Center:    [[fallthrough]];
        case Origin::SENTINEL:
            break; // leave default orientation
    }

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        glViewport(0, 0, surface.width * surface.bpp(), surface.height * surface.bpp());
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(u0, v0); glVertex2f(-1.f, -1.f);
            glTexCoord2f(u1, v0); glVertex2f( 1.f, -1.f);
            glTexCoord2f(u0, v1); glVertex2f(-1.f,  1.f);
            glTexCoord2f(u1, v1); glVertex2f( 1.f,  1.f);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(win);
    }
}

namespace {

void glfwErrorCallback(int code, const char* desc) {
    logErr("GLFW error {}: {}", code, desc);
}

constexpr GLenum pickGLFormat(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return GL_BGRA;
        case PixelFormat::BGRX8888: return GL_BGRA;
        case PixelFormat::BGR888:   return GL_BGR;

        case PixelFormat::BGRA5551: return GL_BGRA;
        case PixelFormat::BGR555:   return GL_BGRA;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return false;
    }
}

constexpr GLint pickGLInternalFormat(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return GL_RGBA8;
        case PixelFormat::BGRX8888: return GL_RGBA8;
        case PixelFormat::BGR888:   return GL_RGB8;

        case PixelFormat::BGRA5551: return GL_RGB5_A1;
        case PixelFormat::BGR555:   return GL_RGB5;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return false;
    }
}

constexpr GLenum pickGLType(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return GL_UNSIGNED_BYTE;
        case PixelFormat::BGRX8888: return GL_UNSIGNED_BYTE;
        case PixelFormat::BGR888:   return GL_UNSIGNED_BYTE;

        case PixelFormat::BGRA5551: return GL_UNSIGNED_SHORT_1_5_5_5_REV;
        case PixelFormat::BGR555:   return GL_UNSIGNED_SHORT_1_5_5_5_REV;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return false;
    }
}

} // namespace
