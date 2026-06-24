#pragma once

#include "RendererCore.hpp"

static constexpr VertexNormal gCubeVertexData[] = {
    // Front face (+Z)
    {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}}, // 0
    {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}}, // 1
    {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}}, // 2
    {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}}, // 3

    // Back face (-Z)
    {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}}, // 4
    {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}}, // 5
    {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}}, // 6
    {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}}, // 7

    // Left face (-X)
    {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}}, // 8
    {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}}, // 9
    {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}}, //10
    {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}}, //11

    // Right face (+X)
    {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}}, //12
    {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}}, //13
    {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}}, //14
    {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}}, //15

    // Top face (+Y)
    {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}}, //16
    {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}}, //17
    {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}}, //18
    {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}}, //19

    // Bottom face (-Y)
    {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}}, //20
    {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}}, //21
    {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}}, //22
    {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}}, //23
};


static constexpr uint32 gCubeIndexData[] = {
    // Front face (+Z)
    0,1,2, 0,2,3,
    // Back face (-Z)
    4,6,5, 4,7,6,
    // Left face (-X)
    8,11,10, 8,10,9,
    // Right face (+X)
    12,13,14, 12,14,15,
    // Top face (+Y)
    16,17,18, 16,18,19,
    // Bottom face (-Y)
    20,22,21, 20,23,22
};


static constexpr VertexNormal gPlaneVertexData[] = {
    { {-0.5f, 0.0f, -0.5f}, {0, 1, 0} }, // 0, bottom-left
    { { 0.5f, 0.0f, -0.5f}, {0, 1, 0} }, // 1, bottom-right
    { { 0.5f, 0.0f,  0.5f}, {0, 1, 0} }, // 2, top-right
    { {-0.5f, 0.0f,  0.5f}, {0, 1, 0} }  // 3, top-left
};

static constexpr uint32 gPlaneIndexData[] = {
    0, 2, 1, 0, 3, 2
};

template<uint32 LAT, uint32 LON>
struct SphereMesh
{
    static constexpr uint32 VERTS = (LAT + 1) * (LON + 1);
    static constexpr uint32 INDICES = LAT * LON * 6;

    VertexNormal vertices[VERTS]{};
    uint32 indices[INDICES]{};

    explicit constexpr SphereMesh(float radius = 0.5f)
    {
        uint32 v = 0;
        for (uint32 y = 0; y <= LAT; y++)
        {
            const float vlat  = static_cast<float>(y) / LAT;
            const float phi   = vlat * 3.14159265358979f;

            for (uint32 x = 0; x <= LON; x++)
            {
                const float ulon  = static_cast<float>(x) / LON;
                const float theta = ulon * 6.28318530718f;

                const float sx = sin(phi) * cos(theta);
                const float sy = cos(phi);
                const float sz = sin(phi) * sin(theta);

                vertices[v++] = {
                    { radius * sx, radius * sy, radius * sz },
                    { sx, sy, sz }
                };
            }
        }

        uint32 i = 0;
        for (uint32 y = 0; y < LAT; y++)
        {
            for (uint32 x = 0; x < LON; x++)
            {
                uint32 i0 = y * (LON + 1) + x;
                uint32 i1 = i0 + 1;
                uint32 i2 = i0 + (LON + 1);
                uint32 i3 = i2 + 1;

                indices[i++] = i0; indices[i++] = i1; indices[i++] = i2;
                indices[i++] = i1; indices[i++] = i3; indices[i++] = i2;
            }
        }
    }
};

static SphereMesh<16, 16> gSphereMesh;

template<uint32 SEG>
struct CylinderMesh
{
    static constexpr uint32 SIDE_VERTS = (SEG + 1) * 2;
    static constexpr uint32 CAP_VERTS  = (SEG + 2) * 2;
    static constexpr uint32 VERTS      = SIDE_VERTS + CAP_VERTS;

