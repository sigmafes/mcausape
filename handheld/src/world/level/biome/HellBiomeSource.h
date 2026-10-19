#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiomeSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiomeSource_H__

//package net.minecraft.world.level.biome;

#include "BiomeSource.h"
#include "Biome.h"

// Bioma fijo para el Nether (equivalente a la FixedBiomeSource de MCPE 1.0)
class HellBiomeSource: public BiomeSource
{
	Biome** fixedBiomes;

public:
	HellBiomeSource(Level* level)
	:	BiomeSource(level),
		fixedBiomes(NULL)
	{
	}

	~HellBiomeSource() {
		delete[] fixedBiomes;
	}

	virtual Biome** getBiomeBlock(int x, int z, int w, int h) {
		int size = w * h;
		if (fixedBiomes == NULL || lenBiomes < size) {
			delete[] fixedBiomes;
			fixedBiomes = new Biome*[size];
			lenBiomes = size;
		}
		for (int i = 0; i < size; i++) {
			fixedBiomes[i] = Biome::hell;
		}
		return fixedBiomes;
	}

	virtual float* getTemperatureBlock(int x, int z, int w, int h) {
		// Temperatura constante: el generador la usa para nieve/decoracion
		int size = w * h;
		if (lenTemperatures < size) {
			delete[] temperatures;
			temperatures = new float[size];
			lenTemperatures = size;
		}
		for (int i = 0; i < size; i++) {
			temperatures[i] = 0.5f;
		}
		return temperatures;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiomeSource_H__*/
