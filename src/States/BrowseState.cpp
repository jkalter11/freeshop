#include "BrowseState.hpp"
#include "SyncState.hpp"
#include "../Notification.hpp"
#include "../AssetManager.hpp"
#include "../Util.hpp"
#include "../Installer.hpp"
#include "SleepState.hpp"
#include "../InstalledList.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"
#include "../FreeShop.hpp"
#include "../Theme.hpp"
#ifndef EMULATION
#include "../KeyboardApplet.hpp"
#endif
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <sstream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <time.h>

#define SECONDS_TO_SLEEP 60.f


namespace FreeShop {

BrowseState *g_browseState = nullptr;
bool g_isLatestFirmwareVersion = true;
cpp3ds::Clock BrowseState::clockDownloadInactivity;

BrowseState::BrowseState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_appListPositionX(0.f)
, m_threadInitialize(&BrowseState::initialize, this)
, m_threadLoadApp(&BrowseState::loadApp, this)
, m_threadBusy(false)
, m_activeDownloadCount(0)
, m_mode(Downloads)
, m_gwenRenderer(nullptr)
, m_gwenSkin(nullptr)
, m_settingsGUI(nullptr)
, m_isTransitioning(false)
, m_isJapKeyboard(false)
, m_isTIDKeyboard(false)
{
	g_browseState = this;
	m_musicLoop.setLoop(true);

#ifdef EMULATION
	g_syncComplete = true;
	initialize();
#else
	m_threadInitialize.setRelativePriority(1);
	m_threadInitialize.launch();
#endif
}

BrowseState::~BrowseState()
{
	m_settingsGUI->saveToConfig();
	Config::saveToFile();

	if (m_settingsGUI)
		delete m_settingsGUI;
	if (m_gwenSkin)
		delete m_gwenSkin;
	if (m_gwenRenderer)
		delete m_gwenRenderer;
}

void BrowseState::initialize()
{
	// Initialize AppList singleton first
	AppList::getInstance().refresh();
	InstalledList::getInstance().refresh();

	//Var init
	m_topBG = false;
	m_botBG = false;
	m_ctrSdPath = "";
	m_keyHistory = {};

	m_iconSet.addIcon(L"\uf0ae");
	m_iconSet.addIcon(L"\uf290");
	m_iconSet.addIcon(L"\uf019");
	m_iconSet.addIcon(L"\uf11b");
	m_iconSet.addIcon(L"\uf013");
	m_iconSet.addIcon(L"\uf002");
	m_iconSet.setPosition(60.f, 13.f);

	m_textActiveDownloads.setCharacterSize(8);
	m_textActiveDownloads.setFillColor(cpp3ds::Color::Black);
	m_textActiveDownloads.setOutlineColor(cpp3ds::Color::White);
	m_textActiveDownloads.setOutlineThickness(1.f);
	m_textActiveDownloads.setPosition(128.f, 3.f);

	//If there's no title key available, or no cache, one of these messages will appear
	if (TitleKeys::getIds().empty())
		m_textListEmpty.setString(_("No title keys found.\nMake sure you have keys in\n%s\n\nManually copy keys to the directory\nor check settings to enter a URL\nfor downloading title keys.", FREESHOP_DIR "/keys/"));
	else
		m_textListEmpty.setString(_("No cache entries found\nfor your title keys.\n\nTry refreshing cache in settings.\nIf that doesn't work, then your\ntitles simply won't work with\nfreeShop currently."));
	m_textListEmpty.useSystemFont();
	m_textListEmpty.setCharacterSize(16);
	m_textListEmpty.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textListEmpty.setPosition(200.f, 140.f);
	m_textListEmpty.setOrigin(m_textListEmpty.getLocalBounds().width / 2, m_textListEmpty.getLocalBounds().height / 2);

	//Load keyboard file
	reloadKeyboard();

	m_textMatches.resize(4);
	for (auto& text : m_textMatches)
	{
		text.setCharacterSize(13);
		text.useSystemFont();
	}


	m_scrollbarInstalledList.setPosition(314.f, 30.f);
	m_scrollbarInstalledList.setDragRect(cpp3ds::IntRect(0, 30, 320, 210));
	m_scrollbarInstalledList.setScrollAreaSize(cpp3ds::Vector2u(320, 210));
	m_scrollbarInstalledList.setSize(cpp3ds::Vector2u(8, 210));
	m_scrollbarInstalledList.setColor(cpp3ds::Color(150, 150, 150, 150));
	m_scrollbarDownloadQueue = m_scrollbarInstalledList;

	m_scrollbarInstalledList.attachObject(&InstalledList::getInstance());
	m_scrollbarDownloadQueue.attachObject(&DownloadQueue::getInstance());

	setMode(Info);

#ifdef _3DS
	while (!m_gwenRenderer)
		cpp3ds::sleep(cpp3ds::milliseconds(10));
	m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);

