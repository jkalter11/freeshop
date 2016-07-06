#ifndef FREESHOP_APPITEM_HPP
#define FREESHOP_APPITEM_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <rapidjson/document.h>
#include "GUI/NinePatch.hpp"
#include "TweenObjects.hpp"

namespace FreeShop {

class AppItem {
public:
	enum Region {
		Japan     = 1 << 0,
		USA       = 1 << 1,
		Europe    = 1 << 2,
		Australia = 1 << 3,
		China     = 1 << 4,
		Korea     = 1 << 5,
		Taiwan    = 1 << 6,
	};

	enum Language {
		Japanese = 1 << 0,
		English  = 1 << 1,
	};

	AppItem();

	void loadFromJSON(const char* titleId, const rapidjson::Value &json);

	const cpp3ds::String &getTitle() const;
	const std::string &getNormalizedTitle() const;

	cpp3ds::Uint64 getTitleId() const;
	const std::string &getTitleIdStr() const;
	const std::string &getContentId() const;
	const std::string &getUriRegion() const;

	int getRegions() const;
	int getLanguages() const;
	const std::vector<char> &getSeed() const;
	cpp3ds::Uint64 getFilesize() const;

	void setInstalled(bool installed);
	bool isInstalled() const;
	const std::string getJsonFilename() const;
	bool isCached() const;

	const cpp3ds::Texture* getIcon(cpp3ds::IntRect &outRect) const;
	int getIconIndex() const;

	const std::vector<int> &getGenres() const;
	int getPlatform() const;
	int getPublisher() const;

	void queueForInstall();

private:
	void setIconIndex(size_t iconIndex);

	cpp3ds::String m_title;
	std::string m_normalizedTitle;
	cpp3ds::Uint64 m_filesize;
	cpp3ds::Uint64 m_titleId;
	std::string m_titleIdStr;
	std::string m_contentId;
	std::string m_uriRegion;
	std::vector<char> m_seed;
	std::vector<int> m_genres;
	std::vector<int> m_features;
	int m_platform;
	int m_publisher;
	int m_languages;
	int m_regions;

	std::vector<cpp3ds::Uint64> m_updates;
	std::vector<cpp3ds::Uint64> m_demos;
	std::vector<cpp3ds::Uint64> m_dlc;

	time_t m_releaseDate;
	int m_iconIndex;
	bool m_installed;

	cpp3ds::Texture *m_iconTexture;
	cpp3ds::IntRect m_iconRect;
};

} // namepace FreeShop


#endif // FREESHOP_APPITEM_HPP
