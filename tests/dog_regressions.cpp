/* Overture Remade, GNU GPL v3 or later. See COPYING. */
#include "core/StdAfx.h"
#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include <cmath>

// Like engine_regressions, exercise the actual implementation with controlled
// internal state. This TU is included only in BUILD_TESTING builds.
#define private public
#define protected public
#include "enemies/GameEnemy_Dog.h"
#include "systems/AttackHandler.h"
#undef protected
#undef private

namespace {
bool Near(float a, float b) { return std::abs(a - b) < 0.0001f; }
bool Near(const cVector3f &a, const cVector3f &b) { return cMath::Vector3DistSqr(a, b) < 0.000001f; }

class CompletionState : public iGameEnemyState_Dog_Base
{
public:
	CompletionState(cInit *init, iGameEnemy *enemy)
		: iGameEnemyState_Dog_Base(50, init, enemy) {}
	void OnEnterState(iGameEnemyState*) override {}
	void OnLeaveState(iGameEnemyState*) override {}
	void OnUpdate(float) override {}
	void OnAnimationOver(const tString&) override { ++completions; }
	int completions = 0;
};
}

int RunDogRegressions(cInit *init)
{
	const bool previousMode = init->GetImprovedDogAI();
	init->SetImprovedDogAI(true);
	int failures = 0, checks = 0;
	auto check = [&](bool ok, const char *name) {
		++checks;
		Log("DOGTEST: %s %s\n", ok ? "PASS" : "FAIL", name);
		if(!ok) ++failures;
	};
	const float dt = 1.0f / 60.0f;
	cWorld3D *world = init->mpGame->GetScene()->GetWorld3D();
	iCharacterBody *player = init->mpPlayer->GetCharacterBody();
	const cVector3f playerStart = player->GetFeetPosition();
	const cVector3f origin = player->GetFeetPosition() + cVector3f(0.15f, 0, 0);
	world->CreateEntity("dog_foundations_test", cMath::MatrixTranslate(origin),
		"character_dog.ent", true);
	iGameEntity *entity = init->mpMapHandler->GetGameEntity("dog_foundations_test", false);
	if(!entity || entity->GetSubType() != "Dog")
	{
		check(false, "original dog loads");
		return failures;
	}
	auto *dog = static_cast<cGameEnemy_Dog*>(entity);
	// The renderer/interaction test map has no navigation graph. Supply a
	// small local graph before the usual enemy world-load setup.
	for(int x = -2; x <= 2; ++x)
		for(int z = -2; z <= 2; ++z)
			world->AddAINode("dog_test_" + cString::ToString(x) + "_" + cString::ToString(z),
				dog->msGroundNodeType, origin + cVector3f(x, 0.05f, z));
	dog->OnWorldLoad();
	iCharacterBody *body = dog->GetMover()->GetCharBody();
	check(dog->GetMeshEntity()->GetAnimationStateFromName("AttackLow") &&
		dog->GetMeshEntity()->GetAnimationStateFromName("Walk"), "original clips load");
	if(failures) return failures;

	// Compare loaded AI settings with the original entity file, not new values.
	TiXmlDocument definition;
	const tString file = init->mpGame->GetResources()->GetFileSearcher()->GetFilePath("character_dog.ent");
	if(definition.LoadFile(file.c_str()))
	{
		TiXmlElement *game = definition.RootElement()->FirstChildElement("GAME");
		check(game && Near(dog->mfHuntMinSeeChance,
			cString::ToFloat(game->Attribute("HuntMinSeeChance"), dog->mfIdleMinSeeChance)),
			"hunt uses authored sight threshold");
		check(game && Near(dog->mfHuntMinHearVolume,
			cString::ToFloat(game->Attribute("HuntMinHearVolume"), dog->mfIdleMinHearVolume)),
			"hunt uses authored hearing threshold");
	}
	else check(false, "original entity definition is readable");

	// Idle -> Hunt during a proximity perception check must keep Hunt's FOV.
	const float seeDistance = dog->mfMaxSeeDist;
	const float idleChance = dog->mfIdleMinSeeChance;
	const float attentionDistance = dog->mfAttentionMinDist;
	dog->mfMaxSeeDist = 0.01f; // Use the original proximity branch, without rays.
	dog->mfIdleMinSeeChance = 0;
	dog->mfAttentionMinDist = 100;
	dog->mlPlayerInLOSCount = dog->mlMaxPlayerInLOSCount - 1;
	dog->mfCheckForPlayerCount = dog->mfCheckForPlayerRate;
	dog->UpdateCheckForPlayer(dt);
	check(dog->GetCurrentStateId() == STATE_HUNT, "perception enters hunt");
	check(Near(dog->mfFOV, dog->mfHuntFOV), "perception keeps the new state's FOV");
	dog->mfMaxSeeDist = seeDistance;
	dog->mfIdleMinSeeChance = idleChance;
	dog->mfAttentionMinDist = attentionDistance;
	check(dog->GetState(STATE_IDLE) == dog->mvStates[STATE_IDLE], "state lookup honours its argument");
	check(dog->GetState(-1) == nullptr && dog->GetState(1000) == nullptr,
		"invalid state lookup is safe");

	// A failed sight check must not sample the moving player's live position
	// during the legacy 1.5-second memory grace period.
	const cVector3f remembered = origin + cVector3f(0, 0, -1);
	dog->SetLastPlayerPos(remembered);
	// Stay beyond the original automatic close-contact detection radius.
	player->SetFeetPosition(origin + cVector3f(10, 0, 0));
	dog->mfMaxSeeDist = 0.01f;
	dog->mfCalcPlayerHiddenPosCount = 1.5f;
	dog->mfCheckForPlayerCount = dog->mfCheckForPlayerRate;
	dog->UpdateCheckForPlayer(dt);
	check(!dog->CanSeePlayer() && Near(dog->GetLastPlayerPos(), remembered),
		"lost sight preserves the last observation");
	check(Near(dog->GetPursuitPosition(), remembered), "hidden pursuit uses memory");
	dog->mfMaxSeeDist = seeDistance;
	dog->mpMover->TurnToPos(remembered);
	const float memoryAngle = dog->mpMover->mfGoalAngle;
	dog->ChangeState(STATE_ATTENTION);
	dog->mvStates[STATE_ATTENTION]->OnUpdate(dt);
	check(Near(dog->mpMover->mfGoalAngle, memoryAngle), "attention turns towards memory");
	dog->ChangeState(STATE_ATTACK);
	auto *bite = static_cast<cGameEnemyState_Dog_Attack*>(dog->mvStates[STATE_ATTACK]);
	player->SetFeetPosition(origin + cVector3f(-2, 0, 0));
	bite->OnUpdate(dt);
	check(Near(bite->mvTargetPosition, remembered) && Near(dog->mpMover->mfGoalAngle, memoryAngle),
		"unseen attack keeps its observed aim");
	check(Near(bite->mfDamageTimer, dog->mfAttackDamageTime - dt),
		"attack retains its authored countdown");
	dog->ChangeState(STATE_IDLE);
	body->SetMoveSpeed(eCharDir_Forward, 0);
	dog->mpMover->mbTurning = false;
	dog->mpMover->mfTurnSpeed = 0;

	// Reusing a moving clip as a scripted clip must keep its phase while
	// applying the new playback options. Idle must return to speed 1 too.
	dog->PlayAnim("Walk", true, 0.2f, true, 2, false, false);
	cAnimationState *walk = dog->GetCurrentAnimation();
	walk->SetRelativeTimePosition(0.4f);
	walk->SetSpeed(0);
	const float time = walk->GetTimePosition();
	dog->PlayAnim("Walk", false, 0.2f, false, 1.25f, false, true);
	check(Near(walk->GetTimePosition(), time), "same clip preserves phase");
	check(!walk->IsLooping() && dog->mbOverideMoveState && !dog->mbAnimationIsSpeedDependant &&
		Near(walk->GetSpeed(), 1.25f), "same clip applies playback options and unfreezes");
	dog->UseMoveStateAnimations();
	dog->PlayAnim("Walk", true, 0.2f, true, 2, false, false);
	dog->PlayAnim("Run", true, 0.2f, true, 2, true, false);
	check(Near(dog->GetCurrentAnimation()->GetRelativeTimePosition(),
		walk->GetRelativeTimePosition()), "walk/run phase synchronisation is preserved");

	// A vertical displacement must not select Walk/Run or drive their speed.
	dog->UseMoveStateAnimations();
	dog->mMoveState = eEnemyMoveState_Stopped;
	dog->PlayAnim("Idle", true, 0.2f, false, 1, false, false);
	body->SetPosition(body->GetPosition());
	const cVector3f start = body->GetPosition();
	const_cast<cVector3f&>(body->GetPosition()).y += 0.1f;
	dog->UpdateAnimations(dt);
	check(dog->mMoveState == eEnemyMoveState_Stopped, "vertical motion does not select a gait");
	dog->mMoveState = eEnemyMoveState_Walking;
	dog->PlayAnim("Walk", true, 0.2f, true, 1, false, true);
	dog->UpdateAnimations(dt);
	check(Near(walk->GetSpeed(), 0), "vertical motion does not accelerate a gait");
	body->SetPosition(start);

	// Stuck recovery must finish the old state's tick. Without the return,
	// Hunt can immediately enter Attack again after it just changed to Idle.
	dog->ChangeState(STATE_IDLE);
	dog->ChangeState(STATE_HUNT);
	auto *hunt = static_cast<cGameEnemyState_Dog_Hunt*>(dog->mvStates[STATE_HUNT]);
	hunt->mlStuckAtMaxCount = 5;
	hunt->mfUpdatePathCount = 1;
	hunt->mbFreePlayerPath = true;
	dog->mpMover->mfStuckCounter = 1.2f;
	hunt->OnUpdate(dt);
	check(dog->GetCurrentStateId() == STATE_IDLE, "stuck hunt does not keep attacking after exit");

	// Reaching the observation starts the bounded search, even when the
	// hidden player is within the old attack radius at a different position.
	dog->ChangeState(STATE_HUNT);
	body->SetFeetPosition(origin);
	dog->SetLastPlayerPos(body->GetFeetPosition());
	dog->mpMover->ResetStuckCounter();
	hunt->OnUpdate(dt);
	check(hunt->mbLostPlayer && dog->GetCurrentStateId() == STATE_HUNT &&
		Near(body->GetMoveSpeed(eCharDir_Forward), 0), "arrival searches without a blind lunge");
	hunt->OnUpdate(dt);
	check(tString(dog->GetCurrentAnimation()->GetName()) == "Idle", "search begins with the original idle pose");
	hunt->OnUpdate(dog->mfHuntForLostPlayerTime * 0.25f);
	check(hunt->mlSearchPhase == 1 && !dog->mbOverideMoveState &&
		Near(body->GetMoveSpeed(eCharDir_Forward), 0), "search turns without advancing");
	hunt->OnUpdate(hunt->mfLostPlayerCount);
	check(dog->GetCurrentStateId() == STATE_IDLE, "search expires within the original duration");

	dog->ChangeState(STATE_HUNT);
	hunt->BeginSearch();
	const cVector3f sound = origin + cVector3f(1.3f, 0, 0.7f);
	check(!hunt->OnHearNoise(sound, dog->mfHuntMinHearVolume * 0.5f) && hunt->mbLostPlayer,
		"quiet sound does not restart search");
	check(hunt->OnHearNoise(sound, dog->mfHuntMinHearVolume) && !hunt->mbLostPlayer &&
		Near(dog->GetLastPlayerPos(), sound), "fresh sound remembers its actual source");
	hunt->BeginSearch();
	hunt->OnSeePlayer(player->GetPosition(), dog->mfHuntMinSeeChance);
	check(!hunt->mbLostPlayer && !dog->mbOverideMoveState, "fresh sight resumes pursuit");

	// Exercise real physics clearance and graph edges with a movable heavy
	// obstacle. A* recovery must reconsider an edge compiled before it moved.
	cCharacterMove *mover = dog->mpMover;
	cAINodeContainer *nodes = mover->GetNodeContainer();
	iPhysicsWorld *physics = world->GetPhysicsWorld();
	body->SetFeetPosition(origin);
	const cVector3f route = origin + cVector3f(0, 0, 2);
	check(mover->FreeDirectPathToPos(route), "clear direct route is accepted");
	iPhysicsBody *obstacle = physics->CreateBody("dog_test_blocker",
		physics->CreateBoxShape(cVector3f(2, 2, 0.3f), nullptr));
	obstacle->SetMass(dog->mfMaxPushMass + 10);
	obstacle->SetMatrix(cMath::MatrixTranslate(origin + cVector3f(0, 0.7f, 1)));
	check(!mover->FreeDirectPathToPos(route), "heavy obstacle blocks direct pursuit");
	cAINode *first = nodes->GetNodeFromName("dog_test_0_0");
	cAINode *second = nodes->GetNodeFromName("dog_test_0_2");
	mover->mpAStarCallback->mbCheckDynamic = true;
	check(!mover->mpAStarCallback->CanAddNode(first, second), "recovery rejects the blocked graph edge");
	mover->mpAStarCallback->mbCheckDynamic = false;
	obstacle->SetMass(1);
	check(mover->FreeDirectPathToPos(route), "small pushable prop remains traversable");
	obstacle->SetMass(0);
	check(!mover->FreeDirectPathToPos(route), "static obstacle blocks direct pursuit");
	obstacle->SetCollideCharacter(false);
	check(mover->FreeDirectPathToPos(route), "nonblocking geometry does not invalidate a route");
	obstacle->SetCollideCharacter(true);
	physics->DestroyBody(obstacle);
	check(!mover->MoveToPos(origin + cVector3f(0, 10, 0), true) && !mover->IsMoving() &&
		!mover->mpAStarCallback->mbCheckDynamic, "failed recovery stops and restores callback mode");
	// Visible but unreachable must hold and retry, without blind movement.
	hunt->mbFoundNoPath = true;
	hunt->mbFreePlayerPath = false;
	hunt->mfUpdatePathCount = 1;
	dog->mbCanSeePlayer = true;
	hunt->OnUpdate(dt);
	check(!hunt->mbLostPlayer && !mover->IsMoving() && Near(body->GetMoveSpeed(eCharDir_Forward), 0),
		"visible unreachable target waits for a route retry");
	dog->mbCanSeePlayer = false;

	// Look ahead around an open corner; the same steering query must refuse
	// that shortcut when a wall occupies its clearance rays.
	dog->ChangeState(STATE_IDLE);
	first = nodes->GetNodeFromName("dog_test_0_0");
	second = nodes->GetNodeFromName("dog_test_1_0");
	auto cornerTick = [&] {
		body->SetFeetPosition(origin + cVector3f(0, 0.05f, 0.5f));
		body->SetYaw(0);
		body->SetMoveSpeed(eCharDir_Forward, 0);
		mover->mlstNodes.clear();
		mover->mlstNodes.push_back(second);
		mover->mlstNodes.push_back(first);
		mover->mbMoving = true;
		mover->Update(dt);
		return mover->mfGoalAngle;
	};
	const float openCornerAngle = cornerTick();
	check(std::abs(openCornerAngle) > 0.2f, "open corner steers into the following segment");
	obstacle = physics->CreateBody("dog_test_corner_wall",
		physics->CreateBoxShape(cVector3f(0.2f, 2, 2), nullptr));
	obstacle->SetMatrix(cMath::MatrixTranslate(origin + cVector3f(0.3f, 0.7f, 0)));
	check(Near(cornerTick(), 0), "blocked corner keeps the original node target");
	physics->DestroyBody(obstacle);
	mover->Stop();

	body->SetMoveSpeed(eCharDir_Forward, 0);
	body->SetYaw(0);
	mover->SetAngleDistTurnMul(1000);
	mover->SetMaxTurnSpeed(10000);
	mover->TurnToAngle(0.2f);
	mover->Update(dt);
	check(Near(body->GetYaw(), 0.2f), "turning cannot overshoot in one tick");
	dog->SetupBody();
	mover->mbTurning = false;
	body->SetPosition(body->GetPosition());
	body->SetMoveSpeed(eCharDir_Forward, 0);
	mover->ResetStuckCounter();
	mover->Update(dt);
	check(Near(mover->GetStuckCounter(), 0), "a stopped dog is not stuck");
	body->SetMoveSpeed(eCharDir_Forward, -1);
	const_cast<cVector3f&>(body->GetPosition()) += body->GetForward() * -dt;
	mover->Update(dt);
	check(Near(mover->GetStuckCounter(), 0), "successful backward movement is not stuck");
	body->SetMoveSpeed(eCharDir_Forward, 0);

	// No rear-wall hit must not cause a correction towards the world origin.
	body->SetPosition(cVector3f(0, 0.4f, 0));
	body->SetYaw(0);
	const cVector3f noWallPosition = body->GetPosition();
	check(!dog->GetGroundFinder()->GetGround(noWallPosition, body->GetForward() * -1,
		nullptr, nullptr, dog->mfLengthBodyToAss), "rear-clearance fixture has no wall");
	dog->OnUpdate(dt);
	dog->OnUpdate(dt);
	check(Near(body->GetPosition(), noWallPosition), "rear correction requires a wall hit");
	body->SetFeetPosition(origin);

	// Door impacts use the entity's configured delay once, and fire once.
	dog->ChangeState(STATE_BREAKDOOR);
	auto *door = static_cast<cGameEnemyState_Dog_BreakDoor*>(dog->mvStates[STATE_BREAKDOOR]);
	const float delay = dog->mfBreakDoorDamageTime;
	door->OnUpdate(delay * 0.25f);
	check(Near(door->mfDamageTimer, delay * 0.75f) && !door->mbAttacked,
		"door impact countdown advances once per tick");
	door->OnUpdate(delay * 0.75f);
	check(door->mbAttacked, "door impact fires at the authored time");
	door->OnUpdate(dt);
	check(door->mbAttacked, "door impact remains consumed");

	// A completion handler may keep the final pose. It gets one notification
	// per playback, including another notification when the clip is replayed.
	auto *completion = hplNew(CompletionState, (init, dog));
	dog->AddState(completion);
	dog->ChangeState(completion->GetId());
	dog->PlayAnim("AttackLow", false, 0.2f);
	cAnimationState *attack = dog->GetCurrentAnimation();
	attack->SetTimePosition(attack->GetLength());
	dog->UpdateAnimations(dt);
	dog->UpdateAnimations(dt);
	check(completion->completions == 1, "animation completion is delivered once");
	dog->PlayAnim("AttackLow", false, 0.2f);
	attack->SetTimePosition(attack->GetLength());
	dog->UpdateAnimations(dt);
	check(completion->completions == 2, "replaying a clip enables its next completion");

	// Runtime fixture nodes are not part of the saved map. Remove the test
	// dog before the ordinary smoke save/reload (original levels separately
	// cover enemy save/reload with their authored navigation graphs).
	dog->ChangeState(STATE_IDLE);
	init->mpMapHandler->RemoveGameEntity(dog);
	player->SetFeetPosition(playerStart);
	init->SetImprovedDogAI(previousMode);
	Log("DOGTEST: done, %d checks, %d failure(s)\n", checks, failures);
	return failures;
}

