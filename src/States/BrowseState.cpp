#include "BrowseState.hpp"
#include "SyncState.hpp"
#include "../Notification.hpp"
#include "../AssetManager.hpp"
#include "../Util.hpp"
#include "../Installer.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <iostream>
#include <sstream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>


namespace FreeShop {

BrowseState::BrowseState(StateStack& stack, Context& context)
: State(stack, context)
, m_appList("sdmc:/freeShop/cache/data.json")
, m_appListPositionX(0.f)
, m_loadThread(&BrowseState::loadApp, this)
, m_iconSelectedIndex(-1)
, m_busy(false)
, m_activeDownloadCount(0)
{
	m_iconSet.addIcon(L"\uf11b");
	m_iconSet.addIcon(L"\uf019");
	m_iconSet.addIcon(L"\uf013");
	m_iconSet.addIcon(L"\uf002");
	m_iconSet.setPosition(210.f, 15.f);

	m_textActiveDownloads.setString("23");
	m_textActiveDownloads.setCharacterSize(8);
	m_textActiveDownloads.setFillColor(cpp3ds::Color::Black);
	m_textActiveDownloads.setOutlineColor(cpp3ds::Color::White);
	m_textActiveDownloads.setOutlineThickness(1.f);
	m_textActiveDownloads.setPosition(246.f, 3.f);

	m_whiteScreen.setSize(cpp3ds::Vector2f(320.f, 240.f));
	m_whiteScreen.setFillColor(cpp3ds::Color::White);

	m_keyboard.loadFromFile("kb/keyboard.xml");
	m_keyboard.setPosition(0.f, 240.f);
	DownloadQueue::getInstance().setPosition(0.f, 240.f);
	m_appInfo.setPosition(0.f, 240.f);

	m_textMatches.resize(4);
	for (auto& text : m_textMatches)
	{
		text.setCharacterSize(13);
		text.useSystemFont();
	}

	loadApp();
	setMode(App);

	m_soundBlip.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/blip.ogg"));
}

void BrowseState::renderTopScreen(cpp3ds::Window& window)
{
	window.draw(m_appList);

	// Special draw method to draw top screenshot if selected
	m_appInfo.drawTop(window);
}

void BrowseState::renderBottomScreen(cpp3ds::Window& window)
{
	if (m_keyboard.getPosition().y < 240.f)
	{
		window.draw(m_keyboard);
		for (auto& textMatch : m_textMatches)
			window.draw(textMatch);
	}

	window.draw(m_iconSet);

	if (m_activeDownloadCount > 0)
		window.draw(m_textActiveDownloads);

	if (m_appInfo.getPosition().y < 240.f)
		window.draw(m_appInfo);
	if (DownloadQueue::getInstance().getPosition().y < 240.f)
		window.draw(DownloadQueue::getInstance());

//	window.draw(m_whiteScreen);
}

bool BrowseState::update(float delta)
{
	int iconIndex = m_iconSet.getSelectedIndex();
	if (m_iconSelectedIndex != iconIndex && m_mode != iconIndex)
	{
		m_iconSelectedIndex = iconIndex;
		setMode((Mode)iconIndex);
	}

	if (m_mode == App)
	{
		m_appInfo.update(delta);
	}

	if (m_activeDownloadCount != DownloadQueue::getInstance().getActiveCount())
	{
		m_activeDownloadCount = DownloadQueue::getInstance().getActiveCount();
		m_textActiveDownloads.setString(_("%u", m_activeDownloadCount));
	}

	m_iconSet.update(delta);
	m_appList.update(delta);
	m_keyboard.update(delta);
	DownloadQueue::getInstance().update(delta);
	m_tweenManager.update(delta);
	return true;
}

bool BrowseState::processEvent(const cpp3ds::Event& event)
{
	if (m_busy)
		return false;

	if (m_mode == App) {
		if (!m_appInfo.processEvent(event))
			return false;
	} else {
		DownloadQueue::getInstance().processEvent(event);
	}

	m_iconSet.processEvent(event);

	if (m_mode == Search)
	{
		if (!m_keyboard.processEvents(event))
			return true;

		cpp3ds::String currentInput;
		if (m_keyboard.popString(currentInput))
		{
			// Enter was pressed, so close keyboard
			m_iconSet.setSelectedIndex(App);
			m_lastKeyboardInput.clear();
		}
		else
		{
			currentInput = m_keyboard.getCurrentInput();
			if (m_lastKeyboardInput != currentInput)
			{
				m_lastKeyboardInput = currentInput;
				m_appList.filterBySearch(currentInput, m_textMatches);
				TweenEngine::Tween::to(m_appList, AppList::POSITION_XY, 0.3f)
					.target(0.f, 0.f)
					.start(m_tweenManager);
			}
		}
	}
	else
	{
		// Events for all modes except Search
		if (event.type == cpp3ds::Event::KeyPressed)
		{
			int index = m_appList.getSelectedIndex();

			switch (event.key.code)
			{
				case cpp3ds::Keyboard::DPadUp:
					m_soundBlip.play();
					if (index % 4 == 0)
						break;
					setItemIndex(index - 1);
					break;
				case cpp3ds::Keyboard::DPadDown:
					m_soundBlip.play();
					if (index % 4 == 3)
						break;
					setItemIndex(index + 1);
					break;
				case cpp3ds::Keyboard::DPadLeft:
#ifdef EMULATION
				case cpp3ds::Keyboard::X:
#endif
					m_soundBlip.play();
					setItemIndex(index - 4);
					break;
				case cpp3ds::Keyboard::DPadRight:
#ifdef EMULATION
				case cpp3ds::Keyboard::Y:
#endif
					m_soundBlip.play();
					setItemIndex(index + 4);
					break;
				case cpp3ds::Keyboard::L:
					setItemIndex(index - 8);
					break;
				case cpp3ds::Keyboard::R:
					setItemIndex(index + 8);
					break;
				default:
					break;
			}
		}
	}

	if (event.type == cpp3ds::Event::KeyPressed)
	{
		int index = m_appList.getSelectedIndex();

		switch (event.key.code)
		{
			case cpp3ds::Keyboard::Select:
				requestStackClear();
				return true;
			case cpp3ds::Keyboard::A: {
				m_busy = true;
				m_loadThread.launch();
//				loadApp();
				break;
			}
			case cpp3ds::Keyboard::B:
				break;
			default:
				break;
		}
	}

	return true;
}


void BrowseState::setItemIndex(int index)
{
	if (index < 0)
		index = 0;
	else if (index >= m_appList.getVisibleCount())
		index = m_appList.getVisibleCount() - 1;

	float extra = 1.0f; //std::abs(m_appList.getSelectedIndex() - index) == 8.f ? 2.f : 1.f;

	float pos = -200.f * extra * (index / 4);
	if (pos > m_appListPositionX)
		m_appListPositionX = pos;
	else if (pos <= m_appListPositionX - 400.f)
		m_appListPositionX = pos + 200.f * extra;

	TweenEngine::Tween::to(m_appList, AppList::POSITION_X, 0.3f)
			.target(m_appListPositionX)
			.start(m_tweenManager);

	m_appList.setSelectedIndex(index);
}


void BrowseState::loadApp()
{
	AppItem* item = m_appList.getSelected();
	m_iconSet.setSelectedIndex(App);
	if (m_appInfo.getAppItem() == item)
	{
		m_busy = false;
		return;
	}

	TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.5f)
		.target(0.f)
		.start(m_tweenManager);

