#include <cmath>
#include "InstalledList.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"

namespace FreeShop {

InstalledList::InstalledList()
: m_scrollPos(0.f)
, m_size(320.f, 0.f)
{
	//
}

InstalledList &InstalledList::getInstance()
{
	static InstalledList installedList;
	return installedList;
}

void InstalledList::refresh()
{
	cpp3ds::Uint64 relatedTitleId;
	std::vector<cpp3ds::Uint64> titleIds;
#ifdef EMULATION
	// some hardcoded title IDs for testing
	titleIds.push_back(0x00040000000edf00); // [US] Super Smash Bros.
	titleIds.push_back(0x0004000000030100); // [US] Kid Icarus: Uprising
	titleIds.push_back(0x0004000000030800); // [US] Mario Kart 7
	titleIds.push_back(0x0004000000041700); // [US] Kirby's Dream Land
	titleIds.push_back(0x0004008000008f00);
#else
	u32 titleCount;
	AM_GetTitleCount(MEDIATYPE_SD, &titleCount);
	titleIds.resize(titleCount);
	AM_GetTitleList(nullptr, MEDIATYPE_SD, titleCount, &titleIds[0]);
	AM_GetTitleCount(MEDIATYPE_NAND, &titleCount);
	titleIds.resize(titleCount + titleIds.size());
	AM_GetTitleList(nullptr, MEDIATYPE_NAND, titleCount, &titleIds[titleIds.size() - titleCount]);
#endif

	for (auto& titleId : titleIds)
	{
		TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);
		cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
		relatedTitleId = 0;

		if (titleType == TitleKeys::DLC || titleType == TitleKeys::Update)
		{
			relatedTitleId = (static_cast<cpp3ds::Uint64>(titleType) << 32) | (titleLower << 8);
		}
		for (auto& appItemGUI : AppList::getInstance().getList())
		{
			auto appItem = appItemGUI->getAppItem();
			if (appItem->getTitleId() == (relatedTitleId ? relatedTitleId : titleId))
			{
				appItem->setInstalled(true);
			}
		}
	}

	// Add all primary game titles first
	for (auto& titleId : titleIds)
	{
		TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);
		if (titleType == TitleKeys::Game || titleType == TitleKeys::DSiWare)
			try
			{
				std::unique_ptr<InstalledItem> item(new InstalledItem(titleId));
				m_installedItems.emplace_back(std::move(item));
			}
			catch (int e)
			{
				//
			}
	}
	repositionItems();

	// Add updates that have not yet been installed for which we have a titlekey
	for (auto& titleKey : TitleKeys::getList())
	{
		size_t titleType = titleKey.first >> 32;
		if (titleType == TitleKeys::Update || titleType == TitleKeys::DLC)
		{
			size_t titleLower = (titleKey.first & 0xFFFFFFFF) >> 8;
			for (auto& installedItem : m_installedItems)
			{
				if (titleLower == (installedItem->getTitleId() & 0xFFFFFFFF) >> 8)
				{
					if (titleType == TitleKeys::Update)
						installedItem->setUpdateStatus(titleKey.first, false);
					else
						installedItem->setDLCStatus(titleKey.first, false);
				}
			}
		}
	}

	// Add all installed updates/DLC to the above titles added.
	// If not found, attempt to fetch parent title info from system.
	for (auto& titleId : titleIds)
	{
		TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);
		if (titleType == TitleKeys::Update || titleType == TitleKeys::DLC)
		{
			bool found = false;
			cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
			for (auto& installedItem : m_installedItems)
			{
				if (titleLower == (installedItem->getTitleId() & 0xFFFFFFFF) >> 8)
				{
					found = true;
					if (titleType == TitleKeys::Update)
						installedItem->setUpdateStatus(titleId, true);
					else
						installedItem->setDLCStatus(titleId, true);
					break;
				}
			}
		}
	}
}

void InstalledList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();
	states.scissor = cpp3ds::UintRect(0, 30, 320, 210);

	for (auto& item : m_installedItems)
	{
		target.draw(*item, states);
	}
}

void InstalledList::update(float delta)
{
	m_tweenManager.update(delta);
}

bool InstalledList::processEvent(const cpp3ds::Event &event)
{
	return false;
}

void InstalledList::setScroll(float position)
{
	m_scrollPos = std::round(position);
	repositionItems();
}

float InstalledList::getScroll()
{
	return m_scrollPos;
}

void InstalledList::repositionItems()
{
	float posY = 30.f + m_scrollPos;
	for (auto& item : m_installedItems)
	{
		item->setPosition(0.f, posY);
		posY += 16.f*4;
	}

	m_size.y = posY - 30.f;
}

const cpp3ds::Vector2f &InstalledList::getScrollSize()
{
	return m_size;
}


} // namespace FreeShop
