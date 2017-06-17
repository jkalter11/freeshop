#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Theme.hpp"
#include "Util.hpp"

namespace FreeShop {
//Images vars
bool Theme::isFlagsThemed = false;
bool Theme::isItemBG9Themed = false;
bool Theme::isButtonRadius9Themed = false;
bool Theme::isFSBGSD9Themed = false;
bool Theme::isFSBGNAND9Themed = false;
bool Theme::isInstalledItemBG9Themed = false;
bool Theme::isItemBGSelected9Themed = false;
bool Theme::isListItemBG9Themed = false;
bool Theme::isMissingIconThemed = false;
bool Theme::isNotification9Themed = false;
bool Theme::isQrSelector9Themed = false;
bool Theme::isScrollbar9Themed = false;

//Sounds vars
bool Theme::isSoundBlipThemed = false;
bool Theme::isSoundChimeThemed = false;
bool Theme::isSoundStartupThemed = false;

//Text theming
bool Theme::isTextThemed = false;
cpp3ds::Color Theme::primaryTextColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::secondaryTextColor = cpp3ds::Color(130, 130, 130, 255);
cpp3ds::Color Theme::iconSetColor = cpp3ds::Color(100, 100, 100);
cpp3ds::Color Theme::iconSetColorActive = cpp3ds::Color(0, 0, 0);
cpp3ds::Color Theme::transitionScreenColor = cpp3ds::Color(255, 255, 255);
cpp3ds::Color Theme::loadingIcon = cpp3ds::Color(110, 110, 110, 255);
cpp3ds::Color Theme::loadingText = cpp3ds::Color::Black;
cpp3ds::Color Theme::freText = cpp3ds::Color(255, 255, 255, 0);
cpp3ds::Color Theme::versionText = cpp3ds::Color(0, 0, 0, 100);
cpp3ds::Color Theme::percentageText = cpp3ds::Color::Black;

//Theme informations
std::string Theme::themeName = "Classic";
std::string Theme::themeDesc = "The default theme of freeShop.\nMade by arc13 / Cruel.";
std::string Theme::themeVersion = FREESHOP_VERSION;

Theme &Theme::getInstance()
{
	static Theme theme;
	return theme;
}

#define ADD_DEFAULT(key, val) \
	if (!m_json.HasMember(key)) \
		m_json.AddMember(rapidjson::StringRef(key), val, m_json.GetAllocator());

void Theme::loadDefaults()
{
	if (!m_json.IsObject())
		m_json.SetObject();

	ADD_DEFAULT("primaryText", "000000");
	ADD_DEFAULT("secondaryText", "828282");
	ADD_DEFAULT("iconSet", "646464");
	ADD_DEFAULT("iconSetActive", "363636");
	ADD_DEFAULT("transitionScreen", "FFFFFF");
	ADD_DEFAULT("loadingColor", "898989");
	ADD_DEFAULT("loadingText", "353535");
	ADD_DEFAULT("freText", "420420");
	ADD_DEFAULT("versionText", "546978");
	ADD_DEFAULT("percentageText", "115599");

	ADD_DEFAULT("themeName", "My custom theme");
	ADD_DEFAULT("themeDesc", "Theme made by someone.");
	ADD_DEFAULT("themeVer", "x.y");

	getInstance().saveToFile();
}

bool Theme::loadFromFile(const std::string &filename)
{
	rapidjson::Document &json = getInstance().m_json;
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::string jsonString;
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	jsonString.resize(file.getSize());
	file.read(&jsonString[0], jsonString.size());
	json.Parse(jsonString.c_str());
	getInstance().loadDefaults();
	return !json.HasParseError();
}

const rapidjson::Value &Theme::get(std::string key)
{
	return getInstance().m_json[key.c_str()];
}

void Theme::saveToFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::ofstream file(path);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	getInstance().m_json.Accept(writer);
}

} // namespace FreeShop
