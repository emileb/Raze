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

#include "build.h"

#include "blood.h"

BEGIN_BLD_NS

ZONE gStartZone[8];
#ifdef NOONE_EXTENSIONS
    ZONE gStartZoneTeam1[8];
    ZONE gStartZoneTeam2[8];
    bool gTeamsSpawnUsed = false;
#endif

void validateLinks()
{
    int snum = 0;
    for (auto& sect : sectors())
    {

        if (sect.upperLink && !sect.upperLink->GetOwner())
        {
            Printf(PRINT_HIGH, "Unpartnered upper link in sector %d\n", snum);
            sect.upperLink = nullptr;
        }
        if (sect.upperLink && !sect.upperLink->GetOwner())
        {
            Printf(PRINT_HIGH, "Unpartnered lower link in sector %d\n", snum);
            sect.lowerLink = nullptr;
        }
        snum++;
    }
}

void warpInit(void)
{
    #ifdef NOONE_EXTENSIONS
    int team1 = 0; int team2 = 0; gTeamsSpawnUsed = false; // increment if team start positions specified.
    #endif
    BloodLinearSpriteIterator it;
    while (auto actor = it.Next())
    {
        spritetype* pSprite = &actor->s();
            if (actor->hasX()) {
                XSPRITE *pXSprite = &actor->x();
                switch (pSprite->type) {
                    case kMarkerSPStart:
                        if (gGameOptions.nGameType < 2 && pXSprite->data1 >= 0 && pXSprite->data1 < kMaxPlayers) {
                            ZONE *pZone = &gStartZone[pXSprite->data1];
                            pZone->x = pSprite->x;
                            pZone->y = pSprite->y;
                            pZone->z = pSprite->z;
                            pZone->sector = pSprite->sector();
                            pZone->ang = pSprite->ang;
                        }
                        DeleteSprite(actor);
                        break;
                    case kMarkerMPStart:
                        if (pXSprite->data1 >= 0 && pXSprite->data2 < kMaxPlayers) {
                            if (gGameOptions.nGameType >= 2) {
                                // default if BB or teams without data2 specified
                                ZONE* pZone = &gStartZone[pXSprite->data1];
                                pZone->x = pSprite->x;
                                pZone->y = pSprite->y;
                                pZone->z = pSprite->z;
                                pZone->sector = pSprite->sector();
                                pZone->ang = pSprite->ang;
                            
                                #ifdef NOONE_EXTENSIONS
                                    // fill player spawn position according team of player in TEAMS mode.
                                    if (gModernMap && gGameOptions.nGameType == 3) {
                                        if (pXSprite->data2 == 1) {
                                            pZone = &gStartZoneTeam1[team1];
                                            pZone->x = pSprite->x;
                                            pZone->y = pSprite->y;
                                            pZone->z = pSprite->z;
                                            pZone->sector = pSprite->sector();
                                            pZone->ang = pSprite->ang;
                                            team1++;

                                        } else if (pXSprite->data2 == 2) {
                                            pZone = &gStartZoneTeam2[team2];
                                            pZone->x = pSprite->x;
                                            pZone->y = pSprite->y;
                                            pZone->z = pSprite->z;
                                            pZone->sector = pSprite->sector();
                                            pZone->ang = pSprite->ang;
                                            team2++;
                                        }
                                    }
                                #endif

                            }
                            DeleteSprite(actor);
                        }
                        break;
                    case kMarkerUpLink:
                        pSprite->sector()->upperLink = actor;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        break;
                    case kMarkerLowLink:
                        pSprite->sector()->lowerLink = actor;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        break;
                    case kMarkerUpWater:
                    case kMarkerUpStack:
                    case kMarkerUpGoo:
                        pSprite->sector()->upperLink = actor;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        pSprite->z = getflorzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
                        break;
                    case kMarkerLowWater:
                    case kMarkerLowStack:
                    case kMarkerLowGoo:
                        pSprite->sector()->lowerLink = actor;
                        pSprite->cstat |= 32768;
                        pSprite->cstat &= ~257;
                        pSprite->z = getceilzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
                        break;
                }
            }
    }
    
    #ifdef NOONE_EXTENSIONS
    // check if there is enough start positions for teams, if any used
    if (team1 > 0 || team2 > 0) {
        gTeamsSpawnUsed = true;
        if (team1 < kMaxPlayers / 2 || team2 < kMaxPlayers / 2) {
            viewSetSystemMessage("At least 4 spawn positions for each team is recommended.");
            viewSetSystemMessage("Team A positions: %d, Team B positions: %d.", team1, team2);
        }
    }
    #endif

    for (int i = 0; i < numsectors; i++)
    {
        auto actor = getUpperLink(i);
        if (actor && actor->hasX())
        {
            spritetype *pSprite = &actor->s();
            XSPRITE *pXSprite = &actor->x();
            int nLink = pXSprite->data1;
            for (int j = 0; j < numsectors; j++)
            {
                auto actor2 = getLowerLink(j);
                if (actor2 && actor2->hasX())
                {
                    spritetype *pSprite2 = &actor2->s();
                    XSPRITE *pXSprite2 = &actor2->x();
                    if (pXSprite2->data1 == nLink)
                    {
                        actor->SetOwner(actor2);
                        actor2->SetOwner(actor);
                    }
                }
            }
        }
    }
	validateLinks();
}

