#ifndef NET_MINECRAFT_WORLD_EFFECT__MobEffect_H__
#define NET_MINECRAFT_WORLD_EFFECT__MobEffect_H__

/** Potion effects (docs/brewing-plan.md). Ids, colours and icons are Java's
    (MobEffect in the Console Edition), so more can be added by filling in
    the table; only water breathing exists so far. Icons are cells of
    gui/effects.png, Java's inventory.png grid of 18x18 icons. */
namespace MobEffects
{
	static const int WaterBreathing = 13;

	struct Info {
		int id;
		int color;		///< 0xRRGGBB: the particles and the potion
		int iconX, iconY;	///< cell in gui/effects.png
		const char* name;
	};

	inline const Info* get(int id) {
		static const Info table[] = {
			// Cyan-green like the potion here, not Java's blue (0x2E5299).
			{ WaterBreathing, 0x2EB4A6, 0, 2, "Water Breathing" },
		};
		for (unsigned int i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
			if (table[i].id == id) return &table[i];
		return 0;
	}
}

/// An effect a mob has, and how many ticks it has left.
struct MobEffectInstance {
	int id;
	int duration;
	MobEffectInstance(int id = 0, int duration = 0) : id(id), duration(duration) {}
};

#endif
