#include "LoadingState.hpp"
#include "../AssetManager.hpp"
#include "../Theme.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Service.hpp>
#include <cpp3ds/System/FileSystem.hpp>

namespace FreeShop {

LoadingState::LoadingState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_topBG(false)
, m_botBG(false)
{
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string loadingColorValue = Theme::get("loadingColor").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(loadingColorValue, &R, &G, &B);
			Theme::loadingIcon = cpp3ds::Color(R, G, B);
		}
	}

	m_background.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_background.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_icon.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	if (Theme::isTextThemed)
		m_icon.setFillColor(Theme::loadingIcon);
	else
		m_icon.setFillColor(cpp3ds::Color(110, 110, 110, 255));
	m_icon.setCharacterSize(80);
	m_icon.setString(L"\uf110");
	cpp3ds::FloatRect textRect = m_icon.getLocalBounds();
	m_icon.setOrigin(textRect.left + textRect.width / 2.f, textRect.top + 1.f + textRect.height / 2.f);
	m_icon.setPosition(160.f, 120.f);
	m_icon.setScale(0.5f, 0.5f);

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
}

void LoadingState::renderTopScreen(cpp3ds::Window& window)
{
	if (m_topBG == true)
		window.draw(m_rectTopBG);
}

void LoadingState::renderBottomScreen(cpp3ds::Window& window)
{
	if (m_botBG == true)
		window.draw(m_rectBotBG);

	window.draw(m_icon);
}

bool LoadingState::update(float delta)
{
	if (m_rotateClock.getElapsedTime() > cpp3ds::milliseconds(80))
	{
		m_icon.rotate(45);
		m_rotateClock.restart();
	}
	return true;
}

bool LoadingState::processEvent(const cpp3ds::Event& event)
{
	return false;
}

} // namespace FreeShop
