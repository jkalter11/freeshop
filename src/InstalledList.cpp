#include <cmath>
#include <TweenEngine/Tween.h>
#include "InstalledList.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"

namespace FreeShop {

InstalledList::InstalledList()
: m_scrollPos(0.f)
, m_size(320.f, 0.f)
, m_expandedItem(nullptr)
{
	// Make install options initially transparent for fade in
	TweenEngine::Tween::set(m_options, InstalledOptions::ALPHA)
			.target(0.f).start(m_tweenManager);
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

	m_installedItems.clear();
	m_expandedItem = nullptr;

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
		target.draw(*item, states);

	if (m_expandedItem)
		target.draw(m_options, states);
}

void InstalledList::update(float delta)
{
	m_tweenManager.update(delta);
}

bool InstalledList::processEvent(const cpp3ds::Event &event)
{
	if (m_tweenManager.getRunningTweensCount() > 0)
		return false;

	if (event.type == cpp3ds::Event::TouchEnded)
	{
		for (auto &item : m_installedItems)
		{
			float posY = getPosition().y + item->getPosition().y;
			if (event.touch.y > posY && event.touch.y < posY + item->getHeight())
			{
				if (item.get() == m_expandedItem)
				{
					if (event.touch.y < posY + 16.f)
						expandItem(nullptr);
					else
						m_options.processTouchEvent(event);
				}
				else
					expandItem(item.get());
				break;
			}
		}
	}
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
		if (item.get() == m_expandedItem)
			m_options.setPosition(0.f, posY + 20.f);
		item->setPosition(0.f, posY);
		posY += item->getHeight();
	}
	m_size.y = posY - 6.f - m_scrollPos;
	if (m_expandedItem)
		m_size.y -= 24.f;
	updateScrollSize();
}

const cpp3ds::Vector2f &InstalledList::getScrollSize()
{
	return m_size;
}

void InstalledList::expandItem(InstalledItem *item)
{
	if (item == m_expandedItem)
		return;

	const float optionsFadeDelay = 0.15f;
	const float expandDuration = 0.2f;

	// Expand animation
	if (m_expandedItem)
	{
		TweenEngine::Tween::to(*m_expandedItem, InstalledItem::HEIGHT, expandDuration)
			.target(16.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_expandedItem = item;
				repositionItems();
			})
			.delay(optionsFadeDelay)
			.start(m_tweenManager);
		TweenEngine::Tween::to(m_options, InstalledOptions::ALPHA, optionsFadeDelay + 0.05f)
			.target(0.f)
			.start(m_tweenManager);
	}
	if (item)
	{
		TweenEngine::Tween::to(*item, InstalledItem::HEIGHT, expandDuration)
			.target(40.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_expandedItem = item;
				m_options.setInstalledItem(item);
				repositionItems();
			})
			.delay(m_expandedItem ? optionsFadeDelay : 0.f)
			.start(m_tweenManager);
		TweenEngine::Tween::to(m_options, InstalledOptions::ALPHA, expandDuration)
			.target(255.f)
			.delay(m_expandedItem ? expandDuration + optionsFadeDelay : optionsFadeDelay)
			.start(m_tweenManager);
	}

	// Move animation for items in between expanded items
	bool foundItem = false;
	bool foundExpanded = false;
	for (auto &itemToMove : m_installedItems)
	{
		if (foundItem && !foundExpanded)
		{
			TweenEngine::Tween::to(*itemToMove, InstalledItem::POSITION_Y, expandDuration)
				.targetRelative(24.f)
				.delay(m_expandedItem ? optionsFadeDelay : 0.f)
				.start(m_tweenManager);
		}
		else if (foundExpanded && !foundItem)
		{
			TweenEngine::Tween::to(*itemToMove, InstalledItem::POSITION_Y, expandDuration)
				.targetRelative(-24.f)
				.delay(m_expandedItem ? optionsFadeDelay : 0.f)
				.start(m_tweenManager);
		}

		if (itemToMove.get() == m_expandedItem)
			foundExpanded = true;
		else if (itemToMove.get() == item)
		{
			foundItem = true;
		}

		if (foundExpanded && foundItem)
			break;
	}
}


} // namespace FreeShop
