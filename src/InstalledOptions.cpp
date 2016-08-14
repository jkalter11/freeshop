#include "InstalledOptions.hpp"
#include "InstalledItem.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "InstalledList.hpp"
#include <cpp3ds/System/I18n.hpp>

namespace FreeShop {

InstalledOptions::InstalledOptions()
: m_installedItem(nullptr)
{
	m_textGame.setString(_("Game"));
	m_textGame.setFillColor(cpp3ds::Color(100, 100, 100));
	m_textGame.setCharacterSize(10);
	m_textGame.setPosition(20.f, 0.f);
	m_textGame.useSystemFont();

	m_textUpdates = m_textGame;
	m_textUpdates.setString(_("Updates"));
	m_textUpdates.move(m_textGame.getLocalBounds().width + 40.f, 0.f);

	m_textDLC = m_textUpdates;
	m_textDLC.setString(_("DLC"));
	m_textDLC.move(m_textUpdates.getLocalBounds().width + 40.f, 0.f);

	m_textIconGame.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textIconGame.setString(L"\uf1f8");
	m_textIconGame.setFillColor(cpp3ds::Color(50, 100, 50));
	m_textIconGame.setCharacterSize(18);
	m_textIconGame.setPosition(m_textGame.getPosition().x + m_textGame.getLocalBounds().width + 5.f, -5.f);

	m_textIconUpdates = m_textIconGame;
	m_textIconUpdates.setPosition(m_textUpdates.getPosition().x + m_textUpdates.getLocalBounds().width + 5.f, -5.f);

	m_textIconDLC = m_textIconGame;
	m_textIconDLC.setPosition(m_textDLC.getPosition().x + m_textDLC.getLocalBounds().width + 5.f, -5.f);

}

void InstalledOptions::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();
	target.draw(m_textGame, states);
	target.draw(m_textIconGame, states);

	if (m_updatesAvailable)
	{
		target.draw(m_textUpdates, states);
		target.draw(m_textIconUpdates, states);
	}
	if (m_dlcAvailable)
	{
		target.draw(m_textDLC, states);
		target.draw(m_textIconDLC, states);
	}
}


#define SET_ALPHA(obj, alpha) \
	color = obj.getFillColor(); \
	color.a = alpha; \
	obj.setFillColor(color);

void InstalledOptions::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case ALPHA: {
			cpp3ds::Color color;
			auto alpha = static_cast<cpp3ds::Uint8>(newValues[0]);
			SET_ALPHA(m_textGame, alpha);
			SET_ALPHA(m_textUpdates, alpha);
			SET_ALPHA(m_textDLC, alpha);
			SET_ALPHA(m_textIconGame, alpha);
			SET_ALPHA(m_textIconUpdates, alpha);
			SET_ALPHA(m_textIconDLC, alpha);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int InstalledOptions::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case ALPHA:
			returnValues[0] = m_textGame.getFillColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void InstalledOptions::processTouchEvent(const cpp3ds::Event &event)
{
	cpp3ds::String appTitle = m_installedItem->getAppItem()->getTitle();
	cpp3ds::Vector2f touchPos(event.touch.x - getPosition().x, event.touch.y - getPosition().y);
	if (m_textIconGame.getGlobalBounds().contains(touchPos))
	{
		cpp3ds::Uint64 titleId = m_installedItem->getTitleId();
#ifdef _3DS
		FS_MediaType mediaType = ((titleId >> 32) & 0x8010) != 0 ? MEDIATYPE_NAND : MEDIATYPE_SD;
		AM_DeleteTitle(m_mediaType, titleId);
#endif
		m_installedItem->getAppItem()->setInstalled(false);
		InstalledList::getInstance().refresh();
	}
	else if (m_textIconUpdates.getGlobalBounds().contains(touchPos))
	{
		if (m_updatesInstalled)
		{
			for (auto &id : m_installedItem->getAppItem()->getUpdates()) {
#ifdef _3DS
				AM_DeleteTitle(m_mediaType, id);
#endif
				m_installedItem->setUpdateStatus(id, false);
			}
			m_updatesInstalled = false;
			appTitle.insert(0, _("Deleted update: "));
		}
		else
		{
			for (auto &id : m_installedItem->getAppItem()->getUpdates())
				if (!m_installedItem->getUpdateStatus(id))
					DownloadQueue::getInstance().addDownload(m_installedItem->getAppItem(), id, [=](bool succeeded){
						m_installedItem->setUpdateStatus(id, succeeded);
						m_updatesAvailable = true;
						update();
					});
			appTitle.insert(0, _("Queued update: "));
			m_updatesAvailable = false;
		}
		Notification::spawn(appTitle);
		update();
	}
	else if (m_textIconDLC.getGlobalBounds().contains(touchPos))
	{
		if (m_dlcInstalled)
		{
			for (auto &id : m_installedItem->getAppItem()->getDLC()) {
#ifdef _3DS
				AM_DeleteTitle(m_mediaType, id);
#endif
				m_installedItem->setDLCStatus(id, false);
			}
			m_dlcInstalled = false;
			appTitle.insert(0, _("Deleted DLC: "));
		}
		else
		{
			for (auto &id : m_installedItem->getAppItem()->getDLC())
				if (!m_installedItem->getDLCStatus(id))
					DownloadQueue::getInstance().addDownload(m_installedItem->getAppItem(), id, [=](bool succeeded){
						m_installedItem->setDLCStatus(id, succeeded);
						m_dlcAvailable = true;
						update();
					});
			appTitle.insert(0, _("Queued DLC: "));
			m_dlcAvailable = false;
		}
		Notification::spawn(appTitle);
		update();
	}
}

void InstalledOptions::setInstalledItem(InstalledItem *installedItem)
{
	m_installedItem = installedItem;
	m_updatesAvailable = installedItem->getUpdates().size() > 0;
	m_dlcAvailable = installedItem->getDLC().size() > 0;
	for (auto &id : m_installedItem->getAppItem()->getUpdates())
		if (DownloadQueue::getInstance().isDownloading(id))
			m_updatesAvailable = false;
	for (auto &id : m_installedItem->getAppItem()->getDLC())
		if (DownloadQueue::getInstance().isDownloading(id))
			m_dlcAvailable = false;
	update();
#ifdef _3DS
	m_mediaType = ((installedItem->getTitleId() >> 32) & 0x8010) != 0 ? MEDIATYPE_NAND : MEDIATYPE_SD;
#endif
}

InstalledItem *InstalledOptions::getInstalledItem() const
{
	return m_installedItem;
}

void InstalledOptions::update()
{
	m_updatesInstalled = true;
	for (auto &update : m_installedItem->getUpdates())
		if (!update.second)
		{
			m_updatesInstalled = false;
			break;
		}
	m_dlcInstalled = true;
	for (auto &dlc : m_installedItem->getDLC())
		if (!dlc.second)
		{
			m_dlcInstalled = false;
			break;
		}

	m_textIconUpdates.setString(m_updatesInstalled ? L"\uf1f8" : L"\uf019");
	m_textIconDLC.setString(m_dlcInstalled ? L"\uf1f8" : L"\uf019");
}


} // namespace FreeShop