    static constexpr uint32 SIDE_IND   = SEG * 6;
    static constexpr uint32 CAP_IND    = SEG * 6;
    static constexpr uint32 INDICES    = SIDE_IND + CAP_IND;

    VertexNormal vertices[VERTS]{};
    uint32 indices[INDICES]{};

    explicit constexpr CylinderMesh(float radius = 0.5f, float height = 1.0f)
    {
        const float h = height * 0.5f;
        uint32 v = 0;

        // ======================
        // Side vertices
        // ======================
        for (uint32 i = 0; i <= SEG; ++i)
        {
            const float u   = static_cast<float>(i) / SEG;
            const float ang = u * 6.28318530718f;

            const float x = cos(ang);
            const float z = sin(ang);

            vertices[v++] = {{ radius * x, -h, radius * z }, { x, 0, z }};
            vertices[v++] = {{ radius * x,  h, radius * z }, { x, 0, z }};
        }

        // ======================
        // Top cap (+Y)
        // ======================
        const uint32 topCenter = v;
        vertices[v++] = {{ 0.0f, h, 0.0f }, { 0, 1, 0 }};

        for (uint32 i = 0; i <= SEG; ++i)
        {
            const float u   = static_cast<float>(i) / SEG;
            const float ang = u * 6.28318530718f;

            vertices[v++] = {
                { radius * cosf(ang), h, radius * sinf(ang) },
                { 0, 1, 0 }
            };
        }

        // ======================
        // Bottom cap (-Y)
        // ======================
        const uint32 botCenter = v;
        vertices[v++] = {{ 0.0f, -h, 0.0f }, { 0, -1, 0 }};

        for (uint32 i = 0; i <= SEG; ++i)
        {
            const float u   = static_cast<float>(i) / SEG;
            const float ang = u * 6.28318530718f;

            vertices[v++] = {
                { radius * cosf(ang), -h, radius * sinf(ang) },
                { 0, -1, 0 }
            };
        }

        // ======================
        // Indices
        // ======================
        uint32 idx = 0;

        // ---- Sides (CCW outward) ----
        for (uint32 i = 0; i < SEG; ++i)
        {
            const uint32 b0 = i * 2;
            const uint32 b1 = b0 + 1;
            const uint32 t0 = b0 + 2;
            const uint32 t1 = b0 + 3;

            indices[idx++] = b0; indices[idx++] = b1; indices[idx++] = t0;
            indices[idx++] = b1; indices[idx++] = t1; indices[idx++] = t0;
        }

        // ---- Top cap (CCW when viewed from above) ----
        for (uint32 i = 0; i < SEG; ++i)
        {
            indices[idx++] = topCenter;
            indices[idx++] = topCenter + 2 + i;
            indices[idx++] = topCenter + 1 + i;
        }

        // ---- Bottom cap (CCW when viewed from below) ----
        for (uint32 i = 0; i < SEG; ++i)
        {
            indices[idx++] = botCenter;
            indices[idx++] = botCenter + 1 + i;
            indices[idx++] = botCenter + 2 + i;
        }
    }
};

static CylinderMesh<16> gCylinderMesh;

template<uint32 LAT, uint32 LON>
struct CapsuleMesh
{
    // LAT = hemisphere vertical segments
    // LON = radial segments

    static constexpr uint32 HEMI_VERTS = (LAT + 1) * (LON + 1);
    static constexpr uint32 CYL_VERTS  = (LON + 1) * 2;
    static constexpr uint32 VERTS      = HEMI_VERTS * 2 + CYL_VERTS;

    static constexpr uint32 HEMI_IND   = LAT * LON * 6;
    static constexpr uint32 CYL_IND    = LON * 6;
    static constexpr uint32 INDICES    = HEMI_IND * 2 + CYL_IND;

    VertexNormal vertices[VERTS]{};
    uint32 indices[INDICES]{};

