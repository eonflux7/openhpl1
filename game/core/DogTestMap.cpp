/* OpenHPL1, GNU GPL v3 or later. See COPYING. */
#include "core/DogTestMap.h"
#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/RefView.h"
#include "player/Player.h"
#include "enemies/GameEnemy_Dog.h"
#include "entities/GameSwingDoor.h"
#include <imgui.h>
#include <SDL3/SDL.h>

cDogTestMap::cDogTestMap(cInit *init) : iUpdateable("DogTestMap"), mpInit(init) {}

bool cDogTestMap::IsActive() const
{
    return mpInit->mpMapHandler->GetCurrentMapName() == "dogtest" &&
        !mpInit->mpMapHandler->IsChangingMap();
}

cGameEnemy_Dog *cDogTestMap::GetDog() const
{
    if(!IsActive()) return nullptr;
    iGameEntity *entity = mpInit->mpMapHandler->GetGameEntity("dogtestdog",false);
    return entity && entity->GetSubType() == "Dog" ? static_cast<cGameEnemy_Dog*>(entity) : nullptr;
}

bool cDogTestMap::LoadStations()
{
    TiXmlDocument doc;
    const tString file = tString(SDL_GetBasePath()) + "testmap/dogtest.cfg";
    if(!doc.LoadFile(file.c_str()) || !doc.RootElement()) {
        Error("DOGARENA: cannot load %s\n",file.c_str());
        return false;
    }
    mvStations.clear();
    for(TiXmlElement *e=doc.RootElement()->FirstChildElement("Station");e;e=e->NextSiblingElement("Station")) {
        cStation station;
        station.msName=cString::ToString(e->Attribute("Name"),"");
        station.msLabel=cString::ToString(e->Attribute("Label"),station.msName);
        station.msInstructions=cString::ToString(e->Attribute("Instructions"),"");
        if(station.msName.empty()) continue;
        for(TiXmlElement *p=e->FirstChildElement("Patrol");p;p=p->NextSiblingElement("Patrol"))
            station.mvPatrol.push_back(cString::ToString(p->Attribute("Node"),""));
        for(TiXmlElement *p=e->FirstChildElement("Prop");p;p=p->NextSiblingElement("Prop")) {
            station.mvProps.push_back({cString::ToString(p->Attribute("Name"),""),
                cString::ToString(p->Attribute("File"),""),
                cString::ToVector3f(p->Attribute("Position"),0),
                cMath::Vector3ToRad(cString::ToVector3f(p->Attribute("Rotation"),0)),
                cString::ToBool(p->Attribute("Locked"),false)});
        }
        for(TiXmlElement *p=e->FirstChildElement("Lure");p;p=p->NextSiblingElement("Lure"))
            station.mvLures.push_back(cString::ToVector3f(p->Attribute("Position"),0));
        mvStations.push_back(station);
    }
    return !mvStations.empty();
}

void cDogTestMap::Reset() { mbNeedsSetup=true; }

void cDogTestMap::Update(float)
{
    if(!IsActive()) {
        if(mbEntered && !mpInit->mpMapHandler->IsChangingMap()) {
            mpInit->mpPlayer->SetGodMode(mbPreviousGodMode);
            mbEntered=false;
            mbNeedsSetup=true;
            mlQueuedStation=mlQueuedLure=-1;
            mbReveal=mbKnockDown=false;
        }
        return;
    }
    if(!mbEntered) {
        mbPreviousGodMode=mpInit->mpPlayer->GetGodMode();
        mbEntered=true;
        mbNeedsSetup=true;
    }
    if(mbNeedsSetup) {
        if(!LoadStations()) return;
        // Honour OVERTURE_MAP=dogtest:<start>, including after a save load.
        cWorld3D *world=mpInit->mpGame->GetScene()->GetWorld3D();
        const cVector3f player=mpInit->mpPlayer->GetCharacterBody()->GetFeetPosition();
        float best=1e30f;
        for(size_t i=0;i<mvStations.size();++i) {
            cStartPosEntity *start=world->GetStartPosEntity(mvStations[i].msName);
            if(start) {
                float distance=cMath::Vector3DistSqr(player,start->GetWorldMatrix().GetTranslation());
                if(distance<best) { best=distance; mlStation=i; }
            }
        }
        ResetStation(mlStation,false);
        mbNeedsSetup=false;
        if(mpInit->mpRefView) mpInit->mpRefView->SetOpen(true);
    }
    if(mlQueuedStation>=0) {
        ResetStation(static_cast<size_t>(mlQueuedStation));
        mlQueuedStation=-1;
    }
    mpInit->mpPlayer->SetGodMode(mbGodMode);
    if(cGameEnemy_Dog *dog=GetDog()) {
        dog->SetUsesTriggers(mbPerception);
        if(mbReveal) dog->ShowPlayer(mpInit->mpPlayer->GetCharacterBody()->GetFeetPosition());
        if(mbKnockDown && dog->GetHealth()>0 && dog->GetCurrentStateId()!=STATE_KNOCKDOWN)
            dog->ChangeState(STATE_KNOCKDOWN);
    }
    mbReveal=mbKnockDown=false;
    if(mlQueuedLure>=0) { PlayLure(static_cast<size_t>(mlQueuedLure)); mlQueuedLure=-1; }
}

