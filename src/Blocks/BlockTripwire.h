
#pragma once

#include "BlockHandler.h"





class cBlockTripwireHandler :
	public cBlockHandler
{
public:
	cBlockTripwireHandler(BLOCKTYPE a_BlockType)
		: cBlockHandler(a_BlockType)
	{
	}

	virtual void ConvertToPickups(cItems & a_Pickups, NIBBLETYPE a_BlockMeta) override
	{
		a_Pickups.push_back(cItem(E_ITEM_STRING, 1, 0));
	}

	virtual const char * GetStepSound(void) override
	{
		return "";
	}
};




