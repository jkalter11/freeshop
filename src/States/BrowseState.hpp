#ifndef FREESHOP_BROWSESTATE_HPP
#define FREESHOP_BROWSESTATE_HPP

#include "State.hpp"
#include "../AppList.hpp"
#include "../AppInfo.hpp"
#include "../DownloadQueue.hpp"
#include "../Keyboard/Keyboard.hpp"
#include "../RichText.hpp"
#include "../IconSet.hpp"
#include "../BotInformations.hpp"
#include "../GUI/Settings.hpp"
#include "../GUI/ScrollBar.hpp"
#include "../MusicBCSTM.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/RectangleShape.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Audio/SoundBuffer.hpp>
#include <cpp3ds/Audio/Sound.hpp>
#include <cpp3ds/Audio/Music.hpp>

#include <Gwen/Renderers/cpp3ds.h>
#include <Gwen/Input/cpp3ds.h>
#include <Gwen/Skins/Simple.h>
#include <Gwen/Skins/TexturedBase.h>

namespace FreeShop {

class BrowseState : public State
{
public:
	BrowseState(StateStack& stack, Context& context, StateCallback callback);
	~BrowseState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

	bool playBGM(const std::string &filename);
	bool playBGMeShop();
	void stopBGM();

	static cpp3ds::Clock clockDownloadInactivity;

private:
	enum Mode {
		Info = 0,
		App,
		Downloads,
		Installed,
		Settings,
		Search,
	};

	void initialize();
	void setMode(Mode mode);
	void loadApp();

private:
	AppInfo m_appInfo;
	util3ds::TweenText m_textListEmpty;

	TweenEngine::TweenManager m_tweenManager;

	float m_appListPositionX;

	cpp3ds::Thread m_threadInitialize;
	cpp3ds::Thread m_threadLoadApp;
	bool m_threadBusy;

	IconSet m_iconSet;
	IconSet m_iconInfo;
	BotInformations m_botInfos;

	size_t m_activeDownloadCount;
	util3ds::TweenText m_textActiveDownloads;

	ScrollBar m_scrollbarInstalledList;
	ScrollBar m_scrollbarDownloadQueue;

	// Backgrounds
	bool m_topBG;
	bool m_botBG;

	gui3ds::NinePatch m_rectTopBG;
	gui3ds::NinePatch m_rectBotBG;

	// Sounds
	cpp3ds::Sound  m_soundClick;
	cpp3ds::Sound  m_soundLoading;

	MusicBCSTM m_musicLoopBCSTM;
	cpp3ds::Music m_musicLoop;

	// Keyboard
	util3ds::Keyboard m_keyboard;
	cpp3ds::String m_lastKeyboardInput;

	std::vector<util3ds::RichText> m_textMatches;

	// Transitioning
	bool m_isTransitioning;
	Mode m_mode;
	util3ds::TweenRectangleShape m_whiteScreen;

	// GWEN
	Gwen::Renderer::cpp3dsRenderer *m_gwenRenderer;
	Gwen::Skin::TexturedBase *m_gwenSkin;
	GUI::Settings *m_settingsGUI;
};

extern BrowseState *g_browseState;

} // namespace FreeShop

#endif // FREESHOP_BROWSESTATE_HPP
