/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "AirCAI.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#include <cassert>
#define AUTO_GENERATE_ATTACK_ORDERS 1

CR_BIND_DERIVED(CAirCAI, CMobileCAI, );

CR_REG_METADATA(CAirCAI, (
				CR_MEMBER(basePos),
				CR_MEMBER(baseDir),

				CR_MEMBER(activeCommand),
				CR_MEMBER(targetAge),

				CR_MEMBER(lastPC1),
				CR_MEMBER(lastPC2),
				CR_RESERVED(16)
				));

CAirCAI::CAirCAI()
	: CMobileCAI()
	, activeCommand(0)
	, targetAge(0)
	, lastPC1(-1)
	, lastPC2(-1)
{}

CAirCAI::CAirCAI(CUnit* owner)
	: CMobileCAI(owner)
	, activeCommand(0)
	, targetAge(0)
	, lastPC1(-1)
	, lastPC2(-1)
{
	cancelDistance = 16000;
	CommandDescription c;

	if (owner->unitDef->canAttack) {
		c.id = CMD_AREA_ATTACK;
		c.action = "areaattack";
		c.type = CMDTYPE_ICON_AREA;
		c.name = "Area attack";
		c.mouseicon = c.name;
		c.tooltip = "Sets the aircraft to attack enemy units within a circle";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canLoopbackAttack) {
		c.params.clear();
		c.id = CMD_LOOPBACKATTACK;
		c.action = "loopbackattack";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Loopback";
		c.mouseicon = c.name;
		c.params.push_back("0");
		c.params.push_back("Normal");
		c.params.push_back("Loopback");
		c.tooltip = "Loopback attack: Sets if the aircraft should loopback after an attack instead of overflying target";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_LOOPBACKATTACK);
	}

	basePos = owner->pos;
	goalPos = owner->pos;
}

CAirCAI::~CAirCAI()
{
}

