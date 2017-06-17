#include "LoadInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "Util.hpp"
#include "States/StateIdentifiers.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>

namespace FreeShop {

LoadInformations::LoadInformations()
: m_loadingPercentage(0)
, m_isTopBGThemeAllowed(false)
, m_isBotBGThemeAllowed(false)
{
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string percentageColorValue = Theme::get("percentageText").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(percentageColorValue, &R, &G, &B);
			Theme::percentageText = cpp3ds::Color(R, G, B);
		}
	}

	//Texts
	m_textLoadingPercentage.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textLoadingPercentage.setFillColor(Theme::percentageText);
	else
		m_textLoadingPercentage.setFillColor(cpp3ds::Color::Black);
	m_textLoadingPercentage.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textLoadingPercentage.setOutlineThickness(2.f);
	m_textLoadingPercentage.setPosition(160.f, 230.f);
	m_textLoadingPercentage.useSystemFont();
	m_textLoadingPercentage.setString(_("0%%"));
}

LoadInformations::~LoadInformations()
{

}

void LoadInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	if (m_loadingPercentage > 0)
		target.draw(m_textLoadingPercentage);
}

void LoadInformations::update(float delta)
{
	m_tweenManager.update(delta);
}

void LoadInformations::updateLoadingPercentage(int newPercentage)
{
	if (newPercentage <= 0)
		m_loadingPercentage = 0;
	else if (newPercentage >= 100)
		m_loadingPercentage = 100;
	else
		m_loadingPercentage = newPercentage;

	m_textLoadingPercentage.setString(_("%i%%", m_loadingPercentage));
	cpp3ds::FloatRect rect = m_textLoadingPercentage.getLocalBounds();
	m_textLoadingPercentage.setOrigin(rect.width / 2.f, rect.height / 2.f);
}

LoadInformations &LoadInformations::getInstance()
{
	static LoadInformations loadInfos;
	return loadInfos;
}

void LoadInformations::reset()
{
	updateLoadingPercentage(0);

	m_isBotBGThemeAllowed = false;
	m_isTopBGThemeAllowed = false;
}

} // namespace FreeShop
