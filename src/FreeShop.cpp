#include <sys/stat.h>
#include "FreeShop.hpp"
#include "States/TitleState.hpp"
#include "Notification.hpp"
#include "States/LoadingState.hpp"
#include "States/SyncState.hpp"
#include "States/BrowseState.hpp"
#include "Config.hpp"
#include "Util.hpp"
#include "States/SleepState.hpp"


using namespace cpp3ds;
using namespace TweenEngine;

namespace FreeShop {


FreeShop::FreeShop()
: Game(0x100000)
, m_stateStack(State::Context(m_text, m_data))
{
	m_stateStack.registerState<TitleState>(States::Title);
	m_stateStack.registerState<LoadingState>(States::Loading);
	m_stateStack.registerState<SyncState>(States::Sync);
	m_stateStack.registerState<BrowseState>(States::Browse);
	m_stateStack.registerState<SleepState>(States::Sleep);
//	m_stateStack.pushState(States::Browse);
	m_stateStack.pushState(States::Loading);
	m_stateStack.pushState(States::Sync);
	m_stateStack.pushState(States::Title);

	textFPS.setFillColor(cpp3ds::Color::Red);
	textFPS.setCharacterSize(20);

	// Set up directory structure, if necessary
	std::string path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop/tmp");
	if (pathExists(path.c_str(), false))
		removeDirectory(path.c_str());
	mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop/cache");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	Config::loadFromFile();
}

FreeShop::~FreeShop()
{
	// Destructor called when game exits
}

void FreeShop::update(float delta)
{
	// Need to update before checking if empty
	m_stateStack.update(delta);
	if (m_stateStack.isEmpty())
		exit();

	Notification::update(delta);

#ifndef NDEBUG
	static int i;
	if (i++ % 10 == 0) {
		textFPS.setString(_("%.1f fps", 1.f / delta));
		textFPS.setPosition(395 - textFPS.getGlobalBounds().width, 2.f);
	}
#endif
}

void FreeShop::processEvent(Event& event)
{
	m_stateStack.processEvent(event);
}

void FreeShop::renderTopScreen(Window& window)
{
	window.clear(Color::White);
	m_stateStack.renderTopScreen(window);
	for (auto& notification : Notification::notifications)
		window.draw(*notification);

#ifndef NDEBUG
	window.draw(textFPS);
#endif
}

void FreeShop::renderBottomScreen(Window& window)
{
	window.clear(Color::White);
	m_stateStack.renderBottomScreen(window);
}


} // namespace FreeShop
