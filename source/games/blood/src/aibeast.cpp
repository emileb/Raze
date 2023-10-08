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


AISTATE beastIdle = { kAiStateIdle, 0, nullptr, 0, NULL, NULL, &AF(aiThinkTarget), NULL };
AISTATE beastChase = { kAiStateChase, 8, nullptr, 0, NULL, &AF(beastMoveForward), &AF(beastThinkChase), NULL };
AISTATE beastDodge = { kAiStateMove, 8, nullptr, 60, NULL, &AF(aiMoveDodge), NULL, &beastChase };
AISTATE beastGoto = { kAiStateMove, 8, nullptr, 600, NULL, &AF(beastMoveForward), &AF(beastThinkGoto), &beastIdle };
AISTATE beastSlash = { kAiStateChase, 6, &AF(SlashSeqCallback), 120, NULL, NULL, NULL, &beastChase };
AISTATE beastStomp = { kAiStateChase, 7, &AF(StompSeqCallback), 120, NULL, NULL, NULL, &beastChase };
AISTATE beastSearch = { kAiStateSearch, 8, nullptr, 120, NULL, &AF(beastMoveForward), &AF(beastThinkSearch), &beastIdle };
AISTATE beastRecoil = { kAiStateRecoil, 5, nullptr, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastTeslaRecoil = { kAiStateRecoil, 4, nullptr, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastSwimIdle = { kAiStateIdle, 9, nullptr, 0, NULL, NULL, &AF(aiThinkTarget), NULL };
AISTATE beastSwimChase = { kAiStateChase, 9, nullptr, 0, NULL, &AF(beastMoveSwim), &AF(beastThinkSwimChase), NULL };
AISTATE beastSwimDodge = { kAiStateMove, 9, nullptr, 90, NULL, &AF(aiMoveDodge), NULL, &beastSwimChase };
AISTATE beastSwimGoto = { kAiStateMove, 9, nullptr, 600, NULL, &AF(beastMoveForward), &AF(beastThinkSwimGoto), &beastSwimIdle };
AISTATE beastSwimSearch = { kAiStateSearch, 9, nullptr, 120, NULL, &AF(beastMoveForward), &AF(beastThinkSearch), &beastSwimIdle };
AISTATE beastSwimSlash = { kAiStateChase, 9, &AF(SlashSeqCallback), 0, NULL, NULL, &AF(beastThinkSwimChase), &beastSwimChase };
AISTATE beastSwimRecoil = { kAiStateRecoil, 5, nullptr, 0, NULL, NULL, NULL, &beastSwimDodge };
AISTATE beastMorphToBeast = { kAiStateOther, -1, nullptr, 0, &AF(MorphToBeast), NULL, NULL, &beastIdle };
AISTATE beastMorphFromCultist = { kAiStateOther, 2576, nullptr, 0, NULL, NULL, NULL, &beastMorphToBeast };
AISTATE beastMoveSwimChaseAlt = { kAiStateOther, 9, nullptr, 120, NULL, &AF(beastMoveSwimAlt), &AF(beastThinkSwimChase), &beastSwimChase };
AISTATE beastSwimAttack = { kAiStateOther, 9, nullptr, 0, NULL, &AF(beastMoveIn), &AF(beastThinkSwimChase), &beastSwimChase };
AISTATE beastSwimTurn = { kAiStateOther, 9, nullptr, 120, NULL, &AF(aiMoveTurn), NULL, &beastSwimChase };

void SlashSeqCallback(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DVector3 dv;
	dv.XY() = actor->spr.Angles.Yaw.ToVector();
	// Correct ?
	dv.Z = (actor->spr.pos.Z - target->spr.pos.Z) / 64;

	dv.X += Random3(4000 - 700 * gGameOptions.nDifficulty) / 16384.;
	dv.Y += Random3(4000 - 700 * gGameOptions.nDifficulty) / 16384.;

	actFireVector(actor, 0, 0, dv, kVectorGargSlash);
	actFireVector(actor, 0, 0, dv, kVectorGargSlash);
	actFireVector(actor, 0, 0, dv, kVectorGargSlash);
	sfxPlay3DSound(actor, 9012 + Random(2), -1, 0);
}

void StompSeqCallback(DBloodActor* actor)
{
	auto pos = actor->spr.pos;
	const int nDist = 400;
	auto pSector = actor->sector();
	int nBaseDamage = 5 + 2 * gGameOptions.nDifficulty;
	int nBaseDamage2 = 25 + 30 * gGameOptions.nDifficulty;

	const bool newSectCheckMethod = !cl_bloodvanillaenemies && !VanillaMode(); // use new sector checking logic
	auto sectorMap = GetClosestSpriteSectors(pSector, actor->spr.pos.XY(), nDist, nullptr, newSectCheckMethod);
	int hit = HitScan(actor, actor->spr.pos.Z, DVector3(actor->spr.Angles.Yaw.ToVector() * 1024, 0), CLIPMASK1, 0);
	DBloodActor* actorh = nullptr;
	actHitcodeToData(hit, &gHitInfo, &actorh);

	BloodStatIterator it1(kStatDude);
	while (auto actor2 = it1.Next())
	{
		if (actor != actor2)
		{
			if (actor2->hasX())
			{
				if (actor2->GetType() == kDudeBeast)
					continue;
				if (actor2->spr.flags & 32)
					continue;
				if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, pos, pSector, nDist << 4))
				{
					double top, bottom;
					GetActorExtents(actor, &top, &bottom);
					if (abs(bottom - pSector->floorz) == 0)
					{
						double nDist2 = (actor->spr.pos.XY() - actor2->spr.pos.XY()).Length();
						if (nDist2 <= nDist)
						{
							int nDamage;
							if (nDist2 <= 0)
								nDamage = nBaseDamage + nBaseDamage2;
							else
								nDamage = nBaseDamage + int(nBaseDamage2 *  ((nDist - nDist2) / nDist));
							if (actor2->IsPlayerActor())
								getPlayer(actor2)->quakeEffect += nDamage * 4;
							actDamageSprite(actor, actor2, kDamageFall, nDamage << 4);
						}
					}
				}
			}
		}
	}
	it1.Reset(kStatThing);
	while (auto actor2 = it1.Next())
	{
		if (actor2->spr.flags & 32)
			continue;
		if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, pos, pSector, nDist << 4))
		{
			if (actor2->xspr.locked)
				continue;

			double nDist2 = (actor->spr.pos.XY() - actor2->spr.pos.XY()).Length();
			if (nDist2 <= nDist)
			{
				int nDamage;
				if (nDist2 <= 0)
					nDamage = nBaseDamage + nBaseDamage2;
				else
					nDamage = nBaseDamage + int(nBaseDamage2 * ((nDist - nDist2) / nDist));

				if (actor2->IsPlayerActor())
					getPlayer(actor2)->quakeEffect += nDamage * 4;
				actDamageSprite(actor, actor2, kDamageFall, nDamage << 4);
			}
		}
	}
	sfxPlay3DSound(actor, 9015 + Random(2), -1, 0);
}

