#ifndef FREESHOP_BROWSESTATE_HPP
#define FREESHOP_BROWSESTATE_HPP

#include "State.hpp"
#include "../AppList.hpp"
#include "../AppInfo.hpp"
#include "../DownloadQueue.hpp"
#include "../Keyboard/Keyboard.hpp"
#include "../RichText.hpp"
#include "../IconSet.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/RectangleShape.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Audio/SoundBuffer.hpp>
#include <cpp3ds/Audio/Sound.hpp>
#include <cpp3ds/Audio/Music.hpp>

namespace FreeShop {

class BrowseState : public State
{
public:
	BrowseState(StateStack& stack, Context& context);

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	enum Mode {
		App        = 0,
		Downloads,
		Settings,
		Search,
	};

	void initialize();
	void playMusic();
	void setMode(Mode mode);
	void setItemIndex(int index);
	void loadApp();

private:
	bool m_busy;
	Mode m_mode;
	AppList m_appList;
	AppInfo m_appInfo;
	util3ds::TweenText m_textListEmpty;

	TweenEngine::TweenManager m_tweenManager;

	float m_appListPositionX;

	cpp3ds::Thread m_threadInitialize;
	cpp3ds::Thread m_threadLoadApp;

	IconSet m_iconSet;
	int m_iconSelectedIndex;

	size_t m_activeDownloadCount;
	util3ds::TweenText m_textActiveDownloads;

	// Sounds
	cpp3ds::Sound  m_soundBlip;
	cpp3ds::Sound  m_soundClick;
	cpp3ds::Sound  m_soundLoading;

	cpp3ds::Music m_musicIntro;
	cpp3ds::Music m_musicLoop;
	cpp3ds::Thread m_threadMusic;

	// Keyboard
	util3ds::Keyboard m_keyboard;
	cpp3ds::String m_lastKeyboardInput;

	util3ds::TweenRectangleShape m_whiteScreen;

	std::vector<util3ds::RichText> m_textMatches;
};

} // namespace FreeShop

#endif // FREESHOP_BROWSESTATE_HPP
