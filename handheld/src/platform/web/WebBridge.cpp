#include "WebBridge.h"

#if defined(MC_WASM)
#include <emscripten.h>

// Every one of these tolerates the page not having the object it calls: an
// out-of-date shell.html should cost the feature, not the game.

EM_JS(int, mcpe_web_fs_supported, (), {
	return (window.mcpeFullscreen && window.mcpeFullscreen.supported()) ? 1 : 0;
});
EM_JS(int, mcpe_web_fs_is, (), {
	return (window.mcpeFullscreen && window.mcpeFullscreen.is()) ? 1 : 0;
});
EM_JS(void, mcpe_web_fs_set, (int on), {
	if (!window.mcpeFullscreen) return;
	if (on) window.mcpeFullscreen.enter(); else window.mcpeFullscreen.exit();
});
EM_JS(void, mcpe_web_skin_pick, (), {
	if (window.mcpeSkin) window.mcpeSkin.pick();
});
EM_JS(void, mcpe_web_skin_reset, (), {
	if (window.mcpeSkin) window.mcpeSkin.reset();
});
EM_JS(void, mcpe_web_skin_load, (), {
	if (window.mcpeSkin) window.mcpeSkin.loadSaved();
});
EM_JS(void, mcpe_web_world_export, (const char* id), {
	if (window.mcpeSaves) window.mcpeSaves.exportWorld(UTF8ToString(id));
});
EM_JS(void, mcpe_web_world_import, (), {
	if (window.mcpeSaves) window.mcpeSaves.importWorlds();
});
EM_JS(int, mcpe_web_worlds_changed, (), {
	return (window.mcpeSaves && window.mcpeSaves.takeChanged()) ? 1 : 0;
});
EM_JS(void, mcpe_web_toast, (const char* text), {
	if (window.mcpeToast) window.mcpeToast(UTF8ToString(text));
});

namespace WebBridge
{
	bool supported()					{ return true; }
	bool fullscreenSupported()			{ return mcpe_web_fs_supported() != 0; }
	bool isFullscreen()					{ return mcpe_web_fs_is() != 0; }
	void setFullscreen(bool on)			{ mcpe_web_fs_set(on ? 1 : 0); }
	void pickSkin()						{ mcpe_web_skin_pick(); }
	void resetSkin()					{ mcpe_web_skin_reset(); }
	void loadSavedSkin()				{ mcpe_web_skin_load(); }
	void exportWorld(const std::string& id) { mcpe_web_world_export(id.c_str()); }
	void importWorlds()					{ mcpe_web_world_import(); }
	bool takeWorldsChanged()			{ return mcpe_web_worlds_changed() != 0; }
	void toast(const std::string& text)	{ mcpe_web_toast(text.c_str()); }
}

#else

namespace WebBridge
{
	bool supported()					{ return false; }
	bool fullscreenSupported()			{ return false; }
	bool isFullscreen()					{ return false; }
	void setFullscreen(bool)			{}
	void pickSkin()						{}
	void resetSkin()					{}
	void loadSavedSkin()				{}
	void exportWorld(const std::string&) {}
	void importWorlds()					{}
	bool takeWorldsChanged()			{ return false; }
	void toast(const std::string&)		{}
}

#endif