	if (pathExists(FREESHOP_DIR "/theme/images/DefaultSkin.png", true))
		m_gwenSkin->Init(FREESHOP_DIR "/theme/images/DefaultSkin.png");
	else
		m_gwenSkin->Init("images/DefaultSkin.png");

	m_gwenSkin->SetDefaultFont(L"", 11);
	
	//Check if the system firmware is the latest for sleep download
	NIMS_WantUpdate(&g_isLatestFirmwareVersion);
	g_isLatestFirmwareVersion = !g_isLatestFirmwareVersion;

	// Need to wait until title screen is done to prevent music from
	// settings starting prematurely.
	while(!g_syncComplete)
		cpp3ds::sleep(cpp3ds::milliseconds(10));
		
	//White screen used for transitions
	m_whiteScreen.setPosition(0.f, 30.f);
	m_whiteScreen.setSize(cpp3ds::Vector2f(320.f, 210.f));
	if (Theme::isTextThemed)
		m_whiteScreen.setFillColor(Theme::transitionScreenColor);
	else
		m_whiteScreen.setFillColor(cpp3ds::Color::White);
	
	m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif


	if (pathExists(FREESHOP_DIR "/theme/images/topBG.png", true)) {
		m_rectTopBG.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/topBG.png"));
		m_rectTopBG.setPosition(0.f, 0.f);
		m_topBG = true;
	}

	if (pathExists(FREESHOP_DIR "/theme/images/botBG.png", true)) {
		m_rectBotBG.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/botBG.png"));
		m_rectBotBG.setPosition(0.f, 0.f);
		m_botBG = true;
	}

#ifndef EMULATION
	//Get the /Nintendo 3DS/<id0>/<id1> path
	fsInit();
	u8 * outdata = static_cast<u8 *>(malloc(1024));
	FSUSER_GetSdmcCtrRootPath(outdata, 1024);
	char* charOut;
	std::string ctrPath;

	for (size_t i = 0; i < 158; ++i) {
  		if (i % 2 == 0) {
			charOut = (char*)outdata + i;
			ctrPath += charOut;
		}
	}

	fsExit();

	m_ctrSdPath = ctrPath;
	free(outdata);
#endif

	g_browserLoaded = true;

	SleepState::clock.restart();
	clockDownloadInactivity.restart();
	requestStackClearUnder();
}

void BrowseState::renderTopScreen(cpp3ds::Window& window)
{
	if (!g_syncComplete || !g_browserLoaded)
		return;

	if (m_topBG == true)
		window.draw(m_rectTopBG);

	if (AppList::getInstance().getList().size() == 0) {
		window.draw(m_textListEmpty);
	} else {
		window.draw(AppList::getInstance());
	}

	// Special draw method to draw top screenshot if selected
	m_appInfo.drawTop(window);
}