int CheckLink(DBloodActor *actor)
{
    auto pSprite = &actor->s();
    int nSector = pSprite->sectnum;
    auto aUpper = getUpperLink(nSector);
    auto aLower = getLowerLink(nSector);
    if (aUpper)
    {
        spritetype* pUpper = &aUpper->s();
        int z;
        if (pUpper->type == kMarkerUpLink)
            z = pUpper->z;
        else
            z = getflorzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
        if (z <= pSprite->z)
        {
            aLower = aUpper->GetOwner();
            assert(aLower);
            spritetype *pLower = &aLower->s();
            assert(validSectorIndex(pLower->sectnum));
            ChangeActorSect(actor, pLower->sector());
            pSprite->x += pLower->x-pUpper->x;
            pSprite->y += pLower->y-pUpper->y;
            int z2;
            if (pLower->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
            pSprite->z += z2-z;
            actor->interpolated = false;
            return pUpper->type;
        }
    }
    if (aLower)
    {
        spritetype *pLower = &aLower->s();
        int z;
        if (pLower->type == kMarkerLowLink)
            z = pLower->z;
        else
            z = getceilzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
        if (z >= pSprite->z)
        {
            aUpper = aLower->GetOwner();
            assert(aUpper);
            spritetype *pUpper = &aUpper->s();
			assert(validSectorIndex(pUpper->sectnum));
            ChangeActorSect(actor, pUpper->sector());
            pSprite->x += pUpper->x-pLower->x;
            pSprite->y += pUpper->y-pLower->y;
            int z2;
            if (pUpper->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslopeptr(pSprite->sector(), pSprite->x, pSprite->y);
            pSprite->z += z2-z;
            actor->interpolated = false;
            return pLower->type;
        }
    }
    return 0;
}

int CheckLink(int *x, int *y, int *z, int *nSector)
{
    auto upper = getUpperLink(*nSector);
    auto lower = getLowerLink(*nSector);
    if (upper)
    {
        spritetype *pUpper = &upper->s();
        int z1;
        if (pUpper->type == kMarkerUpLink)
            z1 = pUpper->z;
        else
            z1 = getflorzofslope(*nSector, *x, *y);
        if (z1 <= *z)
        {
            lower = upper->GetOwner();
            assert(lower);
            spritetype *pLower = &lower->s();
			assert(validSectorIndex(pLower->sectnum));
            *nSector = pLower->sectnum;
            *x += pLower->x-pUpper->x;
            *y += pLower->y-pUpper->y;
            int z2;
            if (pUpper->type == kMarkerLowLink)
                z2 = pLower->z;
            else
                z2 = getceilzofslope(*nSector, *x, *y);
            *z += z2-z1;
            return pUpper->type;
        }
    }
    if (lower)
    {
        spritetype *pLower = &lower->s();
        int z1;
        if (pLower->type == kMarkerLowLink)
            z1 = pLower->z;
        else
            z1 = getceilzofslope(*nSector, *x, *y);
        if (z1 >= *z)
        {
            upper = lower->GetOwner();
            assert(upper);
            spritetype *pUpper = &upper->s();
			assert(validSectorIndex(pUpper->sectnum));
            *nSector = pUpper->sectnum;
            *x += pUpper->x-pLower->x;
            *y += pUpper->y-pLower->y;
            int z2;
            if (pLower->type == kMarkerUpLink)
                z2 = pUpper->z;
            else
                z2 = getflorzofslope(*nSector, *x, *y);
            *z += z2-z1;
            return pLower->type;
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, ZONE& w, ZONE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("x", w.x)
			("y", w.y)
			("z", w.z)
			("sector", w.sector)
			("angle", w.ang)
			.EndObject();
	}
	return arc;
}

void SerializeWarp(FSerializer& arc)
{
	if (arc.BeginObject("warp"))
	{
		arc.Array("startzone", gStartZone, kMaxPlayers)
			.EndObject();
	}
}

END_BLD_NS
