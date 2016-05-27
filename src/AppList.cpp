#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <functional>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <TweenEngine/Tween.h>
#include <fstream>
#include <cpp3ds/System/Clock.hpp>
#include "AppList.hpp"
#include "Util.hpp"


namespace FreeShop {

AppList::AppList(std::string jsonFilename)
: m_sortType(AlphaNumericAsc)
, m_selectedIndex(0)
, m_collapsed(false)
{
//	cpp3ds::Clock clock;
	m_jsonFilename = jsonFilename;
	refresh();
//	std::cout << "applist:" << clock.getElapsedTime().asSeconds();
}

AppList::~AppList()
{
	for (auto& texture : m_iconTextures)
		delete texture;
}

void AppList::refresh()
{
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
			std::unique_ptr<AppItem> item(new AppItem());
			// Move offscreen to avoid everything being drawn at once and crashing
			item->setPosition(500.f, 100.f);
			item->loadFromJSON(iter->name.GetString(), iter->value);
			m_list.emplace_back(std::move(item));
		}
	}

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
	std::sort(m_list.begin(), m_list.end(), [&](const std::unique_ptr<AppItem>& a, const std::unique_ptr<AppItem>& b)
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
					return a->getTitle() > b->getTitle();
				default:
					return a->getTitle() < b->getTitle();
			}
		}
	});
}

void AppList::resize()
{
	float posY = 4.f;
	int i = 0;

	for (auto& app : m_list)
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
			TweenEngine::Tween::to(*app, AppItem::POSITION_XY, 0.3f)
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
	m_list[m_selectedIndex]->deselect();
	m_selectedIndex = index;
	m_list[m_selectedIndex]->select();
}

int AppList::getSelectedIndex() const
{
	return m_selectedIndex;
}

AppItem *AppList::getSelected()
{
	return m_list[m_selectedIndex].get();
}

void AppList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& app : m_list)
	{
		if (app->isVisible())
			target.draw(*app, states);
	}
}

size_t AppList::getCount() const
{
	return m_list.size();
}

size_t AppList::getVisibleCount() const
{
	size_t count = 0;
	for (auto& item : m_list)
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
		for (auto &app : m_list)
		{
			TweenEngine::Tween::to(*app, AppItem::INFO_ALPHA, 0.3f)
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
				for (auto &app : m_list) {
					app->setInfoVisible(true);
					TweenEngine::Tween::to(*app, AppItem::INFO_ALPHA, 0.3f)
						.target(255.f)
						.start(m_tweenManager);
				}
				TweenEngine::Tween::to(*getSelected(), AppItem::BACKGROUND_ALPHA, 0.3f)
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
	for (auto& item : m_list)
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
			AppItem* itemptr = item.get();
			TweenEngine::Tween::to(*item, SCALE_XY, 0.3)
				.target(0.f, 0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [itemptr](TweenEngine::BaseTween *source) {
					itemptr->setVisible(false);
				})
				.start(m_tweenManager);
		}
	}

	m_list[m_selectedIndex]->deselect();
	sort();
	resize();

	int i = 0;
	for (auto& textMatch : textMatches)
	{
		textMatch.clear();
		if (searchTerm.empty())
			continue;

		auto item = m_list[i].get();
		if (item->getMatchScore() > -99)
		{
			bool matching = false;
			const char *str = item->getNormalizedTitle().c_str();
			const char *pattern = searchTerm.c_str();
			const char *strLastPos = str;

			auto title = item->getTitle().toUtf8();
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

	setSelectedIndex(0);
}

} // namespace FreeShop