void BrowseState::renderBottomScreen(cpp3ds::Window& window)
{
	if (!m_gwenRenderer)
	{
		m_gwenRenderer = new Gwen::Renderer::cpp3dsRenderer(window);

#ifdef EMULATION
		m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);

		if (fopen(FREESHOP_DIR "/theme/images/DefaultSkin.png", "rb")) {
			m_gwenSkin->Init(FREESHOP_DIR "/theme/images/DefaultSkin.png");
		} else {
			m_gwenSkin->Init("images/DefaultSkin.png");
		}

		m_gwenSkin->SetDefaultFont(L"", 11);
		m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif
	}
	if (!g_syncComplete || !g_browserLoaded)
		return;
		
	if (m_botBG == true)
		window.draw(m_rectBotBG);

	window.draw(m_topInfos);

	if (m_mode == Search)
	{
		window.draw(m_keyboard);
		for (auto& textMatch : m_textMatches)
			window.draw(textMatch);
	}
	if (m_mode == Info)
		window.draw(m_botInfos);
	if (m_mode == Settings)
	{
		m_settingsGUI->render();
	}

	window.draw(m_iconSet);

	if (m_activeDownloadCount > 0)
		window.draw(m_textActiveDownloads);

	if (m_mode == App)
		window.draw(m_appInfo);
	if (m_mode == Downloads)
	{
		window.draw(DownloadQueue::getInstance());
		window.draw(m_scrollbarDownloadQueue);
	}
	if (m_mode == Installed)
	{
		window.draw(InstalledList::getInstance());
		window.draw(m_scrollbarInstalledList);
	}

	if (m_isTransitioning)
		window.draw(m_whiteScreen);
}

bool BrowseState::update(float delta)
{
	if (!g_syncComplete || !g_browserLoaded)
		return true;
	if (m_threadBusy)
	{
		clockDownloadInactivity.restart();
		SleepState::clock.restart();
	}

	// Show latest news if requested
	if (Config::get(Config::ShowNews).GetBool())
	{
		Config::set(Config::ShowNews, false);
		requestStackPush(States::News);
	}

	// Go into sleep state after inactivity
	if (!SleepState::isSleeping && Config::get(Config::SleepMode).GetBool() && SleepState::clock.getElapsedTime() > cpp3ds::seconds(SECONDS_TO_SLEEP))
	{
		stopBGM();
		requestStackPush(States::Sleep);
		return false;
	}

	// Power off after sufficient download inactivity
	if (m_activeDownloadCount == 0
		&& Config::get(Config::PowerOffAfterDownload).GetBool()
		&& clockDownloadInactivity.getElapsedTime() > cpp3ds::seconds(Config::get(Config::PowerOffTime).GetInt()))
	{
		g_requestShutdown = true;
		return false;
	}

	// If selected icon changed, change mode accordingly
	// If the selected mode is Search and the "Use system keyboard" option is enabled, show the System keyboard
	int iconIndex = m_iconSet.getSelectedIndex();
	if (iconIndex == Search && Config::get(Config::SystemKeyboard).GetBool()) {
		m_iconSet.setSelectedIndex(m_mode);
#ifndef EMULATION
		//Check if the keyboard mode is Title ID
		if (m_isTIDKeyboard) {
			KeyboardApplet kb(KeyboardApplet::TitleID);
			swkbdSetHintText(kb, _("Type a game Title ID...").toAnsiString().c_str());
			cpp3ds::String input = kb.getInput();
			if (!input.isEmpty())
				AppList::getInstance().filterBySearch(input.toAnsiString(), m_textMatches);
		} else {
			KeyboardApplet kb(KeyboardApplet::Text);
			swkbdSetHintText(kb, _("Type a game name...").toAnsiString().c_str());
			
			cpp3ds::String input = kb.getInput();
			if (!input.isEmpty())
				AppList::getInstance().filterBySearch(input.toAnsiString(), m_textMatches);
		}
#else
		std::cout << "System keyboard." << std::endl;
#endif
	}
	else if (m_mode != iconIndex && iconIndex >= 0)
		setMode(static_cast<Mode>(iconIndex));

	// Update the active mode
	if (m_mode == App)
	{
		m_appInfo.update(delta);
	}
	else if (m_mode == Downloads)
	{
		DownloadQueue::getInstance().update(delta);
		m_scrollbarDownloadQueue.update(delta);
	}
	else if (m_mode == Installed)
	{
		InstalledList::getInstance().update(delta);
		m_scrollbarInstalledList.update(delta);
	}
	else if (m_mode == Search)
	{
		m_keyboard.update(delta);
	}

	if (m_activeDownloadCount != DownloadQueue::getInstance().getActiveCount())
	{
		clockDownloadInactivity.restart();
		m_activeDownloadCount = DownloadQueue::getInstance().getActiveCount();
		m_textActiveDownloads.setString(_("%u", m_activeDownloadCount));
	}

	m_iconSet.update(delta);
	m_topInfos.update(delta);
	m_botInfos.update(delta);
	AppList::getInstance().update(delta);
	m_tweenManager.update(delta);

	return true;
}

