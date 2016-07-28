#include <cpp3ds/System/I18n.hpp>
#include <Gwen/Skins/TexturedBase.h>
#include <Gwen/Controls/RadioButtonController.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <unistd.h>
#include <bits/basic_string.h>
#include <cpp3ds/System/FileSystem.hpp>
#include "Settings.hpp"
#include "../Download.hpp"
#include "../Util.hpp"
#include "../AppList.hpp"

using namespace Gwen::Controls;

namespace FreeShop {
namespace GUI {


Settings::Settings(Gwen::Skin::TexturedBase *skin,  State *state)
: m_ignoreCheckboxChange(false)
, m_state(state)
{
	m_canvas = new Canvas(skin);
	m_canvas->SetPos(0, 0);

	m_input.Initialize(m_canvas);

	m_tabControl = new TabControl(m_canvas);
	m_tabControl->SetBounds(0, 40, 320, 200);

	Base *page = m_tabControl->AddPage(_("Filter").toAnsiString())->GetPage();

	// Filters
	m_filterTabControl = new TabControl(page);
	m_filterTabControl->Dock(Gwen::Pos::Fill);
	m_filterTabControl->SetTabStripPosition(Gwen::Pos::Left);
	Base *filterPage = m_filterTabControl->AddPage(_("Regions").toAnsiString())->GetPage();
	fillRegions(filterPage);

	ScrollControl *scrollBox;
	scrollBox = addFilterPage("Genre");
	fillGenres(scrollBox);
	filterPage = m_filterTabControl->AddPage(_("Language").toAnsiString())->GetPage();
//	filterPage = m_filterTabControl->AddPage(_("Feature").toAnsiString())->GetPage();
	scrollBox = addFilterPage("Platform");
	fillPlatforms(scrollBox);

	page = m_tabControl->AddPage(_("Sort").toAnsiString())->GetPage();

	page = m_tabControl->AddPage(_("Update").toAnsiString())->GetPage();

	page = m_tabControl->AddPage(_("Other").toAnsiString())->GetPage();
}

Settings::~Settings()
{
	delete m_canvas;
}

bool Settings::processEvent(const cpp3ds::Event &event)
{
	return m_input.ProcessMessage(event);
}

bool Settings::update(float delta)
{
	return false;
}

void Settings::render()
{
	m_canvas->RenderCanvas();
}

int Settings::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case POSITION_XY:
			returnValues[0] = getPosition().x;
			returnValues[1] = getPosition().y;
			return 2;
		default: return -1;
	}
}

void Settings::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case POSITION_XY: setPosition(cpp3ds::Vector2f(newValues[0], newValues[1])); break;
		default: break;
	}
}

void Settings::setPosition(const cpp3ds::Vector2f &position)
{
	m_position = position;
	m_canvas->SetPos(position.x, position.y);
}

const cpp3ds::Vector2f &Settings::getPosition() const
{
	return m_position;
}

