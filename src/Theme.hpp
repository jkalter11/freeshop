#ifndef FREESHOP_THEME_HPP
#define FREESHOP_THEME_HPP

#include <string>
#include <rapidjson/document.h>
#include <cpp3ds/Graphics/Color.hpp>

namespace FreeShop {

class Theme {
public:
	//Images vars
	static bool isFlagsThemed;
	static bool isItemBG9Themed;
	static bool isButtonRadius9Themed;
	static bool isFSBGSD9Themed;
	static bool isFSBGNAND9Themed;
	static bool isInstalledItemBG9Themed;
	static bool isItemBGSelected9Themed;
	static bool isListItemBG9Themed;
	static bool isMissingIconThemed;
	static bool isNotification9Themed;
	static bool isQrSelector9Themed;
	static bool isScrollbar9Themed;

	//Sounds vars
	static bool isSoundBlipThemed;
	static bool isSoundChimeThemed;
	static bool isSoundStartupThemed;

	//Text theming
	static Theme& getInstance();
	void loadDefaults();
	static bool loadFromFile(const std::string& filename = FREESHOP_DIR "/theme/texts.json");
	static void saveToFile(const std::string& filename = FREESHOP_DIR "/theme/texts.json");
	static const rapidjson::Value &get(std::string key);

	static bool isTextThemed;
	static cpp3ds::Color primaryTextColor;
	static cpp3ds::Color secondaryTextColor;
	static cpp3ds::Color iconSetColor;

private:
	rapidjson::Document m_json;
};

} // namespace FreeShop

#endif // FREESHOP_THEME_HPP