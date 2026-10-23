#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherBridgePieces_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherBridgePieces_H__

#include <list>
#include <vector>
#include "StructurePiece.h"

// The pieces of a Nether fortress: open bridges with crossings, and enclosed castle
// corridors with a blaze spawner throne, a nether wart garden and chests. Ported
// from the Console Edition's NetherBridgePieces; the chests in the corridor turns
// are the Java Edition's (1.7) addition.
namespace NetherBridgePieces
{
	enum EPieceClass
	{
		EPieceClass_BridgeStraight,
		EPieceClass_BridgeEndFiller,
		EPieceClass_BridgeCrossing,
		EPieceClass_RoomCrossing,
		EPieceClass_StairsRoom,
		EPieceClass_MonsterThrone,
		EPieceClass_CastleEntrance,
		EPieceClass_CastleStalkRoom,
		EPieceClass_CastleSmallCorridorPiece,
		EPieceClass_CastleSmallCorridorCrossingPiece,
		EPieceClass_CastleSmallCorridorRightTurnPiece,
		EPieceClass_CastleSmallCorridorLeftTurnPiece,
		EPieceClass_CastleCorridorStairsPiece,
		EPieceClass_CastleCorridorTBalconyPiece
	};

	struct PieceWeight
	{
		EPieceClass pieceClass;
		int weight;
		int placeCount;
		int maxPlaceCount;
		bool allowInRow;

		PieceWeight() : pieceClass(EPieceClass_BridgeStraight), weight(0), placeCount(0), maxPlaceCount(0), allowInRow(false) {}
		PieceWeight(EPieceClass pieceClass, int weight, int maxPlaceCount, bool allowInRow = false)
		:	pieceClass(pieceClass), weight(weight), placeCount(0), maxPlaceCount(maxPlaceCount), allowInRow(allowInRow) {}

		bool doPlace(int depth) const { return maxPlaceCount == 0 || placeCount < maxPlaceCount; }
		bool isValid() const { return maxPlaceCount == 0 || placeCount < maxPlaceCount; }
	};

	typedef std::list<PieceWeight*> WeightList;

	class StartPiece;

	class NetherBridgePiece: public StructurePiece
	{
	protected:
		NetherBridgePiece(int genDepth);

		StructurePiece* generateChildForward(StartPiece* startPiece, PieceList* pieces, Random* random, int xOff, int yOff, bool isCastle);
		StructurePiece* generateChildLeft(StartPiece* startPiece, PieceList* pieces, Random* random, int yOff, int zOff, bool isCastle);
		StructurePiece* generateChildRight(StartPiece* startPiece, PieceList* pieces, Random* random, int yOff, int zOff, bool isCastle);
		void hazards(Level* level, BoundingBox* chunkBB, int x0, int x1, int z0, int z1, int y);

	public:
		static bool isOkBox(BoundingBox* box);

	private:
		int updatePieceWeight(WeightList* currentPieces);
		NetherBridgePiece* generatePiece(StartPiece* startPiece, WeightList* currentPieces, PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int depth);
		StructurePiece* generateAndAddPiece(StartPiece* startPiece, PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int depth, bool isCastle);
	};

	class BridgeStraight: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 10;
		static const int depth = 19;
	public:
		BridgeStraight(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static BridgeStraight* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class BridgeEndFiller: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 10;
		static const int depth = 8;
		int selfSeed;
	public:
		BridgeEndFiller(int genDepth, Random* random, BoundingBox* box, int direction);
		static BridgeEndFiller* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class BridgeCrossing: public NetherBridgePiece
	{
	protected:
		static const int width = 19;
		static const int height = 10;
		static const int depth = 19;
		BridgeCrossing(Random* random, int west, int north);
	public:
		BridgeCrossing(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static BridgeCrossing* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	// The crossing every fortress grows from. It keeps the lists of pieces still
	// allowed, and the queue that makes children grow in a random order.
	class StartPiece: public BridgeCrossing
	{
	public:
		StartPiece(Random* random, int west, int north);

		PieceWeight* previousPiece;
		WeightList availableBridgePieces;
		WeightList availableCastlePieces;
		std::vector<StructurePiece*> pendingChildren;

	private:
		PieceWeight bridgeWeights[6];
		PieceWeight castleWeights[7];
	};

	class RoomCrossing: public NetherBridgePiece
	{
		static const int width = 7;
		static const int height = 9;
		static const int depth = 7;
	public:
		RoomCrossing(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static RoomCrossing* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class StairsRoom: public NetherBridgePiece
	{
		static const int width = 7;
		static const int height = 11;
		static const int depth = 7;
	public:
		StairsRoom(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static StairsRoom* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	// The raised platform with the blaze spawner.
	class MonsterThrone: public NetherBridgePiece
	{
		static const int width = 7;
		static const int height = 8;
		static const int depth = 9;
		bool hasPlacedMobSpawner;
	public:
		MonsterThrone(int genDepth, Random* random, BoundingBox* box, int direction);
		static MonsterThrone* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleEntrance: public NetherBridgePiece
	{
		static const int width = 13;
		static const int height = 14;
		static const int depth = 13;
	public:
		CastleEntrance(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleEntrance* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	// The hall with the nether wart garden.
	class CastleStalkRoom: public NetherBridgePiece
	{
		static const int width = 13;
		static const int height = 14;
		static const int depth = 13;
	public:
		CastleStalkRoom(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleStalkRoom* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleSmallCorridorPiece: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 7;
		static const int depth = 5;
	public:
		CastleSmallCorridorPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleSmallCorridorPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleSmallCorridorCrossingPiece: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 7;
		static const int depth = 5;
	public:
		CastleSmallCorridorCrossingPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleSmallCorridorCrossingPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleSmallCorridorRightTurnPiece: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 7;
		static const int depth = 5;
		bool isNeedingChest;
	public:
		CastleSmallCorridorRightTurnPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleSmallCorridorRightTurnPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleSmallCorridorLeftTurnPiece: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 7;
		static const int depth = 5;
		bool isNeedingChest;
	public:
		CastleSmallCorridorLeftTurnPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleSmallCorridorLeftTurnPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleCorridorStairsPiece: public NetherBridgePiece
	{
		static const int width = 5;
		static const int height = 14;
		static const int depth = 10;
	public:
		CastleCorridorStairsPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleCorridorStairsPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};

	class CastleCorridorTBalconyPiece: public NetherBridgePiece
	{
		static const int width = 9;
		static const int height = 7;
		static const int depth = 9;
	public:
		CastleCorridorTBalconyPiece(int genDepth, Random* random, BoundingBox* box, int direction);
		void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
		static CastleCorridorTBalconyPiece* createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth);
		bool postProcess(Level* level, Random* random, BoundingBox* chunkBB);
	};
}

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherBridgePieces_H__*/
