#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__

#include "IBuildInput.h"
#include "../../../platform/input/Mouse.h"

/** A Mouse Build input */
class MouseBuildInput : public IBuildInput {
public:
	MouseBuildInput()
	:	buildDelayTicks(10),
		buildHoldTicks(0),
		leftHoldTicks(0)
	{}

	virtual bool tickBuild(Player* p, BuildActionIntention* bai) {
		if (Mouse::getButtonState(MouseAction::ACTION_LEFT) != 0) {
			// The first tick of a press is the one Minecraft::tickInput lets
			// through -- a bare BAI_REMOVE reads as "continuing" and is
			// skipped, which is why a click on a mob never reached
			// handleBuildAction and nothing happened at all. FIRSTREMOVE
			// carries the same meaning for tiles (start destroying) while
			// still getting there, which is where the entity under the
			// crosshair is attacked. Same shape as a tap on the touch UI.
			const int remove = (++leftHoldTicks == 1)
					? BuildActionIntention::BAI_FIRSTREMOVE
					: BuildActionIntention::BAI_REMOVE;
			*bai = BuildActionIntention(remove | BuildActionIntention::BAI_ATTACK);
			return true;
		}
		leftHoldTicks = 0;
		if (Mouse::getButtonState(MouseAction::ACTION_RIGHT) != 0) {
			if (buildHoldTicks >= buildDelayTicks) buildHoldTicks = 0;
			if (++buildHoldTicks == 1) {
				*bai = BuildActionIntention(BuildActionIntention::BAI_BUILD | BuildActionIntention::BAI_INTERACT);
				return true;
			}
		} else {
			buildHoldTicks = 0;
		}
		return false;
	}
private:
	int buildHoldTicks;
	int buildDelayTicks;
	int leftHoldTicks;
};

#endif /*NET_MINECRAFT_CLIENT_PLAYER_INPUT_MouseBuildInput_H__*/
