#ifndef FREESHOP_SYNCSTATE_HPP
#define FREESHOP_SYNCSTATE_HPP

#include "State.hpp"
#include "../TweenObjects.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <TweenEngine/TweenManager.h>
#include <git2/oid.h>
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Audio/Sound.hpp>


namespace FreeShop {

class SyncState : public State
{
public:
	SyncState(StateStack& stack, Context& context);
	~SyncState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);
	void startupSound();
	void sync();

private:
	bool autoUpdate();
	void setStatus(const std::string& message);

	cpp3ds::Thread m_threadSync;
	cpp3ds::Thread m_threadStartupSound;
	cpp3ds::Clock m_timer;
	util3ds::TweenText m_textStatus;
	TweenEngine::TweenManager m_tweenManager;

	cpp3ds::Sound m_soundStartup;
	cpp3ds::Sound m_soundLoading;
};

} // namespace FreeShop

#endif // FREESHOP_SYNCSTATE_HPP