void cDogTestMap::ResetStation(size_t alStation, bool abMovePlayer)
{
    if(!IsActive() || alStation>=mvStations.size()) return;
    cWorld3D *world=mpInit->mpGame->GetScene()->GetWorld3D();
    cStartPosEntity *spawn=world->GetStartPosEntity(mvStations[alStation].msName+"dog");
    if(!spawn) { Error("DOGARENA: missing Dog start\n"); return; }
    if(mpInit->mpRefView && abMovePlayer) mpInit->mpRefView->ReleaseForDogTest();
    std::vector<cSoundEntity*> sounds;
    auto soundIt=world->GetSoundEntityIterator();
    while(soundIt.HasNext()) {
        cSoundEntity *sound=soundIt.Next();
        if(sound->GetName()=="dogtestlureA" || sound->GetName()=="dogtestlureB") sounds.push_back(sound);
    }
    for(cSoundEntity *sound:sounds) world->DestroySoundEntity(sound);
    // Remove our fixtures (including broken-door debris) before recreating.
    std::vector<iGameEntity*> remove;
    auto it=mpInit->mpMapHandler->GetGameEntityIterator();
    while(it.HasNext()) {
        iGameEntity *entity=it.Next();
        const tString &name=entity->GetName();
        if(name=="dogtestdog" || name=="heavycrate" || name=="smallcrate" || name.find("testdoor")==0)
            remove.push_back(entity);
    }
    for(iGameEntity *entity:remove) {
        if(entity->GetSubType()=="Dog") static_cast<cGameEnemy_Dog*>(entity)->OnWorldExit();
        mpInit->mpMapHandler->RemoveGameEntity(entity);
    }
    mlStation=alStation;
    const cStation &station=mvStations[alStation];
    for(const cProp &prop:station.mvProps) {
        cMatrixf transform=cMath::MatrixRotate(prop.mvRotation,eEulerRotationOrder_XYZ);
        transform.SetTranslation(prop.mvPosition);
        world->CreateEntity(prop.msName,transform,prop.msFile,true);
        iGameEntity *entity=mpInit->mpMapHandler->GetGameEntity(prop.msName,false);
        if(entity) {
            entity->OnWorldLoad();
            if(prop.mbLocked && entity->GetType()==eGameEntityType_SwingDoor)
                static_cast<cGameSwingDoor*>(entity)->SetLocked(true);
        }
    }
    world->CreateEntity("dogtestdog",spawn->GetWorldMatrix(),"character_dog.ent",true);
    if(cGameEnemy_Dog *dog=GetDog()) {
        dog->OnWorldLoad();
        dog->SetUsesTriggers(mbPerception);
        for(const tString &patrol:station.mvPatrol) dog->AddPatrolNode(patrol,1,"Idle");
        if(!station.mvPatrol.empty()) dog->ChangeState(STATE_PATROL);
    }
    if(abMovePlayer) {
        mpInit->mpPlayer->SetHealth(100);
        mpInit->mpPlayer->GetCharacterBody()->SetForceVelocity(0);
        mpInit->mpPlayer->GetCharacterBody()->SetMoveSpeed(eCharDir_Forward,0);
        mpInit->mpPlayer->GetCharacterBody()->SetMoveSpeed(eCharDir_Right,0);
        mpInit->mpPlayer->SetStartPos(station.msName);
    }
    Log("DOGARENA: reset %s, %s AI\n",station.msName.c_str(),mpInit->GetImprovedDogAI()?"Improved":"Original");
}

