#include <cpp3ds/System/I18n.hpp>
#include <Gwen/Skins/TexturedBase.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <unistd.h>
#include <bits/basic_string.h>
#include <cpp3ds/System/FileSystem.hpp>
#include "Settings.hpp"
#include "../Download.hpp"
#include "../Util.hpp"
#include "../AppList.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"

#ifdef _3DS
#include "../KeyboardApplet.hpp"
#endif

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
	fillFilterRegions(filterPage);

	ScrollControl *scrollBox;
	scrollBox = addFilterPage("Genre");
	fillFilterGenres(scrollBox);

	scrollBox = addFilterPage("Language");
	fillFilterLanguages(scrollBox);

//	scrollBox = addFilterPage("Feature");

	scrollBox = addFilterPage("Platform");
	fillFilterPlatforms(scrollBox);

	// Sorting options
	page = m_tabControl->AddPage(_("Sort").toAnsiString())->GetPage();
	fillSortPage(page);

	page = m_tabControl->AddPage(_("Update").toAnsiString())->GetPage();
	fillUpdatePage(page);

	page = m_tabControl->AddPage(_("Other").toAnsiString())->GetPage();

	loadConfig();
	updateEnabledStates();
	updateSorting(nullptr);
}

Settings::~Settings()
{
	saveToConfig();
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

void Settings::saveToConfig()
{
	Config::set("auto-update", m_checkboxAutoUpdate->Checkbox()->IsChecked());
	Config::set("download_title_keys", m_checkboxDownloadKeys->Checkbox()->IsChecked());

	rapidjson::Value list(rapidjson::kArrayType);
	if (m_comboBoxUrls->GetSelectedItem())
	{
		auto menuList = m_comboBoxUrls->GetSelectedItem()->GetParent()->GetChildren();
		for (auto it = menuList.begin(); it != menuList.end(); ++it)
		{
			rapidjson::Value val(rapidjson::StringRef((*it)->GetValue().c_str()), Config::getAllocator());
			list.PushBack(val, Config::getAllocator());
		}
	}
	Config::set("key_urls", list);
}

void Settings::loadConfig()
{
	m_checkboxAutoUpdate->Checkbox()->SetChecked(Config::get("auto-update").GetBool());
	m_checkboxDownloadKeys->Checkbox()->SetChecked(Config::get("download_title_keys").GetBool());

	auto urls = Config::get("key_urls").GetArray();
	m_comboBoxUrls->ClearItems();
	for (int i = 0; i < urls.Size(); ++i)
	{
		const char *url = urls[i].GetString();
		std::wstring ws(url, url + urls[i].GetStringLength());
		m_comboBoxUrls->AddItem(ws);
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

void Settings::fillFilterGenres(Gwen::Controls::Base *parent)
{
	std::string jsonFilename = FREESHOP_DIR "/cache/genres.json";
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
			labelCount->SetBounds(0, 2 + i * 18, 31, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(35, i * 18);
			checkbox->Label()->SetText(genreName);
			checkbox->Checkbox()->SetValue(_("%d", genreId).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

			m_filterGenreCheckboxes.push_back(checkbox);
		}
	}
}

void Settings::fillFilterPlatforms(Gwen::Controls::Base *parent)
{
	std::string jsonFilename = FREESHOP_DIR "/cache/platforms.json";
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
			labelCount->SetBounds(0, 2 + i * 18, 31, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(35, i * 18);
			checkbox->Label()->SetText(platformName);
			checkbox->Checkbox()->SetValue(_("%d", platformId).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

			m_filterPlatformCheckboxes.push_back(checkbox);
		}
	}
}

void Settings::fillFilterRegions(Gwen::Controls::Base *parent)
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

		auto labelCount = new Label(parent);
		labelCount->SetText(_("%d", count).toAnsiString());
		labelCount->SetBounds(0, 2 + i * 18, 31, 18);
		labelCount->SetAlignment(Gwen::Pos::Right);

		auto checkbox = new CheckBoxWithLabel(parent);
		checkbox->SetPos(35, i * 18);
		checkbox->Label()->SetText(strRegion.toAnsiString());
		checkbox->Checkbox()->SetValue(_("%d", region).toAnsiString());
		checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterRegionCheckboxChanged);

		m_filterRegionCheckboxes.push_back(checkbox);
	}
}