void Settings::fillGenres(Gwen::Controls::Base *parent)
{
	std::string jsonFilename = "sdmc:/freeShop/cache/genres.json";
	if (!pathExists(jsonFilename.c_str()))
	{
		Download download("https://samurai.ctr.shop.nintendo.net/samurai/ws/US/genres/?shop_id=1", jsonFilename);
		download.setField("Accept", "application/json");
		download.run();
	}

	rapidjson::Document doc;
	std::string json;
	cpp3ds::FileInputStream file;
	if (file.open(jsonFilename))
	{
		json.resize(file.getSize());
		file.read(&json[0], json.size());
		doc.Parse(json.c_str());
		if (doc.HasParseError())
		{
			unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
			return;
		}

		CheckBoxWithLabel* checkbox;
		Label *labelCount;
		rapidjson::Value &list = doc["genres"]["genre"];
		std::vector<std::pair<int, std::string>> genres;
		for (int i = 0; i < list.Size(); ++i)
		{
			int genreId = list[i]["id"].GetInt();
			std::string genreName = list[i]["name"].GetString();
			genres.push_back(std::make_pair(genreId, genreName));
		}

		// Alphabetical sort
		std::sort(genres.begin(), genres.end(), [=](std::pair<int, std::string>& a, std::pair<int, std::string>& b) {
			return a.second < b.second;
		});

		for (int i = 0; i < genres.size(); ++i)
		{
			int genreId = genres[i].first;
			std::string &genreName = genres[i].second;
			int gameCount = 0;
			for (auto &app : AppList::getInstance().getList())
				for (auto &id : app->getAppItem()->getGenres())
					if (id == genreId)
						gameCount++;

			labelCount = new Label(parent);
			labelCount->SetText(_("%d", gameCount).toAnsiString());
			labelCount->SetBounds(0, 2 + i * 18, 23, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(27, i * 18);
			checkbox->Label()->SetText(genreName);
			checkbox->Checkbox()->SetValue(_("%d", genreId).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

			m_filterGenreCheckboxes.push_back(checkbox);
		}
	}
}

void Settings::fillPlatforms(Gwen::Controls::Base *parent)
{
	std::string jsonFilename = "sdmc:/freeShop/cache/platforms.json";
	if (!pathExists(jsonFilename.c_str()))
	{
		Download download("https://samurai.ctr.shop.nintendo.net/samurai/ws/US/platforms/?shop_id=1", jsonFilename);
		download.setField("Accept", "application/json");
		download.run();
	}

	rapidjson::Document doc;
	std::string json;
	cpp3ds::FileInputStream file;
	if (file.open(jsonFilename))
	{
		json.resize(file.getSize());
		file.read(&json[0], json.size());
		doc.Parse(json.c_str());
		if (doc.HasParseError())
		{
			unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
			return;
		}

		CheckBoxWithLabel* checkbox;
		Label *labelCount;
		rapidjson::Value &list = doc["platforms"]["platform"];
		std::vector<std::pair<int, std::string>> platforms;
		for (int i = 0; i < list.Size(); ++i)
		{
			int platformId = list[i]["id"].GetInt();
			std::string platformName = list[i]["name"].GetString();
			platforms.push_back(std::make_pair(platformId, platformName));
		}

		// Alphabetical sort
		std::sort(platforms.begin(), platforms.end(), [=](std::pair<int, std::string>& a, std::pair<int, std::string>& b) {
			return a.second < b.second;
		});

		for (int i = 0; i < platforms.size(); ++i)
		{
			int platformId = platforms[i].first;
			std::string &platformName = platforms[i].second;
			int gameCount = 0;
			for (auto &app : AppList::getInstance().getList())
				if (platformId == app->getAppItem()->getPlatform())
					gameCount++;

			labelCount = new Label(parent);
			labelCount->SetText(_("%d", gameCount).toAnsiString());
			labelCount->SetBounds(0, 2 + i * 18, 23, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(27, i * 18);
			checkbox->Label()->SetText(platformName);
			checkbox->Checkbox()->SetValue(_("%d", platformId).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

			m_filterPlatformCheckboxes.push_back(checkbox);
		}
	}
}

#define CHECKBOXES_SET(checkboxes, value) \
	{ \
        for (auto &checkbox : checkboxes) \
            checkbox->Checkbox()->SetChecked(value); \
		m_ignoreCheckboxChange = false; \
		auto checkbox = checkboxes.front()->Checkbox(); \
		checkbox->onCheckChanged.Call(checkbox); \
    }

void Settings::selectAll(Gwen::Controls::Base *control)
{
	std::string filterName = control->GetParent()->GetParent()->GetName();
	m_ignoreCheckboxChange = true;
	if (filterName == "Genre")
		CHECKBOXES_SET(m_filterGenreCheckboxes, true)
	else if (filterName == "Platform")
		CHECKBOXES_SET(m_filterPlatformCheckboxes, true)
}

void Settings::selectNone(Gwen::Controls::Base *control)
{
	std::string filterName = control->GetParent()->GetParent()->GetName();
	m_ignoreCheckboxChange = true;
	if (filterName == "Genre")
		CHECKBOXES_SET(m_filterGenreCheckboxes, false)
	else if (filterName == "Platform")
		CHECKBOXES_SET(m_filterPlatformCheckboxes, false)
}

void Settings::filterCheckboxChanged(Gwen::Controls::Base *control)
{
	if (m_ignoreCheckboxChange)
		return;
	std::string filterName = control->GetParent()->GetParent()->GetParent()->GetName();
	if (filterName == "Genre")
	{
		std::vector<int> genres;
		for (auto &checkbox : m_filterGenreCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int genreId = atoi(checkbox->Checkbox()->GetValue().c_str());
				genres.push_back(genreId);
			}
		AppList::getInstance().setFilterGenres(genres);
	}
	else if (filterName == "Platform")
	{
		std::vector<int> platforms;
		for (auto &checkbox : m_filterPlatformCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int platformId = atoi(checkbox->Checkbox()->GetValue().c_str());
				platforms.push_back(platformId);
			}
		AppList::getInstance().setFilterPlatforms(platforms);
	}
}

void Settings::filterRegionCheckboxChanged(Gwen::Controls::Base *control)
{
	int regions = 0;
	for (auto &checkbox : m_filterRegionCheckboxes)
		if (checkbox->Checkbox()->IsChecked())
		{
			int region = atoi(checkbox->Checkbox()->GetValue().c_str());
			regions |= region;
		}
	AppList::getInstance().setFilterRegions(regions);
}

Gwen::Controls::ScrollControl *Settings::addFilterPage(const std::string &name)
{
	// Add tab page
	Base *filterPage = m_filterTabControl->AddPage(_(name).toAnsiString())->GetPage();
	filterPage->SetName(name);
	// Add group parent for the buttons
	Base *group = new Base(filterPage);
	group->Dock(Gwen::Pos::Top);
	group->SetSize(200, 18);
	group->SetMargin(Gwen::Margin(0, 0, 0, 3));
	// Add the two Select buttons
	Button *button = new Button(group);
	button->Dock(Gwen::Pos::Left);
	button->SetText(_("Select All").toAnsiString());
	button->onPress.Add(this, &Settings::selectAll);
	button = new Button(group);
	button->Dock(Gwen::Pos::Left);
	button->SetText(_("Select None").toAnsiString());
	button->onPress.Add(this, &Settings::selectNone);
	// Scrollbox to put under the two buttons
	ScrollControl *scrollBox = new ScrollControl(filterPage);
	scrollBox->Dock(Gwen::Pos::Fill);
	scrollBox->SetScroll(false, true);
	// Return scrollbox to be filled with controls (probably checkboxes)
	return scrollBox;
}

void Settings::fillRegions(Gwen::Controls::Base *page)
{
	for (int i = 0; i < 3; ++i)
	{
		cpp3ds::String strRegion;
		int region = 1 << i;
		int count = 0;

		// Get region title counts
		for (auto &app : AppList::getInstance().getList())
			if (app->getAppItem()->getRegions() & region)
				count++;

		if (i == 0)
			strRegion = _("Japan");
		else if (i == 1)
			strRegion = _("USA");
		else
			strRegion = _("Europe");

		auto labelCount = new Label(page);
		labelCount->SetText(_("%d", count).toAnsiString());
		labelCount->SetBounds(0, 2 + i * 18, 31, 18);
		labelCount->SetAlignment(Gwen::Pos::Right);

		auto checkbox = new CheckBoxWithLabel(page);
		checkbox->SetPos(35, i * 18);
		checkbox->Label()->SetText(strRegion.toAnsiString());
		checkbox->Checkbox()->SetValue(_("%d", region).toAnsiString());
		checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterRegionCheckboxChanged);

		m_filterRegionCheckboxes.push_back(checkbox);
	}
}

} // namespace GUI
} // namespace FreeShop
