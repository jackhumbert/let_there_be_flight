// LTBF R41 (macOS): offline test of the GetMatrixFromOrientation replacement (src/red4ext/Hooks/GetMatrixFromOrientation.hpp).
// Checks the identity, a 90 degree yaw, and that each row is the image of its basis axis under the SDK's own quaternion rotation
// (Quaternion::Transform) for a few arbitrary rotations. Build and run: macos/tests/run.sh
#include <cmath>
#include <cstdio>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>

#include "Hooks/GetMatrixFromOrientation.hpp"

static int g_failures = 0;

static void Expect(bool aOk, const char* aWhat)
{
    if (!aOk)
    {
        std::printf("FAIL %s\n", aWhat);
        ++g_failures;
    }
}

static bool Near(float a, float b)
{
    return std::fabs(a - b) < 1e-5f;
}

static bool RowIs(const RED4ext::Vector4& aRow, float x, float y, float z, float w)
{
    return Near(aRow.X, x) && Near(aRow.Y, y) && Near(aRow.Z, z) && Near(aRow.W, w);
}

int main()
{
    RED4ext::Matrix m{};

    RED4ext::Quaternion identity(0.0f, 0.0f, 0.0f, 1.0f);
    GetMatrixFromOrientation(&identity, &m);
    Expect(RowIs(m.X, 1, 0, 0, 0) && RowIs(m.Y, 0, 1, 0, 0) && RowIs(m.Z, 0, 0, 1, 0) && RowIs(m.W, 0, 0, 0, 1), "identity");

    const float h = std::sqrt(0.5f);
    RED4ext::Quaternion yaw90(0.0f, 0.0f, h, h); // 90 degrees about +Z
    GetMatrixFromOrientation(&yaw90, &m);
    Expect(RowIs(m.X, 0, 1, 0, 0) && RowIs(m.Y, -1, 0, 0, 0) && RowIs(m.Z, 0, 0, 1, 0), "yaw 90");

    RED4ext::Quaternion scaled(0.0f, 0.0f, 3.0f * h, 3.0f * h); // not unit length: normalized first
    GetMatrixFromOrientation(&scaled, &m);
    Expect(RowIs(m.X, 0, 1, 0, 0), "yaw 90, length 3");

    RED4ext::Quaternion zero(0.0f, 0.0f, 0.0f, 0.0f);
    GetMatrixFromOrientation(&zero, &m);
    Expect(RowIs(m.X, 1, 0, 0, 0) && RowIs(m.W, 0, 0, 0, 1), "zero quaternion stays zero (rows from the formula)");

    const float samples[][4] = {{0.1f, 0.2f, 0.3f, 0.9f}, {-0.5f, 0.5f, -0.5f, 0.5f}, {0.7f, 0.0f, 0.0f, 0.71414f}};
    for (const auto& s : samples)
    {
        RED4ext::Quaternion q(s[0], s[1], s[2], s[3]);
        GetMatrixFromOrientation(&q, &m);
        // The SDK's own rotation (Quaternion::Transform, which normalizes first) of each basis axis is the matching row.
        const RED4ext::Vector4 ex = q.Transform(RED4ext::Vector4(1, 0, 0, 0));
        const RED4ext::Vector4 ey = q.Transform(RED4ext::Vector4(0, 1, 0, 0));
        const RED4ext::Vector4 ez = q.Transform(RED4ext::Vector4(0, 0, 1, 0));
        Expect(RowIs(m.X, ex.X, ex.Y, ex.Z, 0) && RowIs(m.Y, ey.X, ey.Y, ey.Z, 0) && RowIs(m.Z, ez.X, ez.Y, ez.Z, 0),
               "rows are the rotated axes (SDK Quaternion::Transform)");
    }

    std::printf("matrix_test: %s (%d failure(s))\n", g_failures ? "FAILED" : "ok", g_failures);
    return g_failures ? 1 : 0;
}
