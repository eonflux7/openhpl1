// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_AI_H
#define HPL_AI_H

#include "system/SystemTypes.h"
#include "game/GameTypes.h"

#include "game/Updateable.h"

namespace hpl {

	class cAINodeGenerator;

	class cAI : public iUpdateable
	{
	public:
		cAI();
		~cAI() override;

		void Reset() override;
		void Update(float afTimeStep) override;

		void Init();

		cAINodeGenerator *GetNodeGenerator(){ return mpAINodeGenerator;}

	private:
		cAINodeGenerator *mpAINodeGenerator;
	};

};
#endif // HPL_AI_H