#include "core/DogTestMap.h"

// Validate the authored arena and both selectable decision policies using
// original entities, real physics rays and the compiled navigation graph.
int RunDogArenaRegressions(cInit *init)
{
    int checks=0, failures=0;
    auto check=[&](bool ok,const char *label) {
        ++checks; if(!ok) ++failures;
        Log("DOGARENA: %s %s\n",ok?"PASS":"FAIL",label);
    };
    const bool previousMode=init->GetImprovedDogAI();
    auto *arena=init->mpDogTestMap;
    arena->Update(0);
    check(arena->GetStationCount()==6,"six stations load");
    for(bool improved:{false,true}) {
        init->SetImprovedDogAI(improved);
        for(size_t i=0;i<arena->GetStationCount();++i) {
            arena->ResetStation(i);
            auto *dog=arena->GetDog();
            check(dog && dog->GetHealth()>0,"station resets a living original Dog");
            if(!dog) continue;
            auto *mover=dog->GetMover();
            check(mover->GetNodeContainer() && mover->GetNodeContainer()->GetNodeNum()>500,
                "arena has an authored navigation graph");
            check(mover->mbSmoothNavigation==improved && dog->TrackPlayerAfterSightLoss()!=improved,
                "selected AI controls navigation and sight memory");
            check(Near(dog->GetHuntMinSeeChance(),improved?dog->mfHuntMinSeeChance:dog->mfIdleMinSeeChance),
                "selected AI uses its pursuit threshold");
            dog->mbCanSeePlayer=false;
            const cVector3f target=init->mpPlayer->GetCharacterBody()->GetFeetPosition();
            const cVector3f memory=target+cVector3f(3,0,2);
            dog->SetLastPlayerPos(memory);
            check(Near(dog->GetPursuitPosition(),improved?memory:target),"selected AI chooses memory or legacy live target");
            dog->SetUsesTriggers(false);
            if(i==0) {
                check(dog->GetPatrolNodeNum()==4 && dog->GetCurrentStateId()==STATE_PATROL,
                    "patrol starts on four original-clip waypoints");
                auto *node=mover->GetNodeContainer()->GetNodeFromName("patrol0");
                check(node && mover->MoveToPos(node->GetPosition()),"patrol route is reachable");
            }
            if(i==1) {
                const cVector3f origin(0,.05f,-26);
                mover->GetCharBody()->SetFeetPosition(origin);
                check(!mover->FreeDirectPathToPos(cVector3f(0,.05f,-11)),"hiding cover blocks direct pursuit");
                check(mover->MoveToPos(cVector3f(0,.05f,-11)),"navigation routes around hiding cover");
            }
            if(i==2) {
                arena->PlayLure(0); arena->PlayLure(1);
                auto *world=init->mpGame->GetScene()->GetWorld3D();
                cSoundEntity *a=world->GetSoundEntity("dogtestlureA");
                cSoundEntity *b=world->GetSoundEntity("dogtestlureB");
                check(a && b && Near(a->GetWorldPosition(),cVector3f(17,1,-17)) &&
                    Near(b->GetWorldPosition(),cVector3f(27,1,-23)),"sound lures play at authored locations");
            }
            if(i==3) {
                iGameEntity *heavy=init->mpMapHandler->GetGameEntity("heavycrate",false);
                iGameEntity *small=init->mpMapHandler->GetGameEntity("smallcrate",false);
                check(heavy && small && heavy->GetBodyNum()>0 && small->GetBodyNum()>0 &&
                    heavy->GetBody(0)->GetMass()>dog->mfMaxPushMass && small->GetBody(0)->GetMass()<dog->mfMaxPushMass,
                    "original heavy and pushable crates load");
                check(!mover->FreeDirectPathToPos(target),"heavy crate blocks direct pursuit");
                check(mover->MoveToPos(target,true),"dynamic route finds the crate detour");
            }
            if(i==4) {
                iGameEntity *door=init->mpMapHandler->GetGameEntity("testdoor",false);
                check(door && door->GetType()==eGameEntityType_SwingDoor,"original breakable door loads");
                check(!mover->FreeDirectPathToPos(target),"door blocks direct pursuit");
                check(door && door->GetHealth()>0 && door->GetToughness()<=dog->mlBreakDoorStrength,
                    "door fixture has authored breakable health and toughness");
                auto *world=init->mpGame->GetScene()->GetWorld3D();
                auto *player=init->mpPlayer->GetCharacterBody();
                auto *body=mover->GetCharBody();
                body->SetFeetPosition(cVector3f(0,.05f,9.25f));
                body->SetYaw(cMath::ToRad(180));
                player->SetFeetPosition(cVector3f(0,.05f,10.65f));
                bool doorwayCovered=true;
                for(float x:{-.35f,0.0f,.35f}) {
                    body->SetFeetPosition(cVector3f(x,.05f,9.25f));
                    player->SetFeetPosition(cVector3f(x,.05f,10.65f));
                    doorwayCovered &= !init->mpAttackHandler->HasFreePathToPlayer(body->GetPosition());
                }
                check(doorwayCovered,"closed door spans the doorway rather than standing edge-on");
                body->SetFeetPosition(cVector3f(0,.05f,9.25f));
                player->SetFeetPosition(cVector3f(0,.05f,10.65f));
                init->mpPlayer->SetGodMode(false);
                init->mpPlayer->SetHealth(100);
                player->SetForce(0);
                auto strike=[&](eAttackTargetFlag flags,float damage,int strength) {
                    cMatrixf offset=cMath::MatrixRotate(cVector3f(0,body->GetYaw(),0),eEulerRotationOrder_XYZ);
                    offset.SetTranslation(body->GetPosition()+body->GetForward()*dog->mfAttackDamageRange);
                    return init->mpAttackHandler->CreateShapeAttack(dog->GetAttackShape(),offset,
                        body->GetPosition(),damage,3,28,2,9,strength,flags,nullptr);
                };
                // A previous line attack can have skipped the held door panel.
                // Shape attacks must supply their own skip body, not reuse it.
                iPhysicsBody *panel=nullptr;
                for(int j=0;door && j<door->GetBodyNum();++j)
                    if(door->GetBody(j)->GetMass()>0) panel=door->GetBody(j);
                init->mpAttackHandler->CreateLineAttack(body->GetPosition(),player->GetPosition(),
                    0,eAttackTargetFlag_Player,panel);
                const bool blockedHit=strike(eAttackTargetFlag_Player,10,dog->mlAttackStrength);
                check(Near(init->mpPlayer->GetHealth(),100),"closed door blocks bite damage after a skipped-body line attack");
                check(Near(player->GetForce(),cVector3f(0)),"closed door blocks bite knockback");
                check(!blockedHit,"blocked bite is not reported as a player hit");
                if(panel) panel->SetActive(false);
                iPhysicsWorld *physics=world->GetPhysicsWorld();
                iPhysicsBody *wall=physics->CreateBody("dogtest_attack_wall",
                    physics->CreateBoxShape(cVector3f(2,2,.15f),nullptr));
                wall->SetMatrix(cMath::MatrixTranslate(cVector3f(0,1,10)));
                init->mpPlayer->SetHealth(100);
                player->SetForce(0);
                const bool wallHit=strike(eAttackTargetFlag_Player,10,dog->mlAttackStrength);
                check(!wallHit && Near(init->mpPlayer->GetHealth(),100) && Near(player->GetForce(),cVector3f(0)),
                    "solid wall blocks bite damage, knockback and hit feedback");
                physics->DestroyBody(wall);
                const bool clearHit=strike(eAttackTargetFlag_Player,10,dog->mlAttackStrength);
                check(clearHit && init->mpPlayer->GetHealth()<100 && player->GetForce().Length()>0,
                    "unobstructed bite still damages and pushes the player");
                if(panel) panel->SetActive(true);
                init->mpPlayer->SetHealth(100);
                player->SetForce(0);
                if(door && door->GetHealth()>0) {
                    door->SetHealth(1);
                    strike(eAttackTargetFlag_Player|eAttackTargetFlag_Bodies,25,dog->mlBreakDoorStrength);
                    check(door->GetBreakMe(),"breaking strike destroys the original door");
                    check(Near(init->mpPlayer->GetHealth(),100) && Near(player->GetForce(),cVector3f(0)),
                        "door protects player for the strike that breaks it");
                }
                // Run real physics, animation and AI ticks from the authored
                // starting positions: a route check alone cannot prove breaking.
                for(float playerZ:{17.0f,10.65f}) {
                    arena->ResetStation(i);
                    dog=arena->GetDog(); mover=dog->GetMover(); body=mover->GetCharBody();
                    player->SetFeetPosition(cVector3f(0,.05f,playerZ));
                    init->mpPlayer->SetGodMode(false);
                    player->SetForce(0);
                    dog->SetUsesTriggers(true);
                    dog->ShowPlayer(player->GetFeetPosition());
                    door=init->mpMapHandler->GetGameEntity("testdoor",false);
                    const float originalHealth=door?door->GetHealth():0;
                    bool enteredBreak=false, damagedDoor=false, brokenDoor=false;
                    const float dt=1.0f/60;
                    for(int tick=0;tick<1800;++tick) {
                        world->Update(dt);
                        init->mpMapHandler->Update(dt);
                        enteredBreak |= dog->GetCurrentStateId()==STATE_BREAKDOOR;
                        door=init->mpMapHandler->GetGameEntity("testdoor",false);
                        damagedDoor |= originalHealth>0 && (!door || door->GetHealth()<originalHealth);
                        brokenDoor |= !door || door->GetBreakMe();
                        if(tick%300==0) Log("DOGARENA: door pursuit %s playerZ %.2f tick %d state %d feet %s stuck %.2f health %.1f\n",
                            improved?"Improved":"Original",playerZ,tick,dog->GetCurrentStateId(),body->GetFeetPosition().ToString().c_str(),
                            mover->GetStuckCounter(),door?door->GetHealth():-1);
                        if(brokenDoor) break;
                    }
                    check(enteredBreak,"natural pursuit reaches the door-breaking state");
                    check(damagedDoor,"natural door strikes reduce authored health");
                    check(brokenDoor && world->GetMeshEntity("testdoor_broken"),"natural pursuit breaks the door and creates original debris");
                    check(Near(init->mpPlayer->GetHealth(),100),"player behind the door stays unharmed while it is broken");
                }
                init->mpPlayer->SetGodMode(true);
            }
            if(i==5) {
                dog->ChangeState(STATE_KNOCKDOWN);
                check(!mover->GetCharBody()->IsActive() && dog->GetMeshEntity()->GetSkeletonPhysicsActive(),
                    "knockdown enters original ragdoll state");
                arena->ResetStation(i);
                check(arena->GetDog()->GetMover()->GetCharBody()->IsActive(),"reset restores a knocked-down Dog");
            }
        }
    }
    // Mode changes also reach already spawned enemies without interrupting
    // attacks or requiring the game/map to restart.
    auto *dog=arena->GetDog();
    init->SetImprovedDogAI(false); dog->OnUpdate(1.0f/60);
    check(!dog->GetMover()->mbSmoothNavigation,"existing Dog switches to Original navigation");
    init->SetImprovedDogAI(true); dog->OnUpdate(1.0f/60);
    check(dog->GetMover()->mbSmoothNavigation,"existing Dog switches to Improved navigation");
    init->SetImprovedDogAI(previousMode);
    arena->ResetStation(0);
    Log("DOGARENA: done, %d checks, %d failure(s)\n",checks,failures);
    return failures;
}
