#ifndef NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__
#define NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__

class SharedFlagsInformation {
public:
	enum SharedFlags {
		FLAG_ONFIRE = 0,
		FLAG_SNEAKING = 1,
		FLAG_RIDING = 2,
		FLAG_SPRINTING = 3,
		FLAG_USINGITEM = 4,
		/// Java Edition's swimming (sprinting under water): lying flat, a 0.6
		/// tall box, faster. Shared so everyone draws it (see Player::updateSwimPose).
		FLAG_SWIMMING = 5
	};
	typedef char SharedFlagsInformationType;
	
	static const unsigned int DATA_SHARED_FLAGS_ID = 0;
};
#endif /* NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__ */