void CAirCAI::GiveCommandReal(const Command& c)
{
	// take care not to allow aircraft to be ordered to move out of the map
	if ((c.GetID() != CMD_MOVE) && !AllowedCommand(c, true)) {
		return;
	} else if (c.GetID() == CMD_MOVE && c.params.size() >= 3 &&
			(c.params[0] < 0.0f || c.params[2] < 0.0f
			 || c.params[0] > gs->mapx*SQUARE_SIZE
			 || c.params[2] > gs->mapy*SQUARE_SIZE))
	{
		return;
	}

	if (c.GetID() == CMD_SET_WANTED_MAX_SPEED) {
	  return;
	}

	if (c.GetID() == CMD_AUTOREPAIRLEVEL) {
		if (c.params.empty()) {
			return;
		}
		CStrafeAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CStrafeAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CStrafeAirMoveType*)owner->moveType;
		}
		switch ((int)c.params[0]) {
			case 0: { airMT->SetRepairBelowHealth(0.0f); break; }
			case 1: { airMT->SetRepairBelowHealth(0.3f); break; }
			case 2: { airMT->SetRepairBelowHealth(0.5f); break; }
			case 3: { airMT->SetRepairBelowHealth(0.8f); break; }
		}
		for (vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi)
		{
			if (cdi->id == CMD_AUTOREPAIRLEVEL) {
				char t[10];
				SNPRINTF(t,10,"%d", (int)c.params[0]);
				cdi->params[0]=t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (c.GetID() == CMD_IDLEMODE) {
		if (c.params.empty()) {
			return;
		}
		CStrafeAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CStrafeAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CStrafeAirMoveType*)owner->moveType;
		}
		switch ((int)c.params[0]){
			case 0: { airMT->autoLand = false; break; }
			case 1: { airMT->autoLand = true;  break; }
		}
		for (vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi)
		{
			if (cdi->id == CMD_IDLEMODE){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (c.GetID() == CMD_LOOPBACKATTACK) {
		if (c.params.empty()) {
			return;
		}
		CStrafeAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CStrafeAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CStrafeAirMoveType*)owner->moveType;
		}
		switch ((int)c.params[0]) {
			case 0: { airMT->loopbackAttack = false; break; }
			case 1: { airMT->loopbackAttack = true;  break; }
		}
		for (vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi)
		{
			if (cdi->id == CMD_LOOPBACKATTACK){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (!(c.options & SHIFT_KEY)
			&& nonQueingCommands.find(c.GetID()) == nonQueingCommands.end())
	{
		activeCommand = 0;
		tempOrder = false;
	}

	if (c.GetID() == CMD_AREA_ATTACK && c.params.size() < 4) {
		Command c2(CMD_ATTACK, c.options);
		c2.params = c.params;
		CCommandAI::GiveAllowedCommand(c2);
		return;
	}

	CCommandAI::GiveAllowedCommand(c);
}

void CAirCAI::SlowUpdate()
{
	if (gs->paused) { // Commands issued may invoke SlowUpdate when paused
		return;
	}
	if (!commandQue.empty() && (commandQue.front().timeOut < gs->frameNum)) {
		FinishCommand();
		return;
	}

	if (owner->usingScriptMoveType) {
		return; // avoid the invalid (CStrafeAirMoveType*) cast
	}

	AAirMoveType* myPlane = (AAirMoveType*) owner->moveType;

	bool wantToRefuel = LandRepairIfNeeded();
	if (!wantToRefuel && owner->unitDef->maxFuel > 0) {
		wantToRefuel = RefuelIfNeeded();
	}

	#if (AUTO_GENERATE_ATTACK_ORDERS == 1)
	if (commandQue.empty()) {
		const bool autoLand = !owner->unitDef->DontLand() && myPlane->autoLand;
		const bool autoAttack = ((owner->fireState >= FIRESTATE_FIREATWILL) && (owner->moveState != MOVESTATE_HOLDPOS));

		if (myPlane->aircraftState == AAirMoveType::AIRCRAFT_FLYING && autoLand) {
			StopMove();
//			myPlane->SetState(AAirMoveType::AIRCRAFT_LANDING);
		}

		if (owner->unitDef->canAttack && autoAttack && owner->maxRange > 0) {
			if (myPlane->IsFighter()) {
				const float3 P = owner->pos + (owner->speed * 10.0);
				const float R = 1000.0f * owner->moveState;
				const CUnit* enemy = helper->GetClosestEnemyAircraft(P, R, owner->allyteam);

				if (IsValidTarget(enemy)) {
					Command nc(CMD_ATTACK);
					nc.params.push_back(enemy->id);
					nc.options = INTERNAL_ORDER;
					commandQue.push_front(nc);
					inCommand = false;
					return;
				}
			} else {
				const float3 P = owner->pos + (owner->speed * 20.0f);
				const float R = 500.0f * owner->moveState;
				const CUnit* enemy = helper->GetClosestValidTarget(P, R, owner->allyteam, this);

				if (enemy != NULL) {
					Command nc(CMD_ATTACK);
					nc.params.push_back(enemy->id);
					nc.options = INTERNAL_ORDER;
					commandQue.push_front(nc);
					inCommand = false;
					return;
				}
			}
		}

		return;
	}
	#endif

	Command& c = commandQue.front();
	const int& cmd_id = c.GetID();
	
	if (cmd_id == CMD_WAIT) {
		if ((myPlane->aircraftState == AAirMoveType::AIRCRAFT_FLYING)
		    	&& !owner->unitDef->DontLand() && myPlane->autoLand)
		{
			StopMove();
//			myPlane->SetState(AAirMoveType::AIRCRAFT_LANDING);
		}
		return;
	}

	if (cmd_id != CMD_STOP && cmd_id != CMD_AUTOREPAIRLEVEL
			&& cmd_id != CMD_IDLEMODE && cmd_id != CMD_SET_WANTED_MAX_SPEED)
	{
		myPlane->Takeoff();
	}

	if (wantToRefuel) {
		switch (cmd_id) {
			case CMD_AREA_ATTACK:
			case CMD_ATTACK:
			case CMD_FIGHT:
				return;
		}
	}

	switch (cmd_id) {
		case CMD_AREA_ATTACK: {
			ExecuteAreaAttack(c);
			return;
		}
		default: {
			CMobileCAI::Execute();
			return;
		}
	}
}



void CAirCAI::ExecuteFight(Command& c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
	AAirMoveType* myPlane = (AAirMoveType*) owner->moveType;
	if (tempOrder) {
		tempOrder = false;
		inCommand = true;
	}
	if (c.params.size() < 3) { // this shouldn't happen but anyway ...
		LOG_L(L_ERROR,
				"Received a Fight command with less than 3 params on %s in AirCAI",
				owner->unitDef->humanName.c_str());
		return;
	}
	if (c.params.size() >= 6) {
		if (!inCommand) {
			commandPos1 = float3(c.params[3], c.params[4], c.params[5]);
		}
	} else {
		// HACK to make sure the line (commandPos1,commandPos2) is NOT
		// rotated (only shortened) if we reach this because the previous return
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(127*127)){'
		// condition, but is actually updated correctly if you click somewhere
		// outside the area close to the line (for a new command).
		commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		if ((owner->pos - commandPos1).SqLength2D() > (150 * 150)) {
			commandPos1 = owner->pos;
		}
	}
	goalPos = c.GetPos(0);
	if (!inCommand) {
		inCommand = true;
		commandPos2 = goalPos;
	}
	if (c.params.size() >= 6) {
		goalPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	}

	// CMD_FIGHT is pretty useless if !canAttack, but we try to honour the modders wishes anyway...
	if (owner->unitDef->canAttack && (owner->fireState >= FIRESTATE_FIREATWILL)
			&& (owner->moveState != MOVESTATE_HOLDPOS) && (owner->maxRange > 0))
	{
		CUnit* enemy = NULL;

		if (myPlane->IsFighter()) {
			const float3 P = ClosestPointOnLine(commandPos1, commandPos2, owner->pos + owner->speed*10);
			const float R = 1000.0f * owner->moveState;

			enemy = helper->GetClosestEnemyAircraft(P, R, owner->allyteam);
		}
		if (IsValidTarget(enemy) && (owner->moveState != MOVESTATE_MANEUVER
				|| LinePointDist(commandPos1, commandPos2, enemy->pos) < 1000))
		{
			Command nc(CMD_ATTACK, c.options | INTERNAL_ORDER);
			nc.params.push_back(enemy->id);
			commandQue.push_front(nc);
			tempOrder = true;
			inCommand = false;

			if (lastPC1 != gs->frameNum) { // avoid infinite loops
				lastPC1 = gs->frameNum;
				SlowUpdate();
			}
			return;
		} else {
			const float3 P = ClosestPointOnLine(commandPos1, commandPos2, owner->pos + owner->speed * 20);
			const float R = 500.0f * owner->moveState;

			enemy = helper->GetClosestValidTarget(P, R, owner->allyteam, this);

			if (enemy != NULL) {
				Command nc(CMD_ATTACK, c.options | INTERNAL_ORDER);
				nc.params.push_back(enemy->id);
				PushOrUpdateReturnFight();
				commandQue.push_front(nc);
				tempOrder = true;
				inCommand = false;

				if (lastPC2 != gs->frameNum) { // avoid infinite loops
					lastPC2 = gs->frameNum;
					SlowUpdate();
				}
				return;
			}
		}
	}

	myPlane->goalPos = goalPos;

	const CStrafeAirMoveType* airMT = dynamic_cast<const CStrafeAirMoveType*>(myPlane);
	const float radius = (airMT != NULL)? std::max(airMT->turnRadius + 2*SQUARE_SIZE, 128.f) : 127.f;

	// we're either circling or will get to the target in 8 frames
	if ((owner->pos - goalPos).SqLength2D() < (radius * radius)
			|| (owner->pos + owner->speed*8 - goalPos).SqLength2D() < 127*127)
	{
		FinishCommand();
	}
}

void CAirCAI::ExecuteAttack(Command& c)
{
	assert(owner->unitDef->canAttack);
	targetAge++;

	if (tempOrder && owner->moveState == MOVESTATE_MANEUVER) {
		// limit how far away we fly
		if (orderTarget && LinePointDist(commandPos1, commandPos2, orderTarget->pos) > 1500) {
			owner->AttackUnit(NULL, false, false);
			FinishCommand();
			return;
		}
	}

	if (inCommand) {
		if (targetDied || (c.params.size() == 1 && UpdateTargetLostTimer(int(c.params[0])) == 0)) {
			FinishCommand();
			return;
		}
		if ((c.params.size() == 3) && (owner->commandShotCount < 0) && (commandQue.size() > 1)) {
			owner->AttackGround(c.GetPos(0), (c.options & INTERNAL_ORDER) == 0, true);
			FinishCommand();
			return;
		}
		if (orderTarget) {
			if (orderTarget->unitDef->canfly && orderTarget->crashing) {
				owner->AttackUnit(NULL, false, false);
				FinishCommand();
				return;
			}
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				owner->AttackUnit(NULL, false, false);
				FinishCommand();
				return;
			}
		}
	} else {
		targetAge = 0;
		owner->commandShotCount = -1;

		if (c.params.size() == 1) {
			CUnit* targetUnit = uh->GetUnit(c.params[0]);

			if (targetUnit == NULL) { FinishCommand(); return; }
			if (targetUnit == owner) { FinishCommand(); return; }
			if (targetUnit->GetTransporter() != NULL && !modInfo.targetableTransportedUnits) {
				FinishCommand(); return;
			}

			SetGoal(targetUnit->pos, owner->pos, cancelDistance);
			SetOrderTarget(targetUnit);
			owner->AttackUnit(targetUnit, (c.options & INTERNAL_ORDER) == 0, false);

			inCommand = true;
		} else {
			owner->AttackGround(c.GetPos(0), (c.options & INTERNAL_ORDER) == 0, false);
			inCommand = true;
		}
	}

	return;
}

void CAirCAI::ExecuteAreaAttack(Command& c)
{
	assert(owner->unitDef->canAttack);
	AAirMoveType* myPlane = (AAirMoveType*) owner->moveType;

	if (targetDied) {
		targetDied = false;
		inCommand = false;
	}

	const float3 pos = c.GetPos(0);
	const float radius = c.params[3];

	if (inCommand) {
		if (myPlane->aircraftState == AAirMoveType::AIRCRAFT_LANDED)
			inCommand = false;
		if (orderTarget && orderTarget->pos.SqDistance2D(pos) > Square(radius)) {
			inCommand = false;
			SetOrderTarget(NULL);
		}
		if (owner->commandShotCount < 0) {
			if ((c.params.size() == 4) && (commandQue.size() > 1)) {
				owner->AttackUnit(NULL, (c.options & INTERNAL_ORDER) == 0, true);
				FinishCommand();
			}
			else if (owner->userAttackGround) {
				// reset the attack position after each run
				float3 attackPos = pos + (gs->randVector() * radius);
				attackPos.y = ground->GetHeightAboveWater(attackPos.x, attackPos.z);

				owner->AttackGround(attackPos, (c.options & INTERNAL_ORDER) == 0, false);
				owner->commandShotCount = 0;
			}
		}
	} else {
		owner->commandShotCount = -1;

		if (myPlane->aircraftState != AAirMoveType::AIRCRAFT_LANDED) {
			inCommand = true;

			std::vector<int> enemyUnitIDs;
			helper->GetEnemyUnits(pos, radius, owner->allyteam, enemyUnitIDs);

			if (enemyUnitIDs.empty()) {
				float3 attackPos = pos + gs->randVector() * radius;
				attackPos.y = ground->GetHeightAboveWater(attackPos.x, attackPos.z);
				owner->AttackGround(attackPos, (c.options & INTERNAL_ORDER) == 0, false);
			} else {
				// note: the range of randFloat() is inclusive of 1.0f
				// note: assumes area-attacks can only be issued by users, not CAI's
				const unsigned int idx(gs->randFloat() * (enemyUnitIDs.size() - 1));
				CUnit* targetUnit = uh->GetUnitUnsafe(enemyUnitIDs[idx]);

				SetOrderTarget(targetUnit);
				owner->AttackUnit(targetUnit, false, false);
			}
		}
	}
}

void CAirCAI::ExecuteGuard(Command& c)
{
	assert(owner->unitDef->canGuard);

	const CUnit* guardee = uh->GetUnit(c.params[0]);

	if (guardee == NULL) { FinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { FinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { FinishCommand(); return; }

	const bool pushAttackCommand =
		(owner->maxRange > 0.0f) &&
		owner->unitDef->canAttack &&
		((guardee->lastAttack + 40) < gs->frameNum) &&
		IsValidTarget(guardee->lastAttacker);

	if (pushAttackCommand) {
		Command nc(CMD_ATTACK, c.options | INTERNAL_ORDER);
		nc.params.push_back(guardee->lastAttacker->id);
		commandQue.push_front(nc);
		SlowUpdate();
	} else {
		Command c2(CMD_MOVE, c.options | INTERNAL_ORDER);
		c2.timeOut = gs->frameNum + 60;

		if (guardee->pos.IsInBounds()) {
			c2.params.push_back(guardee->pos.x);
			c2.params.push_back(guardee->pos.y);
			c2.params.push_back(guardee->pos.z);
		} else {
			float3 clampedGuardeePos = guardee->pos;

			clampedGuardeePos.ClampInBounds();

			c2.params.push_back(clampedGuardeePos.x);
			c2.params.push_back(clampedGuardeePos.y);
			c2.params.push_back(clampedGuardeePos.z);
		}

		commandQue.push_front(c2);
	}
}

int CAirCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack) {
				return CMD_ATTACK;
			}
		} else {
			if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	return CMD_MOVE;
}

bool CAirCAI::IsValidTarget(const CUnit* enemy) const {
	return CMobileCAI::IsValidTarget(enemy) && !enemy->crashing
		&& (((CStrafeAirMoveType*)owner->moveType)->isFighter || !enemy->unitDef->canfly);
}



void CAirCAI::FinishCommand()
{
	targetAge = 0;
	CCommandAI::FinishCommand();
}

void CAirCAI::BuggerOff(const float3& pos, float radius)
{
	AAirMoveType* myPlane = dynamic_cast<AAirMoveType*>(owner->moveType);
	if (myPlane) {
		myPlane->Takeoff();
	} else {
		CMobileCAI::BuggerOff(pos, radius);
	}
}

void CAirCAI::SetGoal(const float3& pos, const float3& curPos, float goalRadius)
{
	owner->moveType->SetGoal(pos);
	CMobileCAI::SetGoal(pos, curPos, goalRadius);
}