bool BrowseState::processEvent(const cpp3ds::Event& event)
{
	if (SleepState::isSleeping)
	{
		m_settingsGUI->playMusic();
		return true;
	}


	SleepState::clock.restart();
	clockDownloadInactivity.restart();

	if (m_threadBusy || !g_syncComplete || !g_browserLoaded)
		return false;

	if (m_mode == App) {
		if (!m_appInfo.processEvent(event))
			return false;
	}
	else if (m_mode == Downloads) {
		if (!m_scrollbarDownloadQueue.processEvent(event))
			DownloadQueue::getInstance().processEvent(event);
	} else if (m_mode == Installed) {
		if (!m_scrollbarInstalledList.processEvent(event))
			InstalledList::getInstance().processEvent(event);
	} else if (m_mode == Settings) {
		m_settingsGUI->processEvent(event);
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
				AppList::getInstance().filterBySearch(currentInput, m_textMatches);
				TweenEngine::Tween::to(AppList::getInstance(), AppList::POSITION_XY, 0.3f)
					.target(0.f, 0.f)
					.start(m_tweenManager);
			}
		}
	}
	else
	{
		// Events for all modes except Search
		AppList::getInstance().processEvent(event);
	}

	if (event.type == cpp3ds::Event::KeyPressed)
	{
		int index = AppList::getInstance().getSelectedIndex();

		//Dat secret block of code
		m_keyHistory.push_back(event.key.code);
		if (m_keyHistory.size() > 10) {
			m_keyHistory.erase(m_keyHistory.begin());
		}
		if (m_keyHistory.size() >= 10) {
			if (m_keyHistory[0] == cpp3ds::Keyboard::DPadUp && m_keyHistory[1] == cpp3ds::Keyboard::DPadUp && m_keyHistory[2] == cpp3ds::Keyboard::DPadDown && m_keyHistory[3] == cpp3ds::Keyboard::DPadDown && m_keyHistory[4] == cpp3ds::Keyboard::DPadLeft && m_keyHistory[5] == cpp3ds::Keyboard::DPadRight && m_keyHistory[6] == cpp3ds::Keyboard::DPadLeft && m_keyHistory[7] == cpp3ds::Keyboard::DPadRight && m_keyHistory[8] == cpp3ds::Keyboard::B && m_keyHistory[9] == cpp3ds::Keyboard::A) {
				if (!Config::get(Config::Skiddo).GetBool()) {				
					Notification::spawn(_("Skiddo!"));
					Config::set(Config::Skiddo, true);
					m_settingsGUI->addSkiddoLanguage();
				} else {
#ifndef NDEBUG
					Notification::spawn(_("freeShop " FREESHOP_VERSION " / " __DATE__ " " __TIME__));
#endif
				}
			}
		}

		switch (event.key.code)
		{
			case cpp3ds::Keyboard::Select:
				requestStackClear();
				return true;
			case cpp3ds::Keyboard::A:
			{
				// Don't load if game info is already loaded or nothing is selected
				if (!AppList::getInstance().getSelected())
					break;
				if (m_appInfo.getAppItem() == AppList::getInstance().getSelected()->getAppItem())
					break;

				m_threadBusy = true;
				// Only fade out if a game is loaded already
				if (!m_appInfo.getAppItem())
					m_threadLoadApp.launch();
				else
					TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.3f)
						.target(0.f)
						.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
							m_threadLoadApp.launch();
						})
						.start(m_tweenManager);
				break;
			}
			case cpp3ds::Keyboard::B:
				AppList::getInstance().filterBySearch("", m_textMatches);
				break;
			case cpp3ds::Keyboard::X: {
				if (!AppList::getInstance().getSelected())
					break;
				auto app = AppList::getInstance().getSelected()->getAppItem();
				if (app && !DownloadQueue::getInstance().isDownloading(app))
				{
					if (!app->isInstalled())
					{
						app->queueForInstall();
						Notification::spawn(_("Queued install: %s", app->getTitle().toAnsiString().c_str()));
					}
					else
						Notification::spawn(_("Already installed: %s", app->getTitle().toAnsiString().c_str()));
				} else {
					Notification::spawn(_("Already in downloading: \n%s", app->getTitle().toAnsiString().c_str()));
				}
				break;
			}
			case cpp3ds::Keyboard::Y: {
				if (!AppList::getInstance().getSelected())
					break;
				auto app = AppList::getInstance().getSelected()->getAppItem();
				bool isSleepDownloading = false;
#ifndef EMULATION
				NIMS_IsTaskRegistered(app->getTitleId(), &isSleepDownloading);
#endif
				if (app && !DownloadQueue::getInstance().isDownloading(app) && !isSleepDownloading)
				{
					if (!app->isInstalled())
					{
						app->queueForSleepInstall();
					}
					else
						Notification::spawn(_("Already installed: %s", app->getTitle().toAnsiString().c_str()));
				} else if (isSleepDownloading) {
					app->removeSleepInstall();
				} else {
					Notification::spawn(_("Already in downloading: \n%s", app->getTitle().toAnsiString().c_str()));
				}
				break;
			}
			case cpp3ds::Keyboard::CStickRight: {
				if (getMode() < 5) {
					int newMode = getMode() + 1;
					setMode(static_cast<Mode>(newMode));
					m_iconSet.setSelectedIndex(newMode);
				}
				break;
			}
			case cpp3ds::Keyboard::CStickLeft: {
				if (getMode() > 0) {
					int newMode = getMode() - 1;
					setMode(static_cast<Mode>(newMode));
					m_iconSet.setSelectedIndex(newMode);
				}
				break;
			}
			default:
				break;
		}
	}

	return true;
}


