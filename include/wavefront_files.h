#pragma once

#include "core_init.h"

namespace Wavefront {

enum struct WavefrontError {
    Undefined,

    FailedToStatFile,
    FailedToReadFile,

    InvalidFileFormat,

    SENTINEL
};

const char* errorToCstr(WavefrontError err);

/**
    # The Wavefront format ASCII 3.0 version has the following file structure.

    Vertex data:
        * geometric vertices (v) - (supported ✅)
        * texture vertices (vt) - (support planned ⚠️)
        * vertex normals (vn) - (support planned ⚠️)
        * parameter space vertices (vp) - (support planned ⚠️)

    Free-form curve/surface attributes:
        * rational or non-rational forms of curve or surface type (cstype) - basis matrix, Bezier, B-spline, Cardinal, Taylor
        * degree (deg)
        * basis matrix (bmat)
        * step size (step)

    Elements:
        * point (p)
        * line (l)
        * face (f) - (support planned ⚠️)
        * curve (curv)
        * 2D curve (curv2)
        * surface (surf)

    Free-form curve/surface body statements:
        * parameter values (parm)
        * outer trimming loop (trim)
        * inner trimming loop (hole)
        * special curve (scrv)
        * special point (sp)
        * end statement (end)

    Connectivity between free-form surfaces:
        * connect (con)

    Grouping:
        * group name (g) - (support planned ⚠️)
        * smoothing group (s)
        * merging group (mg)
        * object name (o) - (support planned ⚠️)

    Display/render attributes:
        * bevel interpolation (bevel)
        * color interpolation (c_interp)
        * dissolve interpolation (d_interp)
        * level of detail (lod)
        * material name (usemtl)
        * material library (mtllib)
        * shadow casting (shadow_obj)
        * ray tracing (trace_obj)
        * curve approximation technique (ctech)
        * surface approximation technique (stech)

    # General statements.

    Wavefront object files has support for file nesting and unix command calling:
        * call filename.ext arg1 arg2... - Reads the contents of the specified .obj or .mod file at this location.
            filename.ext is the name of the .obj or .mod file to be read. You must include the extension with the filename.
            (will NOT support ❌)
        * csh command - Executes the requested UNIX command. If the UNIX command returns an error, the parser flags an
            error during parsing. (will NOT support ❌)
*/

enum struct WavefrontVersion {
    Undefined,

    VERSION_3_0,

    SENTINEL
};

struct WavefrontObj {
    struct Object {
        const char* objectName;
        i32 groupsCount;
        i32 groupsOff;
    };

    struct Group {
        const char* groupName;
        i32 facesCount;
        i32 facesOff;
    };

    core::AllocatorContext* actx;
    core::Memory<Object> objects;
    i32 objectsCount;
    core::Memory<Group> groups;
    i32 groupsCount;
    core::Memory<core::vec3f> vertices;
    i32 verticesCount;
    core::Memory<i32> faces;
    i32 facesCount;

    constexpr inline void setAllocator(core::AllocatorContext& _actx) { actx = &_actx; }

    // TODO: does this api make sense ??
    // Get all faces for object and group
    bool nextObj(Object& obj, Group& group, core::Memory<i32>& faces);

    void free();
};

[[nodiscard]] core::expected<WavefrontObj, WavefrontError> loadFile(const char* path,
                                                                    WavefrontVersion fileVersion,
                                                                    core::AllocatorContext& actx = DEF_ALLOC);

} // Wavefront

