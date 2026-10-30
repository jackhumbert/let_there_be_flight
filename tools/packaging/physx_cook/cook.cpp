// Cook triangle meshes into PhysX 3.4 "NXS MESH" v15 blobs, as the game's geometry cache holds
// them (BV4 midphase, no face remap table), for physicsColliderMesh.compiledGeometryBuffer.
//
//   cook.exe <in.bin> <out.bin>
//
// in.bin:  u32 count, then per mesh: u32 nbVerts, u32 nbTris, f32 verts[nbVerts*3], u32 tris[nbTris*3]
// out.bin: u32 count, then per mesh: u32 size, u8 blob[size] (size 0 = cooking failed)
// Build: build.ps1 (needs the PhysX 3.4 cooking libs, see README.md).

#include <PxPhysicsAPI.h>
#include <cooking/PxCooking.h>

#include <cstdio>
#include <vector>

using namespace physx;

static PxDefaultAllocator gAllocator;

struct Errors : PxErrorCallback
{
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        fprintf(stderr, "physx %d: %s (%s:%d)\n", int(code), message, file, line);
    }
} gErrors;

struct Blob : PxOutputStream
{
    std::vector<PxU8> data;
    PxU32 write(const void* src, PxU32 count) override
    {
        data.insert(data.end(), (const PxU8*)src, (const PxU8*)src + count);
        return count;
    }
};

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: cook <in.bin> <out.bin>\n");
        return 2;
    }
    FILE* in = fopen(argv[1], "rb");
    FILE* out = fopen(argv[2], "wb");
    if (!in || !out)
    {
        fprintf(stderr, "cannot open files\n");
        return 2;
    }

    PxFoundation* foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gAllocator, gErrors);
    PxCookingParams params{PxTolerancesScale()};
    params.midphaseDesc = PxMeshMidPhase::eBVH34;
    params.suppressTriangleMeshRemapTable = true;
    params.meshPreprocessParams = PxMeshPreprocessingFlags(0);
    PxCooking* cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, params);

    PxU32 count = 0;
    fread(&count, 4, 1, in);
    fwrite(&count, 4, 1, out);
    int failed = 0;
    for (PxU32 m = 0; m < count; m++)
    {
        PxU32 nv = 0, nt = 0;
        fread(&nv, 4, 1, in);
        fread(&nt, 4, 1, in);
        std::vector<float> verts(size_t(nv) * 3);
        std::vector<PxU32> tris(size_t(nt) * 3);
        fread(verts.data(), 4, verts.size(), in);
        fread(tris.data(), 4, tris.size(), in);

        PxTriangleMeshDesc desc;
        desc.points.count = nv;
        desc.points.stride = 12;
        desc.points.data = verts.data();
        desc.triangles.count = nt;
        desc.triangles.stride = 12;
        desc.triangles.data = tris.data();

        Blob blob;
        PxU32 size = 0;
        if (cooking->cookTriangleMesh(desc, blob))
            size = PxU32(blob.data.size());
        else
            failed++;
        fwrite(&size, 4, 1, out);
        if (size)
            fwrite(blob.data.data(), 1, size, out);
    }
    fclose(in);
    fclose(out);
    cooking->release();
    foundation->release();
    fprintf(stderr, "cooked %u meshes, %d failed\n", count - failed, failed);
    return failed ? 1 : 0;
}
