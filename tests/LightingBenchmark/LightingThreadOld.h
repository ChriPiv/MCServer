
// LightingThread.h

// Interfaces to the cLightingThreadOld class representing the thread that processes requests for lighting

/*
Lighting is done on whole chunks. For each chunk to be lighted, the whole 3x3 chunk area around it is read,
then it is processed, so that the middle chunk area has valid lighting, and the lighting is copied into the ChunkMap.
Lighting is calculated in full char arrays instead of nibbles, so that accessing the arrays is fast.
Lighting is calculated in a flood-fill fashion:
1. Generate seeds from where the light spreads (full skylight / light-emitting blocks)
2. For each seed:
	- Spread the light 1 block in each of the 6 cardinal directions, if the blocktype allows
	- If the recipient block has had lower lighting value than that being spread, make it a new seed
3. Repeat step 2, until there are no more seeds
The seeds need two fast operations:
	- Check if a block at [x, y, z] is already a seed
	- Get the next seed in the row
For that reason it is stored in two arrays, one stores a bool saying a seed is in that position, 
the other is an array of seed coords, encoded as a single int.
Step 2 needs two separate storages for old seeds and new seeds, so there are two actual storages for that purpose,
their content is swapped after each full step-2-cycle.

The thread has two queues of chunks that are to be lighted.
The first queue, m_Queue, is the only one that is publicly visible, chunks get queued there by external requests.
The second one, m_PostponedQueue, is for chunks that have been taken out of m_Queue and didn't have neighbors ready.
Chunks from m_PostponedQueue are moved back into m_Queue when their neighbors get valid, using the ChunkReady callback.
*/



#pragma once

#include "../../src/OSSupport/IsThread.h"
#include "../../src/ChunkDef.h"
#include "../../src/ChunkStay.h"









// fwd: "cWorld.h"
class cWorld;





class cLightingThreadOld
{
	
public:
	
	cLightingThreadOld(void);
	~cLightingThreadOld();
	
	/** Lights the entire chunk. If neighbor chunks don't exist, touches them and re-queues the chunk */
	void LightChunk(int X, int Z);
	
	
	cWorld * m_World;
	
protected:
	
	typedef std::list<cChunkStay *> cChunkStays;
	
	
	
	
	// Buffers for the 3x3 chunk data
	// These buffers alone are 1.7 MiB in size, therefore they cannot be located on the stack safely - some architectures may have only 1 MiB for stack, or even less
	// Placing the buffers into the object means that this object can light chunks only in one thread!
	// The blobs are XZY organized as a whole, instead of 3x3 XZY-organized subarrays ->
	//  -> This means data has to be scatterred when reading and gathered when writing!
	static const int BlocksPerYLayer = cChunkDef::Width * cChunkDef::Width * 3 * 3;
	BLOCKTYPE  m_BlockTypes[BlocksPerYLayer * cChunkDef::Height];
	NIBBLETYPE m_BlockLight[BlocksPerYLayer * cChunkDef::Height];
	NIBBLETYPE m_SkyLight  [BlocksPerYLayer * cChunkDef::Height];
	HEIGHTTYPE m_HeightMap [BlocksPerYLayer];
	
	// Seed management (5.7 MiB)
	// Two buffers, in each calc step one is set as input and the other as output, then in the next step they're swapped
	// Each seed is represented twice in this structure - both as a "list" and as a "position".
	// "list" allows fast traversal from seed to seed
	// "position" allows fast checking if a coord is already a seed
	unsigned char m_IsSeed1 [BlocksPerYLayer * cChunkDef::Height];
	unsigned int  m_SeedIdx1[BlocksPerYLayer * cChunkDef::Height];
	unsigned char m_IsSeed2 [BlocksPerYLayer * cChunkDef::Height];
	unsigned int  m_SeedIdx2[BlocksPerYLayer * cChunkDef::Height];
	int m_NumSeeds;


	
	/** Prepares m_BlockTypes and m_HeightMap data; returns false if any of the chunks fail. Zeroes out the light arrays */
	bool ReadChunks(int a_ChunkX, int a_ChunkZ);
	
	/** Uses m_HeightMap to initialize the m_SkyLight[] data; fills in seeds for the skylight */
	void PrepareSkyLight(void);
	
	/** Uses m_BlockTypes to initialize the m_BlockLight[] data; fills in seeds for the blocklight */
	void PrepareBlockLight(void);
	
	/** Calculates light in the light array specified, using stored seeds */
	void CalcLight(NIBBLETYPE * a_Light);
	
	/** Does one step in the light calculation - one seed propagation and seed recalculation */
	void CalcLightStep(
		NIBBLETYPE * a_Light, 
		int a_NumSeedsIn,    unsigned char * a_IsSeedIn,  unsigned int * a_SeedIdxIn,
		int & a_NumSeedsOut, unsigned char * a_IsSeedOut, unsigned int * a_SeedIdxOut
	);
	
	/** Compresses from 1-block-per-byte (faster calc) into 2-blocks-per-byte (MC storage): */
	void CompressLight(NIBBLETYPE * a_LightArray, NIBBLETYPE * a_ChunkLight);
	
	inline void PropagateLight(
		NIBBLETYPE * a_Light, 
		unsigned int a_SrcIdx, unsigned int a_DstIdx,
		int & a_NumSeedsOut, unsigned char * a_IsSeedOut, unsigned int * a_SeedIdxOut
	)
	{
		ASSERT(a_SrcIdx < ARRAYCOUNT(m_SkyLight));
		ASSERT(a_DstIdx < ARRAYCOUNT(m_BlockTypes));
		
		if (a_Light[a_SrcIdx] <= a_Light[a_DstIdx] + cBlockInfo::GetSpreadLightFalloff(m_BlockTypes[a_DstIdx]))
		{
			// We're not offering more light than the dest block already has
			return;
		}

		a_Light[a_DstIdx] = a_Light[a_SrcIdx] - cBlockInfo::GetSpreadLightFalloff(m_BlockTypes[a_DstIdx]);
		if (!a_IsSeedOut[a_DstIdx])
		{
			a_IsSeedOut[a_DstIdx] = true;
			a_SeedIdxOut[a_NumSeedsOut++] = a_DstIdx;
		}
	}
} ;