void MorphToBeast(DBloodActor* actor)
{
	actHealDude(actor, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
	actor->ChangeType(kDudeBeast);
}

void beastThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void beastThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	auto nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
	}
	aiThinkTarget(actor);
}

void beastThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		auto pSector = actor->sector();
		auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dv = target->spr.pos.XY() - actor->spr.pos.XY();
	aiChooseDirection(actor, dv.Angle());

	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (target->xspr.health == 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	double nDist = dv.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dv.Angle());
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : int(target->spr.pos.Z - actor->spr.pos.Z / nDist);
				if (nDist < 0x140 && nDist > 0xa0 && nDeltaAngle < DAngle15 && (target->spr.flags & 2)
					&& target->IsPlayerActor() && Chance(0x8000))
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv, 0), CLIPMASK1, 0);
					if (target->xspr.health > (unsigned)gPlayerTemplate[0].startHealth / 2)
					{
						switch (hit)
						{
						case -1:
							if (!pXSector || !pXSector->Underwater)
								aiNewState(actor, &beastStomp);
							break;
						case 3:
							if (actor->GetType() != gHitInfo.actor()->GetType())
							{
								if (!pXSector || !pXSector->Underwater)
									aiNewState(actor, &beastStomp);
							}
							else
							{
								if (pXSector && pXSector->Underwater)
									aiNewState(actor, &beastSwimDodge);
								else
									aiNewState(actor, &beastDodge);
							}
							break;
						default:
							if (!pXSector || !pXSector->Underwater)
								aiNewState(actor, &beastStomp);
							break;
						}
					}
				}
				if (nDist < 57.5625 && abs(nDeltaAngle) < mapangle(28))
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv, 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &beastSwimSlash);
						else
							aiNewState(actor, &beastSlash);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &beastSwimSlash);
							else
								aiNewState(actor, &beastSlash);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &beastSwimDodge);
							else
								aiNewState(actor, &beastDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &beastSwimSlash);
						else
							aiNewState(actor, &beastSlash);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, &beastSwimGoto);
	else
		aiNewState(actor, &beastGoto);
	actor->SetTarget(nullptr);
}

void beastThinkSwimGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	auto nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, &beastSwimSearch);
	aiThinkTarget(actor);
}

void beastThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &beastSwimGoto);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();

	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &beastSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &beastSwimSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 64 && nDeltaAngle < DAngle15)
					aiNewState(actor, &beastSwimSlash);
				else
				{
					aiPlay3DSound(actor, 9009 + Random(2), AI_SFX_PRIORITY_1, -1);
					aiNewState(actor, &beastSwimAttack);
				}
			}
		}
		else
			aiNewState(actor, &beastSwimAttack);
		return;
	}
	aiNewState(actor, &beastSwimGoto);
	actor->SetTarget(nullptr);
}

void beastMoveForward(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	if (abs(nAng) > DAngle60)
		return;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (nDist <= 0x40 && Random(64) < 32)
		return;
	actor->vel.XY() += actor->spr.Angles.Yaw.ToVector() * pDudeInfo->FrontSpeed();
}

void beastMoveSwim(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.Angles.Yaw += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Random(64) < 32 && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel * 0.25;
	});
}

void beastMoveSwimAlt(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();

	if (Chance(0x600) && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel;
	});

	double dz = target->spr.pos.Z - actor->spr.pos.Z;
	actor->vel.Z = -dz / 256;
}

void beastMoveIn(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;

	if (abs(nAng) > DAngle60)
	{
		actor->spr.Angles.Yaw += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();

	if (Chance(0x4000) && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});

	double dz = target->spr.pos.Z - actor->spr.pos.Z;
	actor->vel.Z = dz / 32;
}

END_BLD_NS