void BrowseState::loadApp()
{
	auto item = AppList::getInstance().getSelected()->getAppItem();
	if (!item)
		return;

	// TODO: Don't show loading when game is cached?
	bool showLoading = g_browserLoaded && m_appInfo.getAppItem() != item;

	m_iconSet.setSelectedIndex(App);
	if (m_appInfo.getAppItem() != item)
	{
		setMode(App);

		if (showLoading)
			requestStackPush(States::Loading);

		m_appInfo.loadApp(item);
	}

	TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.5f)
		.target(255.f)
		.start(m_tweenManager);

	if (showLoading)
		requestStackPop();

	m_threadBusy = false;
}


void BrowseState::setMode(BrowseState::Mode mode)
{
	if (m_mode == mode || m_isTransitioning)
		return;

	// Transition / end current mode
	if (m_mode == Search)
	{
		float delay = 0.f;
		for (auto& text : m_textMatches)
		{
			TweenEngine::Tween::to(text, util3ds::RichText::POSITION_X, 0.2f)
				.target(-text.getLocalBounds().width)
				.delay(delay)
				.start(m_tweenManager);
			delay += 0.05f;
		}

		AppList::getInstance().setCollapsed(false);
		m_topInfos.setCollapsed(false);

		TweenEngine::Tween::to(m_iconSet, IconSet::POSITION_X, 0.3f)
					.target(60.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textActiveDownloads, util3ds::TweenText::POSITION_X, 0.3f)
					.target(128.f)
					.start(m_tweenManager);
	}
	else if (m_mode == Settings)
	{
		m_settingsGUI->saveToConfig();
	}

	// Transition / start new mode
	if (mode == Search)
	{
		float posY = 1.f;
		for (auto& text : m_textMatches)
		{
			text.clear();
			text.setPosition(5.f, posY);
			posY += 13.f;
		}
		AppList::getInstance().setCollapsed(true);
		AppList::getInstance().setIndexDelta(0);
		m_topInfos.setCollapsed(true);

		TweenEngine::Tween::to(m_iconSet, IconSet::POSITION_X, 0.3f)
					.target(155.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textActiveDownloads, util3ds::TweenText::POSITION_X, 0.3f)
					.target(223.f)
					.start(m_tweenManager);

		m_lastKeyboardInput = "";
		m_keyboard.setCurrentInput(m_lastKeyboardInput);
	}

	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(255.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_mode = mode;
		})
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_isTransitioning = false;
		})
		.delay(0.4f)
		.start(m_tweenManager);

	m_isTransitioning = true;
}

