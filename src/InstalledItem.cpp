#include "InstalledItem.hpp"
#include "AssetManager.hpp"
#include "AppList.hpp"

namespace FreeShop {

InstalledItem::InstalledItem(cpp3ds::Uint64 titleId)
: m_titleId(titleId)
{
	bool found = false;
	for (auto& appItem : AppList::getInstance().getList())
	{
		auto item = appItem->getAppItem();
		if (titleId == item->getTitleId())
		{
			found = true;
			m_textTitle.setString(item->getTitle());
		}
	}

	if (!found)
		throw 0;

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
	m_textDelete.setCharacterSize(15);
	m_textDelete.setFillColor(cpp3ds::Color::Black);
	m_textDelete.setOrigin(0, m_textDelete.getLocalBounds().top + m_textDelete.getLocalBounds().height / 2.f);
	m_textDelete.setPosition(m_background.getSize().x - paddingRight - m_textDelete.getLocalBounds().width,
	                         m_background.getPadding().top + m_background.getContentSize().y / 2.f);

	m_textView = m_textDelete;
	m_textView.setString(L"\uf06e");
	m_textView.move(-20.f, 0);
}

void InstalledItem::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	target.draw(m_background, states);

	target.draw(m_textTitle, states);

	target.draw(m_textView, states);
	target.draw(m_textDelete, states);
}

} // namespace FreeShop