void Settings::fillFilterLanguages(Gwen::Controls::Base *parent)
{
	CheckBoxWithLabel* checkbox;
	Label *labelCount;

	for (int i = 0; i < 9; ++i)
	{
		auto lang = static_cast<FreeShop::AppItem::Language>(1 << i);
		cpp3ds::String langString;
		switch (lang) {
			default:
			case FreeShop::AppItem::English: langString = _("English"); break;
			case FreeShop::AppItem::Japanese: langString = _("Japanese"); break;
			case FreeShop::AppItem::Spanish: langString = _("Spanish"); break;
			case FreeShop::AppItem::French: langString = _("French"); break;
			case FreeShop::AppItem::German: langString = _("German"); break;
			case FreeShop::AppItem::Italian: langString = _("Italian"); break;
			case FreeShop::AppItem::Dutch: langString = _("Dutch"); break;
			case FreeShop::AppItem::Portuguese: langString = _("Portuguese"); break;
			case FreeShop::AppItem::Russian: langString = _("Russian"); break;
		}

		// Get language title counts
		int gameCount = 0;
		for (auto &app : AppList::getInstance().getList())
			if (app->getAppItem()->getLanguages() & lang)
				gameCount++;

		labelCount = new Label(parent);
		labelCount->SetText(_("%d", gameCount).toAnsiString());
		labelCount->SetBounds(0, 2 + i * 18, 31, 18);
		labelCount->SetAlignment(Gwen::Pos::Right);

		checkbox = new CheckBoxWithLabel(parent);
		checkbox->SetPos(35, i * 18);
		checkbox->Label()->SetText(langString.toAnsiString());
		checkbox->Checkbox()->SetValue(_("%d", lang).toAnsiString());
		checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

		m_filterLanguageCheckboxes.push_back(checkbox);
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
	else if (filterName == "Language")
		CHECKBOXES_SET(m_filterLanguageCheckboxes, true)
	else if (filterName == "Platform")
		CHECKBOXES_SET(m_filterPlatformCheckboxes, true)
}

void Settings::selectNone(Gwen::Controls::Base *control)
{
	std::string filterName = control->GetParent()->GetParent()->GetName();
	m_ignoreCheckboxChange = true;
	if (filterName == "Genre")
		CHECKBOXES_SET(m_filterGenreCheckboxes, false)
	else if (filterName == "Language")
		CHECKBOXES_SET(m_filterLanguageCheckboxes, false)
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
	else if (filterName == "Language")
	{
		int languages = 0;
		for (auto &checkbox : m_filterLanguageCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
				languages |= atoi(checkbox->Checkbox()->GetValue().c_str());
		AppList::getInstance().setFilterLanguages(languages);
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

void Settings::fillSortPage(Gwen::Controls::Base *page)
{
	m_radioSortType = new RadioButtonController(page);
	m_radioSortType->SetBounds(10, 0, 180, 120);
	m_radioSortType->AddOption(_("Name").toWideString(), "Name")->Select();
	m_radioSortType->AddOption(_("Size").toWideString(), "Size");
	m_radioSortType->AddOption(_("Vote Score").toWideString(), "Vote Score");
	m_radioSortType->AddOption(_("Vote Count").toWideString(), "Vote Count");
//	m_radioSortType->AddOption(_("Release Date").toWideString(), "Release Date");
	m_radioSortType->onSelectionChange.Add(this, &Settings::updateSorting);

	m_radioSortDirection = new RadioButtonController(page);
	m_radioSortDirection->SetBounds(190, 5, 150, 120);
	m_radioSortDirection->AddOption(_("Ascending").toWideString(), "Ascending")->Select();
	m_radioSortDirection->AddOption(_("Descending").toWideString(), "Descending");
	m_radioSortDirection->onSelectionChange.Add(this, &Settings::updateSorting);
}

void Settings::fillUpdatePage(Gwen::Controls::Base *page)
{
	Gwen::Padding iconPadding(0, 0, 0, 4);
	auto base = new Base(page);
	base->SetBounds(0, 0, 320, 40);

	m_checkboxDownloadKeys = new CheckBoxWithLabel(base);
	m_checkboxDownloadKeys->SetBounds(0, 0, 300, 20);
	m_checkboxDownloadKeys->Label()->SetText(_("Auto-update title keys from URL").toAnsiString());
	m_checkboxDownloadKeys->Checkbox()->onCheckChanged.Add(this, &Settings::updateEnabledState);

	m_buttonUrlQr = new Button(base);
	m_buttonUrlQr->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonUrlQr->SetText(L"\uf029"); // QR icon
	m_buttonUrlQr->SetPadding(iconPadding);
	m_buttonUrlQr->SetBounds(0, 20, 20, 20);
	m_buttonUrlQr->onPress.Add(this, &Settings::updateQrClicked);

	m_buttonUrlKeyboard = new Button(base);
	m_buttonUrlKeyboard->SetFont(L"fonts/fontawesome.ttf", 20, false);
	m_buttonUrlKeyboard->SetText(L"\uf11c"); // Keyboard icon
	m_buttonUrlKeyboard->SetPadding(Gwen::Padding(0, 0, 0, 6));
	m_buttonUrlKeyboard->SetBounds(21, 20, 26, 20);
	m_buttonUrlKeyboard->onPress.Add(this, &Settings::updateKeyboardClicked);

	m_buttonUrlDelete = new Button(base);
	m_buttonUrlDelete->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonUrlDelete->SetText(L"\uf014"); // Trash can icon
	m_buttonUrlDelete->SetPadding(iconPadding);
	m_buttonUrlDelete->SetBounds(288, 20, 20, 20);
	m_buttonUrlDelete->onPress.Add(this, &Settings::updateUrlDeleteClicked);

	m_comboBoxUrls = new ComboBox(base);
	m_comboBoxUrls->SetBounds(50, 20, 235, 20);
	m_comboBoxUrls->onSelection.Add(this, &Settings::updateUrlSelected);


	base = new Base(page);
	base->SetBounds(0, 48, 320, 60);

	m_checkboxAutoUpdate = new CheckBoxWithLabel(base);
	m_checkboxAutoUpdate->SetBounds(0, 0, 300, 20);
	m_checkboxAutoUpdate->Label()->SetText(_("Auto-update freeShop").toAnsiString());
	m_checkboxAutoUpdate->Checkbox()->onCheckChanged.Add(this, &Settings::updateEnabledState);

	// Button to update - disabled when auto-update is checked
	// Label date last checked (and current version)
	m_buttonUpdate = new Button(base);
	m_buttonUpdate->SetBounds(0, 22, 75, 18);
	m_buttonUpdate->SetText(_("Update").toAnsiString());

	auto label = new Label(base);
	label->SetBounds(78, 24, 300, 20);
	label->SetText(_("(last checked %s)", "28/07/16").toAnsiString());
	label->SetTextColor(Gwen::Color(0, 0, 0, 80));
	label = new Label(base);
	label->SetBounds(78, 44, 300, 20);
	label->SetText(_("freeShop %s", FREESHOP_VERSION).toAnsiString());
}

void Settings::fillOtherPage(Gwen::Controls::Base *page)
{
	//
}

void Settings::updateQrClicked(Gwen::Controls::Base *button)
{
	m_state->requestStackPush(States::QrScanner, true, [this](void *data){
		auto text = reinterpret_cast<cpp3ds::String*>(data);
		if (!text->isEmpty())
		{
			cpp3ds::String strError;
			if (TitleKeys::isValidUrl(*text, &strError))
			{
				m_comboBoxUrls->AddItem(text->toWideString());
				return true;
			}
			*text = strError;
		}
		return false;
	});
}

void Settings::updateKeyboardClicked(Gwen::Controls::Base *textbox)
{
#ifdef _3DS
	KeyboardApplet kb(KeyboardApplet::URL);
	kb.addDictWord("http", "http://");
	kb.addDictWord("https", "https://");
	swkbdSetHintText(kb, "https://<encTitleKeys.bin>");
	cpp3ds::String input = kb.getInput();
	std::cout << input.toAnsiString() << std::endl;
	if (!input.isEmpty())
		m_comboBoxUrls->AddItem(input.toWideString());
#else
	m_comboBoxUrls->AddItem(L"Test");
#endif
}

void Settings::updateUrlSelected(Gwen::Controls::Base *combobox)
{
	updateEnabledStates();
}

void Settings::updateUrlDeleteClicked(Gwen::Controls::Base *button)
{
	MenuItem *selected = gwen_cast<MenuItem>(m_comboBoxUrls->GetSelectedItem());
	MenuItem *newSelected = nullptr;
	if (selected)
	{
		// Find next child to select when current one is deleted
		bool foundSelected = false;
		auto menuList = selected->GetParent()->GetChildren();
		for (auto it = menuList.begin(); it != menuList.end(); ++it)
		{
			auto menuItem = gwen_cast<MenuItem>(*it);
			if (foundSelected) {
				newSelected = menuItem;
				break;
			}

			if (menuItem == selected)
				foundSelected = true;
			else
				newSelected = menuItem;
		}

		selected->DelayedDelete();
		m_comboBoxUrls->SelectItem(newSelected);
		updateEnabledStates();
	}
}

void Settings::updateEnabledStates()
{
	Gwen::Color transparent(0,0,0,0); // Transparent cancels the color override
	bool isDownloadEnabled = m_checkboxDownloadKeys->Checkbox()->IsChecked();
	bool isUrlSelected = !!m_comboBoxUrls->GetSelectedItem();

	m_comboBoxUrls->SetDisabled(!isDownloadEnabled);
	m_buttonUrlQr->SetDisabled(!isDownloadEnabled);
	m_buttonUrlKeyboard->SetDisabled(!isDownloadEnabled);
	m_buttonUrlDelete->SetDisabled(!isDownloadEnabled || !isUrlSelected);
	if (!isUrlSelected)
		m_comboBoxUrls->SetText(_("Remote title key URL(s)").toAnsiString());
	if (isDownloadEnabled) {
		m_buttonUrlQr->SetTextColorOverride(Gwen::Color(0, 100, 0));
		m_buttonUrlKeyboard->SetTextColorOverride(Gwen::Color(0, 100, 0));
		m_buttonUrlDelete->SetTextColorOverride(isUrlSelected ? Gwen::Color(150, 0, 0, 200) : transparent);
	} else {
		m_buttonUrlQr->SetTextColorOverride(transparent);
		m_buttonUrlKeyboard->SetTextColorOverride(transparent);
		m_buttonUrlDelete->SetTextColorOverride(transparent);
	}
	m_buttonUpdate->SetDisabled(m_checkboxAutoUpdate->Checkbox()->IsChecked());
}

void Settings::updateEnabledState(Gwen::Controls::Base *control)
{
	updateEnabledStates();
}

void Settings::updateSorting(Gwen::Controls::Base *control)
{
	AppList::SortType sortType;
	std::string sortTypeName(m_radioSortType->GetSelectedName());
	bool isAscending = (m_radioSortDirection->GetSelectedName() == "Ascending");
	if (sortTypeName == "Name")
		sortType = AppList::Name;
	else if (sortTypeName == "Size")
		sortType = AppList::Size;
	else if (sortTypeName == "Vote Score")
		sortType = AppList::VoteScore;
	else if (sortTypeName == "Vote Count")
		sortType = AppList::VoteCount;
	else
		sortType = AppList::ReleaseDate;

	AppList::getInstance().setSortType(sortType, isAscending);
}


} // namespace GUI
} // namespace FreeShop
