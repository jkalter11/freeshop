#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <functional>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <TweenEngine/Tween.h>
#include <fstream>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include "AppList.hpp"
#include "Util.hpp"
#include "Installer.hpp"


namespace FreeShop {

AppList::AppList(std::string jsonFilename)
: m_sortType(AlphaNumericAsc)
, m_selectedIndex(-1)
, m_collapsed(false)
{
	m_jsonFilename = jsonFilename;
}

AppList::~AppList()
{
	for (auto& texture : m_iconTextures)
		delete texture;
}

void AppList::refresh()
{
#ifdef EMULATION
	uint8_t isNew3DS = 1;
#else
	uint8_t isNew3DS = 0;
	APT_CheckNew3DS(&isNew3DS);
#endif

	cpp3ds::FileInputStream file;
	if (file.open(m_jsonFilename))
	{
		// Read file to string
		int size = file.getSize();
		std::string json;
		json.resize(size);
		file.read(&json[0], size);

		// Parse json string
		rapidjson::Document doc;
		doc.Parse(json.c_str());
		int i = 0;
		for (rapidjson::Value::ConstMemberIterator iter = doc.MemberBegin(); iter != doc.MemberEnd(); ++iter)
		{
			std::string id = iter->name.GetString();
			cpp3ds::Uint64 titleId = strtoull(id.c_str(), 0, 16);

			if (!isNew3DS && ((titleId >> 24) & 0xF) == 0xF)
				continue;

			if (Installer::titleKeyExists(titleId))
			{
				std::unique_ptr<AppItem> appItem(new AppItem());
				std::unique_ptr<GUI::AppItem> guiAppItem(new GUI::AppItem());

				appItem->loadFromJSON(iter->name.GetString(), iter->value);
				guiAppItem->setAppItem(appItem.get());
				// Move offscreen to avoid everything being drawn at once and crashing
				guiAppItem->setPosition(500.f, 100.f);

				m_appItems.emplace_back(std::move(appItem));
				m_guiAppItems.emplace_back(std::move(guiAppItem));

				cpp3ds::sleep(cpp3ds::microseconds(300));
			}
		}
	}

	if (m_selectedIndex < 0 && getVisibleCount() > 0)
		m_selectedIndex = 0;

	sort();
	resize();
	setSelectedIndex(m_selectedIndex);
}

void AppList::setSortType(AppList::SortType sortType)
{
	m_sortType = sortType;
	sort();
	resize();
}

AppList::SortType AppList::getSortType() const
{
	return m_sortType;
}

void AppList::sort()
{
	std::sort(m_guiAppItems.begin(), m_guiAppItems.end(), [&](const std::unique_ptr<GUI::AppItem>& a, const std::unique_ptr<GUI::AppItem>& b)
	{
		if (a->getMatchScore() != b->getMatchScore())
		{
			return a->getMatchScore() > b->getMatchScore();
		}
		else
		{
			switch(m_sortType)
			{
				case AlphaNumericDesc:
					return a->getAppItem()->getNormalizedTitle() > b->getAppItem()->getNormalizedTitle();
				default:
					return a->getAppItem()->getNormalizedTitle() < b->getAppItem()->getNormalizedTitle();
			}
		}
	});
}

void AppList::resize()
{
	float posY = 4.f;
	int i = 0;

	for (auto& app : m_guiAppItems)
	{
		if (!app->isVisible())
			continue;

		float posX = 3.f + (i/4) * (m_collapsed ? 59.f : 200.f);
		if ((posX < -200.f || posX > 400.f) && (app->getPosition().x < -200 && app->getPosition().y > 400.f))
		{
			app->setPosition(posX, posY);
		}
		else
		{
			TweenEngine::Tween::to(*app, GUI::AppItem::POSITION_XY, 0.3f)
				.target(posX, posY)
				.start(m_tweenManager);
		}

		if (++i % 4 == 0)
			posY = 4.f;
		else
			posY += 59.f;
	}
}

void AppList::setSelectedIndex(int index)
{
	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->deselect();
	m_selectedIndex = index;
	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->select();
}

int AppList::getSelectedIndex() const
{
	return m_selectedIndex;
}

GUI::AppItem *AppList::getSelected()
{
	if (m_selectedIndex < 0 || m_selectedIndex > m_guiAppItems.size()-1)
		return nullptr;
	return m_guiAppItems[m_selectedIndex].get();
}

void AppList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& app : m_guiAppItems)
	{
		if (app->isVisible())
			target.draw(*app, states);
	}
}

size_t AppList::getCount() const
{
	return m_guiAppItems.size();
}

size_t AppList::getVisibleCount() const
{
	size_t count = 0;
	for (auto& item : m_guiAppItems)
		if (item->isVisible())
			count++;
		else break;

	return count;
}

