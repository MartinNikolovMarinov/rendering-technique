#pragma once

#include "core_init.h"

struct Model3D;

namespace Wavefront {

enum struct WavefrontError {
    Undefined,

    UnsupportedVersion,
    FailedToStatFile,
    FailedToReadFile,
    InvalidFileFormat,

    SENTINEL
};

const char* errorToCstr(WavefrontError err);

/**
    # The Wavefront format ASCII 3.0 version has the following file structure.

    Vertex data:
        * geometric vertices (v) - (supported ✅) - Polygonal and free-form geometry statement.
        * texture vertices (vt) - (support planned ⚠️)
        * vertex normals (vn) - (support planned ⚠️)
        * parameter space vertices (vp) - (support planned ⚠️)

        NOTE: The vertex data is represented by four vertex lists; one for each type of vertex coordinate. A right-hand
        coordinate system is used to specify the coordinate locations.

    Free-form curve/surface attributes:
        * rational or non-rational forms of curve or surface type (cstype) - basis matrix, Bezier, B-spline, Cardinal, Taylor
        * degree (deg)
        * basis matrix (bmat)
        * step size (step)

    Elements:
        * point (p)
        * line (l)
        * face (f) - (supported ✅) - only 3 dimensions!
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
    core::AllocatorContext* actx;

    struct Face {
        using FaceComponent = i32[3];
        static constexpr i32 DIMMENTIONS = 3;

        constexpr FaceComponent& v() { return data[0]; }
        constexpr FaceComponent& vt() { return data[1]; }
        constexpr FaceComponent& vn() { return data[2]; }

        constexpr bool isSet(i32 dimmensionIdx, i32 idx) const {
            return setFieldsMask & ((1 << idx) << (dimmensionIdx * DIMMENTIONS));
        }
        constexpr void set(i32 dimmensionIdx, i32 idx) {
            setFieldsMask |= (1 << idx) << (dimmensionIdx * DIMMENTIONS);
        }

        FaceComponent data[DIMMENTIONS];
        i32 setFieldsMask;
    };

    core::Memory<core::vec4f> vertices;
    i32 verticesCount;
    core::Memory<Face> faces;
    i32 facesCount;

    constexpr inline void setAllocator(core::AllocatorContext& _actx) { actx = &_actx; }

    void free();
};

[[nodiscard]] core::expected<WavefrontObj, WavefrontError> loadFile(
    const char* path,
    WavefrontVersion fileVersion,
    core::AllocatorContext& actx = DEF_ALLOC
);
Model3D createModelFromWavefrontObj(const WavefrontObj& obj, core::AllocatorContext& modelActx = DEF_ALLOC);

} // Wavefront
