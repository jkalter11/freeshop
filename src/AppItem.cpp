#include <cpp3ds/System/Err.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "AssetManager.hpp"
#include "AppItem.hpp"
#include "Util.hpp"
#include <sstream>


namespace {

std::vector<std::unique_ptr<cpp3ds::Texture>> textures;

#include "fuzzysearch.inl"

}


namespace FreeShop
{

AppItem::AppItem()
: m_installed(false)
, m_infoVisible(true)
, m_visible(true)
, m_matchScore(-1)
{
	deselect();

	m_icon.setSize(cpp3ds::Vector2f(48.f, 48.f));
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/missing-icon.png");
	texture.setSmooth(true);
	m_icon.setTexture(&texture, true);
	m_icon.setPosition(4.f, 4.f);
	m_icon.setFillColor(cpp3ds::Color(180,180,180));
	m_icon.setOutlineColor(cpp3ds::Color(0, 0, 0, 50));

	m_progressBar.setFillColor(cpp3ds::Color(0, 0, 0, 50));

	m_titleText.setCharacterSize(12);
	m_titleText.setPosition(57.f, 3.f);
	m_titleText.setFillColor(cpp3ds::Color(50,50,50));
	m_titleText.useSystemFont();

	m_filesizeText.setCharacterSize(10);
	m_filesizeText.setPosition(56.f, 38.f);
	m_filesizeText.setFillColor(cpp3ds::Color(150,150,150));
	m_filesizeText.useSystemFont();

	setSize(194.f, 56.f);
}

AppItem::~AppItem()
{

}

void AppItem::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	cpp3ds::FloatRect rect(0, 0, 400, 240);
	cpp3ds::FloatRect rect2 = states.transform.transformRect(rect);
	if (rect.intersects(rect2)) {
		if (m_infoVisible) {
			target.draw(m_background, states);
			target.draw(m_icon, states);
			target.draw(m_titleText, states);
			target.draw(m_filesizeText, states);
		} else {
			target.draw(m_icon, states);
		}
	}

}

void AppItem::setSize(float width, float height)
{
	m_size.x = width;
	m_size.y = height;

	m_background.setContentSize(m_size.x + m_background.getPadding().width - m_background.getTexture()->getSize().x + 2.f,
								m_size.y + m_background.getPadding().height - m_background.getTexture()->getSize().y + 2.f);
}

const cpp3ds::Vector2f& AppItem::getSize() const
{
	return m_size;
}

void AppItem::setTitle(const cpp3ds::String &title)
{
	m_title = title;

	cpp3ds::Text tmpText = m_titleText;
	auto s = title.toUtf8();

	int lineCount = 1;
	int startPos = 0;
	int lastSpace = 0;
	for (int i = 0; i < s.size(); ++i)
	{
		if (s[i] == ' ')
			lastSpace = i;
		tmpText.setString(cpp3ds::String::fromUtf8(s.begin() + startPos, s.begin() + i));
		if (tmpText.getLocalBounds().width > 126)
		{
			if (lineCount < 2 && lastSpace != 0)
			{
				s[lastSpace] = '\n';
				i = startPos = lastSpace + 1;
				lastSpace = 0;
				lineCount++;
			}
			else
			{
				s.erase(s.begin() + i, s.end());
				break;
			}
		}
	}

	m_titleText.setString(cpp3ds::String::fromUtf8(s.begin(), s.end()));
}

const cpp3ds::String &AppItem::getTitle() const
{
	return m_title;
}

void AppItem::setIcon(size_t iconIndex)
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
			texture->loadFromFile(_("sdmc:/freeShop/cache/images/icons%d.jpg", i));
#else
			texture->loadFromPreprocessedFile(_("sdmc:/freeShop/cache/images/icons%d.bin", i), 1024, 1024, GPU_ETC1);
#endif
			texture->setSmooth(true);
			textures.push_back(std::move(texture));
		}
	}

	m_icon.setTexture(textures[textureIndex].get());
	m_icon.setFillColor(cpp3ds::Color::White);

	int x = (iconIndex / 21) * 48;
	int y = (iconIndex % 21) * 48;
	m_icon.setTextureRect(cpp3ds::IntRect(x, y, 48, 48));
}

void AppItem::loadFromJSON(const char* titleId, const rapidjson::Value &json)
{
	setTitleId(titleId);
	const char *title = json[0].GetString();
	setTitle(cpp3ds::String::fromUtf8(title, title + json[0].GetStringLength()));
	setNormalizedTitle(json[1].GetString());
	setContentId(json[2].GetString());
	setUriRegion(json[4].GetString());
	setFilesize(json[5].GetUint64());
	setVersion(json[7].GetInt());

	m_regions.clear();
	for (auto it = json[3].Begin(); it != json[3].End(); it++)
	{
		std::string region = it->GetString();
		if (region == "US")
			m_regions.push_back(USA);
		else if (region == "EU")
			m_regions.push_back(Europe);
		else
			m_regions.push_back(Japan);
	}

	int iconIndex = json[6].GetInt();
	if (iconIndex != -1)
		setIcon(iconIndex);
}

