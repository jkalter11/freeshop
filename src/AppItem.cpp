#include <cpp3ds/System/Err.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "AssetManager.hpp"
#include "AppItem.hpp"
#include "Util.hpp"
#include "TitleKeys.hpp"
#include "DownloadQueue.hpp"
#include <sstream>


namespace {

std::vector<std::unique_ptr<cpp3ds::Texture>> textures;

}


namespace FreeShop
{

AppItem::AppItem()
	: m_installed(false)
	, m_regions(0)
	, m_languages(0)
{
	//
}

const cpp3ds::String &AppItem::getTitle() const
{
	return m_title;
}

void AppItem::loadFromJSON(const char* titleId, const rapidjson::Value &json)
{
	m_genres.clear();
	m_features.clear();
	m_seed.clear();
	m_languages = 0;

	m_titleIdStr = titleId;
	m_titleId = strtoull(titleId, 0, 16);
	m_updates = TitleKeys::getRelated(m_titleId, TitleKeys::Update);
	m_demos = TitleKeys::getRelated(m_titleId, TitleKeys::Demo);
	m_dlc = TitleKeys::getRelated(m_titleId, TitleKeys::DLC);

	const char *title = json[0].GetString();
	m_title = cpp3ds::String::fromUtf8(title, title + json[0].GetStringLength());
	m_normalizedTitle = json[1].GetString();
	m_contentId = json[2].GetString();
	m_regions = json[3].GetInt();
	m_uriRegion = json[4].GetString();
	m_filesize = json[5].GetUint64();

	int iconIndex = json[6].GetInt();
	if (iconIndex >= 0)
		setIconIndex(iconIndex);

	// Crypto seed
	std::string seed = json[7].GetString();
	if (!seed.empty())
	{
		m_seed.clear();
		for (int i = 0; i < 16; ++i)
		{
			std::string byteStr = seed.substr(i*2, 2);
			char byte = strtol(byteStr.c_str(), NULL, 16);
			m_seed.push_back(byte);
		}
	}

	// Genres
	auto genres = json[8].GetArray();
	for (int i = 0; i < genres.Size(); ++i)
		m_genres.push_back(genres[i].GetInt());
	// Languages
	auto languages = json[9].GetArray();
	for (int i = 0; i < languages.Size(); ++i)
	{
		std::string lang(languages[i].GetString());
		if ("ja" == lang) m_languages |= Japanese;
		else if ("en" == lang) m_languages |= English;
		else if ("es" == lang) m_languages |= Spanish;
		else if ("fr" == lang) m_languages |= French;
		else if ("de" == lang) m_languages |= German;
		else if ("it" == lang) m_languages |= Italian;
		else if ("nl" == lang) m_languages |= Dutch;
		else if ("pt" == lang) m_languages |= Portuguese;
		else if ("ru" == lang) m_languages |= Russian;
	}
	// Features
	auto features = json[10].GetArray();
	for (int i = 0; i < features.Size(); ++i)
		m_features.push_back(features[i].GetInt());

	if (!json[11].IsNull())
		m_voteScore = json[11].GetFloat();
	m_voteCount = json[12].GetInt();
	m_releaseDate = json[13].GetInt();
	m_productCode = json[14].GetString();

	m_platform = json[15].GetInt();
	m_publisher = json[16].GetInt();
}

void AppItem::setIconIndex(size_t iconIndex)
{
	size_t textureIndex = iconIndex / 441;
	iconIndex %= 441;

	// Load texture from file if not done already
	if (textures.size() < textureIndex + 1)
	{
		for (int i = textures.size(); i <= textureIndex; ++i)
		{
			std::unique_ptr<cpp3ds::Texture> texture(new cpp3ds::Texture());
#ifdef EMULATION
			texture->loadFromFile(_(FREESHOP_DIR "/cache/images/icons%d.jpg", i));
#else
			texture->loadFromPreprocessedFile(_(FREESHOP_DIR "/cache/images/icons%d.bin", i), 1024, 1024, GPU_ETC1);
#endif
			texture->setSmooth(true);
			textures.push_back(std::move(texture));
		}
	}

	m_iconIndex = iconIndex;
	m_iconTexture = textures[textureIndex].get();

	int x = (iconIndex / 21) * 48;
	int y = (iconIndex % 21) * 48;
	m_iconRect = cpp3ds::IntRect(x, y, 48, 48);
}

bool AppItem::isCached() const
{
	return pathExists(getJsonFilename().c_str());
}

const std::string AppItem::getJsonFilename() const
{
	std::string filename = FREESHOP_DIR "/tmp/" + getTitleIdStr() + "/data.json";
	return filename;
}

void AppItem::setInstalled(bool installed)
{
	m_installed = installed;
}

bool AppItem::isInstalled() const
{
	return m_installed;
}

cpp3ds::Uint64 AppItem::getFilesize() const
{
	return m_filesize;
}

const std::string &AppItem::getNormalizedTitle() const
{
	return m_normalizedTitle;
}

const std::string &AppItem::getTitleIdStr() const
{
	return m_titleIdStr;
}

cpp3ds::Uint64 AppItem::getTitleId() const
{
	return m_titleId;
}

const std::string &AppItem::getContentId() const
{
	return m_contentId;
}

const std::string &AppItem::getUriRegion() const
{
	return m_uriRegion;
}

int AppItem::getRegions() const
{
	return m_regions;
}

int AppItem::getLanguages() const
{
	return m_languages;
}

const std::vector<char> &AppItem::getSeed() const
{
	return m_seed;
}

const std::vector<int> &AppItem::getGenres() const
{
	return m_genres;
}

int AppItem::getPlatform() const
{
	return m_platform;
}

int AppItem::getPublisher() const
{
	return m_publisher;
}

int AppItem::getIconIndex() const
{
	return m_iconIndex;
}

const cpp3ds::Texture *AppItem::getIcon(cpp3ds::IntRect &outRect) const
{
	outRect = m_iconRect;
	return m_iconTexture;
}

void AppItem::queueForInstall()
{
	DownloadQueue::getInstance().addDownload(shared_from_this());
	for (auto &id : m_updates)
		DownloadQueue::getInstance().addDownload(shared_from_this(), id);
	for (auto &id : m_dlc)
		DownloadQueue::getInstance().addDownload(shared_from_this(), id);
}

const std::vector<cpp3ds::Uint64> &AppItem::getUpdates() const
{
	return m_updates;
}

const std::vector<cpp3ds::Uint64> &AppItem::getDemos() const
{
	return m_demos;
}
const std::vector<cpp3ds::Uint64> &AppItem::getDLC() const
{
	return m_dlc;
}

float AppItem::getVoteScore() const
{
	return m_voteScore;
}

int AppItem::getVoteCount() const
{
	return m_voteCount;
}

time_t AppItem::getReleaseDate() const
{
	return m_releaseDate;
}

const std::string &AppItem::getProductCode() const
{
	return m_productCode;
}


} // namespace FreeShop
