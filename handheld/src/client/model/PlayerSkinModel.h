#ifndef NET_MINECRAFT_CLIENT_MODEL__PlayerSkinModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__PlayerSkinModel_H__

#include "HumanoidModel.h"

/** A player model laid out for a custom skin (see PlayerSkins).

    The base parts are HumanoidModel's own, so everything that poses or reads
    them -- the walk cycle, the held item, sneaking -- works unchanged. For the
    64x64 layouts they are rebuilt with that layout's texture offsets, and the
    left limbs stop mirroring the right ones. The outer layer is one extra part
    per base part, added as its child: a child is drawn inside its parent's
    transform, so the outer layer follows every pose without being told. */
class PlayerSkinModel: public HumanoidModel
{
	typedef HumanoidModel super;
public:
	/// skinModel is one of PlayerSkins::Model.
	explicit PlayerSkinModel(int skinModel);

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale);
	void onGraphicsReset();

private:
	void rebuild(ModelPart& part, int u, int v, float x0, float y0, float z0, int w, int h, int d);
	void addLayer(ModelPart& layer, ModelPart& parent, int u, int v, float x0, float y0, float z0, int w, int h, int d, float grow);

	ModelPart hat, jacket, sleeve0, sleeve1, pants0, pants1;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__PlayerSkinModel_H__*/