bool BrowseState::playBGMeShop()
{
	stopBGM();

	int bgmCount = 2;
	int randIndex = (std::rand() % bgmCount) + 1;

	// In case it doesn't find it, loop down until it hopefully does
	for (int i = randIndex; i > 0; --i)
	{
		std::string filePath = fmt::sprintf(FREESHOP_DIR "/music/eshop/boss_bgm%d", i);
		if (m_musicLoop.openFromFile(filePath))
		{
			m_musicLoop.play();
			break;
		}
	}
}

bool BrowseState::playBGM(const std::string &filename)
{
	stopBGM();

	if (m_musicLoopBCSTM.openFromFile(filename))
		m_musicLoopBCSTM.play();
	else if (m_musicLoop.openFromFile(filename))
		m_musicLoop.play();
	else
		return false;

	return true;
}

void BrowseState::stopBGM()
{
	m_musicLoopBCSTM.stop();
	m_musicLoop.stop();
}

void BrowseState::reloadKeyboard()
{
	m_isJapKeyboard = false;
	m_isTIDKeyboard = false;

	//Loading the keyboard locale file
	if (std::string(Config::get(Config::Keyboard).GetString()) == "azerty")
		m_keyboard.loadFromFile("kb/keyboard.azerty.xml");
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "qwertz")
		m_keyboard.loadFromFile("kb/keyboard.qwertz.xml");
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "jap") {
		m_keyboard.loadFromFile("kb/keyboard.jap.xml");
		m_isJapKeyboard = true;
	}
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "tid") {
		m_keyboard.loadFromFile("kb/keyboard.titleid.xml");
		m_isTIDKeyboard = true;
	}
	else
		m_keyboard.loadFromFile("kb/keyboard.qwerty.xml");
}

int BrowseState::getMode()
{
	return m_mode;
}

bool BrowseState::isAppInfoLoaded()
{
	if (!m_appInfo.getAppItem())
		return false;
	else
		return true;
}

bool BrowseState::getJapKeyboard()
{
	return m_isJapKeyboard;
}

bool BrowseState::getTIDKeyboard()
{
	return m_isTIDKeyboard;
}

std::string BrowseState::getCtrSdPath()
{
	return m_ctrSdPath;
}

} // namespace FreeShop