    explicit constexpr CapsuleMesh(float radius = 0.5f, float height = 2.f)
    {
        const float halfBody = (height * 0.5f) - radius;

        uint32 v = 0;

        // =========================================================
        // Top hemisphere (+Y)
        // =========================================================
        for (uint32 y = 0; y <= LAT; ++y)
        {
            const float vlat = static_cast<float>(y) / LAT;
            const float phi  = vlat * (3.14159265358979f * 0.5f);

            for (uint32 x = 0; x <= LON; ++x)
            {
                const float ulon  = static_cast<float>(x) / LON;
                const float theta = ulon * 6.28318530718f;

                const float sx = cos(phi) * cos(theta);
                const float sy = sin(phi);
                const float sz = cos(phi) * sin(theta);

                vertices[v++] = {
                    { radius * sx, halfBody + radius * sy, radius * sz },
                    { sx, sy, sz }
                };
            }
        }

        const uint32 topHemiStart = 0;
        const uint32 cylStart     = v;

        // =========================================================
        // Cylinder body
        // =========================================================
        for (uint32 i = 0; i <= LON; ++i)
        {
            const float u   = static_cast<float>(i) / LON;
            const float ang = u * 6.28318530718f;

            const float x = cos(ang);
            const float z = sin(ang);

            vertices[v++] = {{ radius * x, -halfBody, radius * z }, { x, 0, z }};
            vertices[v++] = {{ radius * x,  halfBody, radius * z }, { x, 0, z }};
        }

        const uint32 botHemiStart = v;

        // =========================================================
        // Bottom hemisphere (-Y)
        // =========================================================
        for (uint32 y = 0; y <= LAT; ++y)
        {
            const float vlat = static_cast<float>(y) / LAT;
            const float phi  = vlat * (3.14159265358979f * 0.5f);

            for (uint32 x = 0; x <= LON; ++x)
            {
                const float ulon  = static_cast<float>(x) / LON;
                const float theta = ulon * 6.28318530718f;

                const float sx = cos(phi) * cos(theta);
                const float sy = -sin(phi);
                const float sz = cos(phi) * sin(theta);

                vertices[v++] = {
                    { radius * sx, -halfBody + radius * sy, radius * sz },
                    { sx, sy, sz }
                };
            }
        }

        // =========================================================
        // Indices
        // =========================================================
        uint32 idx = 0;

        // ---- Top hemisphere ----
        for (uint32 y = 0; y < LAT; ++y)
        {
            for (uint32 x = 0; x < LON; ++x)
            {
                const uint32 i0 = topHemiStart + y * (LON + 1) + x;
                const uint32 i1 = i0 + 1;
                const uint32 i2 = i0 + (LON + 1);
                const uint32 i3 = i2 + 1;

                indices[idx++] = i0; indices[idx++] = i2; indices[idx++] = i1;
                indices[idx++] = i1; indices[idx++] = i2; indices[idx++] = i3;
            }
        }

        // ---- Cylinder ----
        for (uint32 i = 0; i < LON; ++i)
        {
            const uint32 b0 = cylStart + i * 2;
            const uint32 b1 = b0 + 1;
            const uint32 t0 = b0 + 2;
            const uint32 t1 = b0 + 3;

            indices[idx++] = b0; indices[idx++] = b1; indices[idx++] = t0;
            indices[idx++] = b1; indices[idx++] = t1; indices[idx++] = t0;
        }

        // ---- Bottom hemisphere ----
        for (uint32 y = 0; y < LAT; ++y)
        {
            for (uint32 x = 0; x < LON; ++x)
            {
                const uint32 i0 = botHemiStart + y * (LON + 1) + x;
                const uint32 i1 = i0 + 1;
                const uint32 i2 = i0 + (LON + 1);
                const uint32 i3 = i2 + 1;

                indices[idx++] = i0; indices[idx++] = i1; indices[idx++] = i2;
                indices[idx++] = i1; indices[idx++] = i3; indices[idx++] = i2;
            }
        }
    }
};

static CapsuleMesh<8, 16> gCapsuleMesh;