void AppList::setCollapsed(bool collapsed)
{
	if (m_collapsed == collapsed)
		return;

	m_tweenManager.killAll();
	float newX = m_selectedIndex / 4 * (collapsed ? 59.f : 200.f);

	if (collapsed)
	{
		for (auto &app : m_guiAppItems)
		{
			TweenEngine::Tween::to(*app, GUI::AppItem::INFO_ALPHA, 0.3f)
				.target(0)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [&](TweenEngine::BaseTween *source) {
					app->setInfoVisible(false);
				})
				.start(m_tweenManager);
		}
		TweenEngine::Tween::to(*this, AppList::POSITION_X, 0.3f)
			.target(-newX)
			.delay(0.3f)
			.setCallback(TweenEngine::TweenCallback::START, [=](TweenEngine::BaseTween *source) {
				m_collapsed = collapsed;
				resize();
			})
			.start(m_tweenManager);
	}
	else
	{
		m_collapsed = collapsed;
		resize();
		TweenEngine::Tween::to(*this, AppList::POSITION_X, 0.3f)
			.target(-newX)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween *source) {
				for (auto &app : m_guiAppItems) {
					app->setInfoVisible(true);
					TweenEngine::Tween::to(*app, GUI::AppItem::INFO_ALPHA, 0.3f)
						.target(255.f)
						.start(m_tweenManager);
				}

				GUI::AppItem *item = getSelected();
				if (item)
					TweenEngine::Tween::to(*item, GUI::AppItem::BACKGROUND_ALPHA, 0.3f)
						.target(255.f)
						.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween *source) {
							setSelectedIndex(m_selectedIndex);
						})
						.delay(0.3f)
						.start(m_tweenManager);
			})
			.start(m_tweenManager);
	}
}

bool AppList::isCollapsed() const
{
	return m_collapsed;
}

void AppList::update(float delta)
{
	m_tweenManager.update(delta);
}

void AppList::filterBySearch(const std::string &searchTerm, std::vector<util3ds::RichText> &textMatches)
{
	m_tweenManager.killAll();
	for (auto& item : m_guiAppItems)
	{
		item->setMatchTerm(searchTerm);
		if (item->getMatchScore() > -99)
		{
			item->setVisible(true);
			TweenEngine::Tween::to(*item, SCALE_XY, 0.3)
				.target(1.f, 1.f)
				.start(m_tweenManager);
		}
		else{
			GUI::AppItem* itemptr = item.get();
			TweenEngine::Tween::to(*item, SCALE_XY, 0.3)
				.target(0.f, 0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [itemptr](TweenEngine::BaseTween *source) {
					itemptr->setVisible(false);
				})
				.start(m_tweenManager);
		}
	}

	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->deselect();
	sort();
	resize();

	int i = 0;
	for (auto& textMatch : textMatches)
	{
		textMatch.clear();
		if (searchTerm.empty())
			continue;

		if (i < getVisibleCount())
		{
			auto item = m_guiAppItems[i].get();
			if (item->getMatchScore() > -99)
			{
				bool matching = false;
				const char *str = item->getAppItem()->getNormalizedTitle().c_str();
				const char *pattern = searchTerm.c_str();
				const char *strLastPos = str;

				auto title = item->getAppItem()->getTitle().toUtf8();
				auto titleCurPos = title.begin();
				auto titleLastPos = title.begin();

				textMatch << cpp3ds::Color(150,150,150);
				while (*str != '\0')  {
					if (tolower(*pattern) == tolower(*str)) {
						if (!matching) {
							matching = true;
							if (str != strLastPos) {
								textMatch << cpp3ds::String::fromUtf8(titleLastPos, titleCurPos);
								titleLastPos = titleCurPos;
								strLastPos = str;
							}
							textMatch << cpp3ds::Color::Black;
						}
						++pattern;
					} else {
						if (matching) {
							matching = false;
							if (str != strLastPos) {
								textMatch << cpp3ds::String::fromUtf8(titleLastPos, titleCurPos);
								titleLastPos = titleCurPos;
								strLastPos = str;
							}
							textMatch << cpp3ds::Color(150,150,150);
						}
					}
					++str;
					titleCurPos = cpp3ds::Utf8::next(titleCurPos, title.end());
				}
				if (str != strLastPos)
					textMatch << cpp3ds::String::fromUtf8(titleLastPos, title.end());
			}
			i++;
		}
	}

	if (m_guiAppItems.size() > 0 && textMatches.size() > 0)
		setSelectedIndex(0);
}

std::vector<std::unique_ptr<GUI::AppItem>> &AppList::getList()
{
	return m_guiAppItems;
}

AppList &AppList::getInstance()
{
	static AppList list("sdmc:/freeShop/cache/data.json");
	return list;
}

} // namespace FreeShop