	// No cache to load, so show loading state
	bool showLoading = true; //!item->isCached();
	if (showLoading)
		requestStackPush(States::Loading);

	m_appInfo.loadApp(item);

	TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.5f)
		.target(255.f)
		.start(m_tweenManager);

	if (showLoading)
		requestStackPop();

	m_busy = false;
}


void BrowseState::setMode(BrowseState::Mode mode)
{
	if (m_mode == mode)
		return;

	// Transition / end current mode
	if (m_mode == Search)
	{
		float delay = 0.f;
		for (auto& text : m_textMatches)
		{
			TweenEngine::Tween::to(text, util3ds::RichText::POSITION_X, 0.3f)
					.target(-text.getLocalBounds().width)
					.delay(delay)
					.start(m_tweenManager);
			delay += 0.05f;
		}

		m_appList.setCollapsed(false);
	}

	TweenEngine::Tween::to(m_keyboard, util3ds::Keyboard::POSITION_XY, 0.5f)
		.target(0.f, 240.f)
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_appInfo, AppInfo::POSITION_Y, 0.3f)
		.target(240.f)
		.start(m_tweenManager);
	TweenEngine::Tween::to(DownloadQueue::getInstance(), AppInfo::POSITION_Y, 0.3f)
		.target(240.f)
		.start(m_tweenManager);

	// Transition / start new mode
	if (mode == App)
	{
		TweenEngine::Tween::to(m_appInfo, AppInfo::POSITION_Y, 0.3f)
			.target(0.f)
			.delay(0.3f)
			.start(m_tweenManager);
	}
	else if (mode == Downloads)
	{
		TweenEngine::Tween::to(DownloadQueue::getInstance(), AppInfo::POSITION_Y, 0.3f)
			.target(0.f)
			.delay(0.3f)
			.start(m_tweenManager);
	}
	else if (mode == Search)
	{
		float posY = 1.f;
		for (auto& text : m_textMatches)
		{
			text.clear();
			text.setPosition(5.f, posY);
			posY += 13.f;
		}
		m_appList.setCollapsed(true);
		TweenEngine::Tween::to(m_keyboard, util3ds::Keyboard::POSITION_XY, 0.3f)
			.target(0.f, 0.f)
			.delay(0.3f)
			.start(m_tweenManager);

		m_lastKeyboardInput = "";
		m_keyboard.setCurrentInput(m_lastKeyboardInput);
	}

	m_mode = mode;
}

} // namespace FreeShop
