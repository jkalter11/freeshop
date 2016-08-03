#include <cpp3ds/System/I18n.hpp>
#include "InstalledItem.hpp"
#include "AssetManager.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"

namespace FreeShop {

InstalledItem::InstalledItem(cpp3ds::Uint64 titleId)
: m_titleId(titleId)
, m_updateInstallCount(0)
, m_dlcInstallCount(0)
{
	bool found = false;
	TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);

	for (auto& appItem : AppList::getInstance().getList())
	{
		m_appItem = appItem->getAppItem();
		if (titleId == m_appItem->getTitleId())
		{
			found = true;
			m_textTitle.setString(m_appItem->getTitle());
		}
	}

	if (!found)
	{
		throw 0;
		if (titleType == TitleKeys::Game || titleType == TitleKeys::DSiWare)
			throw 0;
		else
		{
#ifdef _3DS
			char productCode[16];
			AM_TitleEntry titleInfo;
			AM_GetTitleInfo(MEDIATYPE_NAND, 1, &titleId, &titleInfo);
			AM_GetTitleProductCode(MEDIATYPE_NAND, titleId, productCode);
			m_textTitle.setString(productCode);
#endif
		}
	}

	m_background.setTexture(&AssetManager<cpp3ds::Texture>::get("images/installed_item_bg.9.png"));
	m_background.setContentSize(320.f + m_background.getPadding().width - m_background.getTexture()->getSize().x + 2.f, 14.f);
	float paddingRight = m_background.getSize().x - m_background.getContentSize().x - m_background.getPadding().left;
	float paddingBottom = m_background.getSize().y - m_background.getContentSize().y - m_background.getPadding().top;

	m_textTitle.setCharacterSize(11);
	m_textTitle.setPosition(m_background.getPadding().left, m_background.getPadding().top);
	m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.useSystemFont();

	m_textDelete.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textDelete.setString(L"\uf1f8");
	m_textDelete.setCharacterSize(14);
	m_textDelete.setFillColor(cpp3ds::Color::Black);
	m_textDelete.setOrigin(0, m_textDelete.getLocalBounds().top + m_textDelete.getLocalBounds().height / 2.f);
	m_textDelete.setPosition(m_background.getSize().x - paddingRight - m_textDelete.getLocalBounds().width,
	                         m_background.getPadding().top + m_background.getContentSize().y / 2.f);

	m_textView = m_textDelete;
	m_textView.setString(L"\uf06e");
	m_textView.move(-20.f, 0);

	m_textWarningUpdate = m_textDelete;
	m_textWarningUpdate.setString(L"\uf071");
	m_textWarningUpdate.setFillColor(cpp3ds::Color(50, 50, 50));
	m_textWarningUpdate.setOutlineColor(cpp3ds::Color(0, 200, 0));
	m_textWarningUpdate.setOutlineThickness(1.f);
	m_textWarningUpdate.move(-1.f, 0);

	m_textWarningDLC = m_textWarningUpdate;
	m_textWarningDLC.setOutlineColor(cpp3ds::Color(255, 255, 0, 255));
	m_textWarningDLC.move(-20.f, 0);
}

void InstalledItem::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	target.draw(m_background, states);
	target.draw(m_textTitle, states);
	if (m_updates.size() - m_updateInstallCount > 0)
		target.draw(m_textWarningUpdate, states);
	if (m_dlc.size() - m_dlcInstallCount > 0)
		target.draw(m_textWarningDLC, states);
}

cpp3ds::Uint64 InstalledItem::getTitleId() const
{
	return m_titleId;
}

void InstalledItem::setUpdateStatus(cpp3ds::Uint64 titleId, bool installed)
{
	m_updates[titleId] = installed;
	m_updateInstallCount = 0;
	for (auto& update : m_updates)
		if (update.second)
			m_updateInstallCount++;
}

void InstalledItem::setDLCStatus(cpp3ds::Uint64 titleId, bool installed)
{
	m_dlc[titleId] = installed;
	m_dlcInstallCount = 0;
	for (auto& dlc : m_dlc)
		if (dlc.second)
			m_dlcInstallCount++;
}

std::shared_ptr<AppItem> InstalledItem::getAppItem() const
{
	return m_appItem;
}


} // namespace FreeShop
