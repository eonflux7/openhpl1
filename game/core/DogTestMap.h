/* Overture Remade, GNU GPL v3 or later. See COPYING. */
#ifndef GAME_DOG_TEST_MAP_H
#define GAME_DOG_TEST_MAP_H
#include "core/StdAfx.h"
using namespace hpl;
class cInit;
class cGameEnemy_Dog;

// Fixtures and diagnostics for dogtest only. Drawing queues actions; Update
// performs them after the normal entity/physics update has finished.
class cDogTestMap : public iUpdateable
{
public:
    explicit cDogTestMap(cInit *apInit);
    void Update(float afTimeStep) override;
    void Reset() override;
    bool IsActive() const;
    bool DrawControls(); // true = also show the usual F2 renderer tools
    void ResetStation(size_t alStation, bool abMovePlayer = true);
    void PlayLure(size_t alLure);
    size_t GetStationCount() const { return mvStations.size(); }
    cGameEnemy_Dog *GetDog() const;
private:
    struct cProp { tString msName, msFile; cVector3f mvPosition, mvRotation; bool mbLocked; };
    struct cStation {
        tString msName, msLabel, msInstructions;
        tStringVec mvPatrol;
        std::vector<cProp> mvProps;
        std::vector<cVector3f> mvLures;
    };
    bool LoadStations();
    cInit *mpInit;
    std::vector<cStation> mvStations;
    size_t mlStation = 0;
    int mlQueuedStation = -1, mlQueuedLure = -1;
    bool mbEntered = false, mbNeedsSetup = true, mbPreviousGodMode = false;
    bool mbGodMode = true, mbPerception = true, mbAdvanced = false;
    bool mbReveal = false, mbKnockDown = false;
};
#endif
