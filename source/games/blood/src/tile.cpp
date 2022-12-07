//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build.h"

#include "blood.h"
#include "hw_voxels.h"
#include "tilesetbuilder.h"

BEGIN_BLD_NS


int nTileFiles = 0;

// these arrays get partially filled by .def, so they need to remain global.
static uint8_t surfType[kMaxTiles];
static int8_t tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];

#define x(a, b) registerName(#a, b);
static void SetTileNames(TilesetBuildInfo& info)
{
    auto registerName = [&](const char* name, int index)
    {
        info.addName(name, index);
    };
#include "namelist.h"
    // Oh Joy! Plasma Pak changes the tile number of the title screen, but we preferably want mods that use the original one to display it, e.g. Cryptic Passage
    // So let's make this remapping depend on the CRC.
    const int OTITLE = 2046, PTITLE = 2518;
    auto& orgtitle = info.tile[OTITLE];
    auto& pptile = info.tile[PTITLE];

    if (tileGetCRC32(pptile.tileimage) == 1170870757 && (tileGetCRC32(orgtitle.tileimage) != 290208654 || pptile.tileimage->GetWidth() == 0)) registerName("titlescreen", 2046);
    else registerName("titlescreen", 2518);
}
#undef x

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::LoadTextureInfo(TilesetBuildInfo& info)
{
    auto hFile = fileSystem.OpenFileReader("SURFACE.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(surfType, sizeof(surfType));
    }
    hFile = fileSystem.OpenFileReader("VOXEL.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(voxelIndex, sizeof(voxelIndex));
#if WORDS_BIGENDIAN
        for (int i = 0; i < kMaxTiles; i++)
            voxelIndex[i] = LittleShort(voxelIndex[i]);
#endif
    }
    hFile = fileSystem.OpenFileReader("SHADE.DAT");
    if (hFile.isOpen())
    {
		hFile.Read(tileShade, sizeof(tileShade));
    }
    for (int i = 0; i < kMaxTiles; i++)
    {
        if (voxelIndex[i] >= 0 && voxelIndex[i] < MAXVOXELS)
            voxreserve.Set(voxelIndex[i]);
    }
}

void GameInterface::SetupSpecialTextures(TilesetBuildInfo& info)
{
    SetTileNames(info);
    // set up all special tiles here, before we fully hook up with the texture manager.
    info.Delete(504);
    info.MakeWritable(2342);

}

void tileInitProps()
{
    for (int i = 0; i < MAXTILES; i++)
    {
        auto tex = tileGetTexture(i);
        if (tex)
        {
            TextureAttr a = { surfType[i], tileShade[i], voxelIndex[i] };
            tprops.Set(tex->GetID().GetIndex(), a);
        }
    }
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int tileGetSurfType(CollisionBase& hit)
{
    switch (hit.type)
    {
    default:
        return 0;
    case kHitSector:
        return tprops[hit.hitSector->floortexture()].surfType;
    case kHitWall:
        return tprops[hit.hitWall->walltexture()].surfType;
    case kHitSprite:
        return tprops[hit.hitActor->spr.spritetexture()].surfType;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::SetTileProps(int tile, int surf, int vox, int shade)
{
    if (surf != INT_MAX) surfType[tile] = surf;
    if (vox != INT_MAX) voxelIndex[tile] = vox;
    if (shade != INT_MAX) tileShade[tile] = shade;

    mirrortile = tileGetTextureID(504);

}

END_BLD_NS
