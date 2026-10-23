#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Axolotl_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Axolotl_H__

#include "Fish.h"

/** The Aquarium's axolotl, tamed like a wolf: fed a raw fish (cod, salmon,
    pufferfish or shrimp), one time in three it takes the feeder as owner.
    Tamed, it follows them, comes to them when more than 12 blocks off,
    goes after whatever they hit, and when they leave the Aquarium waits by
    its portal. A water bucket picks it up (AxolotlBucketItem). It never
    despawns, and walks rather than flops out of water. */
class Axolotl: public Fish
{
	typedef Fish super;
public:
	Axolotl(Level* level);
	int getEntityTypeId() const;
	int getMaxHealth();
	int getMaxSpawnClusterSize() { return 2; }
	bool interact(Player* player);
	void aiStep();
	bool hurt(Entity* source, int dmg);

	void addAdditonalSaveData(CompoundTag* tag);
	void readAdditionalSaveData(CompoundTag* tag);

	/// The owner's name, empty while wild.
	std::string owner;

	static bool isTamingFood(int itemId);

protected:
	bool removeWhenFarAway() { return false; }
	int getDeathLoot() { return 0; }
	void dropDeathLoot() {}
	void dryStep();

private:
	Player* findOwner();
	int attackId;
	int biteCooldown;
};

#endif
