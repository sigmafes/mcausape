#ifndef NET_MINECRAFT_WORLD_EFFECT__MobEffect_H__
#define NET_MINECRAFT_WORLD_EFFECT__MobEffect_H__

/** Potion effects (docs/brewing-plan.md). Ids, colours and icons are Java's
    (MobEffect in the Console Edition), so more can be added by filling in
    the table; only water breathing exists so far. Icons are cells of
    gui/effects.png, Java's inventory.png grid of 18x18 icons. */
namespace MobEffects
{
	static const int FireResistance = 12;
	static const int WaterBreathing = 13;
	static const int Poison = 19;
	/// Ours: no poison while it lasts (the Unpoison potion).
	static const int Unpoison = 40;
	/// The black squid's: ink over the view (no icon; Gui::renderInk).
	static const int Ink = 41;
	static const int InkTicks = 10 * 20;
	/// How long a pufferfish's poison lasts, stung or eaten.
	static const int PufferPoisonTicks = 15 * 20;

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
			{ Poison, 0x4E9331, 6, 0, "Poison" },
			{ FireResistance, 0xE49A3A, 7, 1, "Fire Resistance" },
			// Fuchsia; its icon is poison's with a red cross over it (0, 3).
			{ Unpoison, 0xFF00FF, 0, 3, "Unpoison" },
			{ Ink, 0x101010, -1, -1, "Ink" },
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
