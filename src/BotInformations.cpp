#include "BotInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <time.h>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

BotInformations::BotInformations()
{
	//Texts
	m_textSD.setString(_("SD"));
	if (Theme::isTextThemed)
		m_textSD.setFillColor(Theme::primaryTextColor);
	else
		m_textSD.setFillColor(cpp3ds::Color::Black);
	m_textSD.setCharacterSize(10);
	m_textSD.setPosition(2.f, 31.f);
	m_textSD.useSystemFont();

	m_textSDStorage.setString(_(""));
	if (Theme::isTextThemed)
		m_textSDStorage.setFillColor(Theme::secondaryTextColor);
	else
		m_textSDStorage.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textSDStorage.setCharacterSize(10);
	m_textSDStorage.setPosition(2.f, 43.f);
	m_textSDStorage.useSystemFont();

	m_textNAND.setString(_("TWL NAND"));
	if (Theme::isTextThemed)
		m_textNAND.setFillColor(Theme::primaryTextColor);
	else
		m_textNAND.setFillColor(cpp3ds::Color::Black);
	m_textNAND.setCharacterSize(10);
	m_textNAND.setPosition(2.f, 59.f);
	m_textNAND.useSystemFont();

	m_textNANDStorage.setString(_(""));
	if (Theme::isTextThemed)
		m_textNANDStorage.setFillColor(Theme::secondaryTextColor);
	else
		m_textNANDStorage.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textNANDStorage.setCharacterSize(10);
	m_textNANDStorage.setPosition(2.f, 71.f);
	m_textNANDStorage.useSystemFont();

	//Progress bars
	m_progressBarNAND.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_progressBarNAND.setPosition(0.f, 58.f);
	m_progressBarNAND.setSize(cpp3ds::Vector2f(0, 29));

	m_progressBarSD.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_progressBarSD.setPosition(0.f, 30.f);
	m_progressBarSD.setSize(cpp3ds::Vector2f(0, 29));

	//Progress bars' backgrounds
	if (Theme::isFSBGNAND9Themed)
		m_backgroundNAND.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/fsbgnand.9.png"));
	else
		m_backgroundNAND.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fsbgnand.9.png"));

	m_backgroundNAND.setSize(320, 23);
	m_backgroundNAND.setPosition(0.f, 58.f);

	if (Theme::isFSBGSD9Themed)
		m_backgroundSD.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/fsbgsd.9.png"));
	else
		m_backgroundSD.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fsbgsd.9.png"));

	m_backgroundSD.setSize(320, 23);
	m_backgroundSD.setPosition(0.f, 30.f);
}

BotInformations::~BotInformations()
{

}

void BotInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Firstly, draw the backgrounds for progress bars
	target.draw(m_backgroundNAND);
	target.draw(m_backgroundSD);

	//Secondly, draw the progress bars
	target.draw(m_progressBarNAND);
	target.draw(m_progressBarSD);

	//Finally, draw all the texts
	target.draw(m_textSD);
	target.draw(m_textSDStorage);
	target.draw(m_textNAND);
	target.draw(m_textNANDStorage);
}

void BotInformations::update()
{
	//Update filesystems size and their progress bars
#ifndef EMULATION
	FS_ArchiveResource resource = {0};

	if(R_SUCCEEDED(FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_SD))) {
		u64 size = (u64) resource.freeClusters * (u64) resource.clusterSize;
		u64 totalSize = (u64) resource.totalClusters * (u64) resource.clusterSize;

		u64 usedSize = totalSize - size;

		m_progressBarSD.setSize(cpp3ds::Vector2f((usedSize * 320) / totalSize, 26));

		if (usedSize > 1024 * 1024 * 1024 || totalSize > 1024 * 1024 * 1024)
			m_textSDStorage.setString(_("%.1f/%.1f GB", static_cast<float>(usedSize) / 1024.f / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f / 1024.f));
		else if (usedSize > 1024 * 1024 || totalSize > 1024 * 1024)
			m_textSDStorage.setString(_("%.1f/%.1f MB", static_cast<float>(usedSize) / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f));
		else
			m_textSDStorage.setString(_("%d/%d KB", usedSize / 1024, totalSize / 1024));
	} else {
		m_textSDStorage.setString("No SD Card detected");
	}

	if(R_SUCCEEDED(FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_TWL_NAND))) {
		u64 size = (u64) resource.freeClusters * (u64) resource.clusterSize;
		u64 totalSize = (u64) resource.totalClusters * (u64) resource.clusterSize;

		u64 usedSize = totalSize - size;

		m_progressBarNAND.setSize(cpp3ds::Vector2f((usedSize * 320) / totalSize, 26));

		if (usedSize > 1024 * 1024 * 1024 || totalSize > 1024 * 1024 * 1024)
			m_textNANDStorage.setString(_("%.1f/%.1f GB", static_cast<float>(usedSize) / 1024.f / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f / 1024.f));
		else if (usedSize > 1024 * 1024 || totalSize > 1024 * 1024)
			m_textNANDStorage.setString(_("%.1f/%.1f MB", static_cast<float>(usedSize) / 1024.f / 1024.f, static_cast<float>(totalSize) / 1024.f / 1024.f));
		else
			m_textNANDStorage.setString(_("%d/%d KB", usedSize / 1024, totalSize / 1024));
	} else {
		m_textNANDStorage.setString("No TWL NAND detected... Wait what ?!");
	}
#else
	m_textSDStorage.setString("16/32 GB");
	m_progressBarSD.setSize(cpp3ds::Vector2f(320, 26));

	m_textNANDStorage.setString("200/400 MB");
	m_progressBarNAND.setSize(cpp3ds::Vector2f(160, 26));
#endif
}

} // namespace FreeShop
