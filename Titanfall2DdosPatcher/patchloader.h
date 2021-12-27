#ifndef PATCHLOADER_H
#define PATCHLOADER_H

#include <patch.h>
#include <memory>

class PatchLoader
{
public:
    PatchLoader() = delete;

    static std::shared_ptr<Patch> loadPatch_StokDokulus_Titanfall2_DeltaBuf_v1();
};

#endif // PATCHLOADER_H