void cDogTestMap::PlayLure(size_t alLure)
{
    if(!IsActive() || mlStation>=mvStations.size() || alLure>=mvStations[mlStation].mvLures.size()) return;
    cSoundEntity *sound=mpInit->mpGame->GetScene()->GetWorld3D()->CreateSoundEntity(
        alLure==0?"dogtestlureA":"dogtestlureB","unbreak_impact_wood_high.snt",true);
    if(sound) sound->SetPosition(mvStations[mlStation].mvLures[alLure]);
}

bool cDogTestMap::DrawControls()
{
    if(!IsActive()) return true;
    ImGui::SetNextWindowPos(ImVec2(18,18),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(470,0),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Dog behaviour tests (F2)",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Original Dog model and animation clips");
        bool improved=mpInit->GetImprovedDogAI();
        if(ImGui::RadioButton("Original AI",!improved)) mpInit->SetImprovedDogAI(false);
        ImGui::SameLine();
        if(ImGui::RadioButton("Improved AI",improved)) mpInit->SetImprovedDogAI(true);
        ImGui::TextWrapped("AI mode is saved with settings. Use Reset and play to compare the same starting conditions.");
        ImGui::Separator();
        for(size_t i=0;i<mvStations.size();++i) {
            if(ImGui::Selectable(mvStations[i].msLabel.c_str(),i==mlStation))
                mlQueuedStation=static_cast<int>(i);
        }
        if(mlStation<mvStations.size()) {
            ImGui::Separator();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+440);
            ImGui::TextUnformatted(mvStations[mlStation].msInstructions.c_str());
            ImGui::PopTextWrapPos();
            if(ImGui::Button("Reset and play")) mlQueuedStation=static_cast<int>(mlStation);
            ImGui::SameLine();
            if(ImGui::Button("Resume")) mpInit->mpRefView->SetOpen(false);
            if(ImGui::Button("Reveal player position once")) {
                mbReveal=true; mpInit->mpRefView->SetOpen(false);
            }
            if(!mvStations[mlStation].mvLures.empty()) {
                if(ImGui::Button("Play lure A")) { mlQueuedLure=0; mpInit->mpRefView->SetOpen(false); }
                ImGui::SameLine();
                if(ImGui::Button("Play lure B")) { mlQueuedLure=1; mpInit->mpRefView->SetOpen(false); }
            }
            if(mvStations[mlStation].msName=="attack" && ImGui::Button("Knock down")) {
                mbKnockDown=true; mpInit->mpRefView->SetOpen(false);
            }
        }
        ImGui::Checkbox("Player cannot be hurt",&mbGodMode);
        ImGui::Checkbox("Sight and hearing enabled",&mbPerception);
        if(cGameEnemy_Dog *dog=GetDog()) {
            static const char *names[]={"Idle","Hunt","Attack","Flee","Knockdown","Dead","Patrol","Investigate","Break door","Call backup","Move to","Eat","Attention"};
            const int state=dog->GetCurrentStateId();
            ImGui::Separator();
            ImGui::Text("State: %s | Health: %.0f | Sees player: %s",state>=0 && state<STATE_NUM?names[state]:"Other",dog->GetHealth(),dog->CanSeePlayer()?"yes":"no");
            cAnimationState *anim=dog->GetCurrentAnimation();
            ImGui::Text("Clip: %s | Stuck: %.2f",anim?anim->GetName():"none",dog->GetMover()->GetStuckCounter());
            const cVector3f known=dog->GetLastPlayerPos();
            ImGui::Text("Last observation: %.1f, %.1f, %.1f",known.x,known.y,known.z);
        }
        ImGui::Separator();
        ImGui::Checkbox("Show renderer and camera tools",&mbAdvanced);
    }
    ImGui::End();
    return mbAdvanced;
}