void AppItem::select()
{
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/itembg-selected.9.png");
	m_background.setTexture(&texture);
	m_background.setColor(cpp3ds::Color(255, 255, 255, 200));
	m_icon.setOutlineThickness(2.f);
}

void AppItem::deselect()
{
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/itembg.9.png");
	m_background.setTexture(&texture);
	m_background.setColor(cpp3ds::Color(255, 255, 255, 40));
	m_icon.setOutlineThickness(0.f);
}

void AppItem::setInstalled(bool installed)
{
	m_installed = installed;
}

bool AppItem::isInstalled() const
{
	return m_installed;
}

void AppItem::setFilesize(cpp3ds::Uint64 filesize)
{
	m_filesize = filesize;

	if (filesize > 1024 * 1024 * 1024)
		m_filesizeText.setString(_("%.1f GB", static_cast<float>(filesize) / 1024.f / 1024.f / 1024.f));
	else if (filesize > 1024 * 1024)
		m_filesizeText.setString(_("%.1f MB", static_cast<float>(filesize) / 1024.f / 1024.f));
	else
		m_filesizeText.setString(_("%d KB", filesize / 1024));

	m_filesizeText.setPosition(189.f - m_filesizeText.getLocalBounds().width, 38.f);
}

cpp3ds::Uint64 AppItem::getFilesize() const
{
	return m_filesize;
}

const cpp3ds::Texture *AppItem::getIcon(cpp3ds::IntRect &outRect) const
{
	outRect = m_icon.getTextureRect();
	return m_icon.getTexture();
}

bool AppItem::isVisible() const
{
	return m_visible;
}

void AppItem::setVisible(bool visible)
{
	m_visible = visible;
}

void AppItem::setInfoVisible(bool visible)
{
	m_infoVisible = visible;
}

bool AppItem::isInfoVisible() const
{
	return m_infoVisible;
}

#define SET_ALPHA(getFunc, setFunc, maxValue) \
	color = getFunc(); \
	color.a = std::max(std::min(newValues[0] * maxValue / 255.f, 255.f), 0.f); \
	setFunc(color);

void AppItem::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case INFO_ALPHA: {
			cpp3ds::Color color;
			SET_ALPHA(m_background.getColor, m_background.setColor, 40.f);
			SET_ALPHA(m_titleText.getFillColor, m_titleText.setFillColor, 255.f);
			SET_ALPHA(m_filesizeText.getFillColor, m_filesizeText.setFillColor, 255.f);
			break;
		}
		case BACKGROUND_ALPHA: {
			cpp3ds::Color color;
			SET_ALPHA(m_background.getColor, m_background.setColor, 255.f);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int AppItem::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case BACKGROUND_ALPHA:
			returnValues[0] = m_background.getColor().a;
			return 1;
		case INFO_ALPHA:
			returnValues[0] = m_titleText.getFillColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void AppItem::setMatchTerm(const std::string &string)
{
	if (string.empty())
		m_matchScore = 0;
	else if (!fuzzy_match(string.c_str(), m_normalizedTitle.c_str(), m_matchScore))
		m_matchScore = -99;
}

int AppItem::getMatchScore() const
{
	return m_matchScore;
}

void AppItem::setNormalizedTitle(const std::string &title)
{
	m_normalizedTitle = title;
}

const std::string &AppItem::getNormalizedTitle() const
{
	return m_normalizedTitle;
}

void AppItem::setTitleId(const std::string &id)
{
	m_titleId = id;
}

const std::string &AppItem::getTitleId() const
{
	return m_titleId;
}

void AppItem::setContentId(const std::string &id)
{
	m_contentId = id;
}

const std::string &AppItem::getContentId() const
{
	return m_contentId;
}

void AppItem::setUriRegion(const std::string &region)
{
	m_uriRegion = region;
}

const std::string &AppItem::getUriRegion() const
{
	return m_uriRegion;
}

const std::vector<AppItem::Region> &AppItem::getRegions() const
{
	return m_regions;
}

bool AppItem::isCached() const
{
	return pathExists(getJsonFilename().c_str());
}

const std::string AppItem::getJsonFilename() const
{
	std::string filename = "sdmc:/freeShop/tmp/" + getTitleId() + "/data.json";
	return filename;
}

void AppItem::setVersion(int version)
{
	m_version = version;
}

int AppItem::getVersion() const
{
	return m_version;
}

} // namespace FreeShop
