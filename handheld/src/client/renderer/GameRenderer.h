#ifndef NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__

//package net.minecraft.client.renderer;

#include "gles.h"
#include <cstddef>
#include "../../util/SmoothFloat.h"
#include "../../world/phys/Vec3.h"
#include "../gui/components/ImageButton.h"

class Minecraft;
class Entity;
class ItemInHandRenderer;
class LevelRenderer;
class GameRenderer
{
public:
	GameRenderer(Minecraft* mc_);
	~GameRenderer();

	void pick(float a);

	void updateAllChunks();

	void zoomRegion(float zoom, float xa, float ya);
	void unZoomRegion();
	void setupGuiScreen(bool clearColorBuffer);

	void tick(int nTick, int maxTick);
	void render(float a);
	/// /panorama: on the next frame, the world from the player's eyes in the
	/// six directions, 90 degrees each, saved as one PNG strip.
	void requestPanorama() { _panoramaRequested = true; }
	bool _panoramaRequested;
	bool _panorama;
	void capturePanorama(float a);
	void renderLevel(float a);
	void renderItemInHand(float a, int eye);

	void onGraphicsReset();

	void setupCamera(float a, int eye);
	void moveCameraToPlayer(float a);

	void setupClearColor(float a);
	float getFov(float a, bool applyEffects);
private:
	void setupFog(int i);

	void tickFov();
	

	void bobHurt(float a);
	void bobView(float a);

	bool updateFreeformPickDirection(float a, Vec3& outDir);
	void prepareAndRenderClouds(LevelRenderer* levelRenderer, float a);

public:
	ItemInHandRenderer* itemInHandRenderer;

private:
	Minecraft* mc;

	float renderDistance;
	int _tick;
	Vec3 pickDirection;

	// smooth camera movement
	SmoothFloat smoothTurnX;
	SmoothFloat smoothTurnY;

	//    // third-person distance etc
	//    SmoothFloat smoothDistance = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRotation = /*new*/ SmoothFloat();
	//    SmoothFloat smoothTilt = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRoll = /*new*/ SmoothFloat();

	float thirdDistance;
	float thirdDistanceO;
	float thirdRotation;
	float thirdRotationO;
	float thirdTilt;
	float thirdTiltO;

	// zoom
	float zoom;
	float zoom_x;
	float zoom_y;

	// fov modification
	float fov, oFov;
	float fovOffset;
	float fovOffsetO;
	float _setupCameraFov;
	bool _postKeepsDepth;
	void shaderParams(float a, float* f);
	float renderScale();
	void updateAutoScale();
	void perfAdd(double shadow, double world, double water, double post);
	int _postW, _postH;
	float _autoScale;
	double _perfWater;
public:
	/// /perf: on, glFinish marks the frame's parts for timing (perf).
	bool _perf;
	struct PerfStats { float shadow, world, water, post, scale; };
	static PerfStats perf;
private:
	void renderShadowMap(float a);
	bool _shadowPass;	///< a shadow map is valid (drawn within 3 s)
	int _shadowAt;
	float _shTh, _shOrigin[3];
	static const int ShadowRadius = 64, ShadowDepth = 128;

	// roll modification
	float cameraRoll;
	float cameraRollO;

	float fr;
	float fg;
	float fb;

	float fogBrO, fogBr;
	/// Ticks with the camera's eyes under water: Java's waterVisionTime, which
	/// the underwater fog opens out with (waterVision).
	int waterVisionTime;
	float waterVision(float a);

	float _rotX;
	float _rotY;
	float _rotXlast;
	float _rotYlast;
	float _lastTickT;

	void saveMatrices();
	float lastProjMatrix[16];
	float lastModelMatrix[16];

	// Scissor area that Minecraft::screen defines
	bool useScreenScissor;
	IntRectangle screenScissorArea;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__*/
