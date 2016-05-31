#ifndef FREESHOP_APPITEM_HPP
#define FREESHOP_APPITEM_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <rapidjson/document.h>
#include "GUI/NinePatch.hpp"
#include "TweenObjects.hpp"

namespace FreeShop {

class AppItem : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int INFO_ALPHA = 11;
	static const int BACKGROUND_ALPHA = 12;

	enum Region {
		USA,
		Europe,
		Japan,
	};

	AppItem();

	~AppItem();

	void loadFromJSON(const char* titleId, const rapidjson::Value &json);

	void setTitle(const cpp3ds::String &title);
	const cpp3ds::String &getTitle() const;
	void setNormalizedTitle(const std::string &title);
	const std::string &getNormalizedTitle() const;

	void setTitleId(const std::string &id);
	const std::string &getTitleId() const;
	void setContentId(const std::string &id);
	const std::string &getContentId() const;
	void setUriRegion(const std::string &region);
	const std::string &getUriRegion() const;
	void setVersion(int version);
	int getVersion() const;

	const std::vector<Region> &getRegions() const;

	void setFilesize(cpp3ds::Uint64 filesize);
	cpp3ds::Uint64 getFilesize() const;

	void setIcon(size_t iconIndex);
	const cpp3ds::Texture* getIcon(cpp3ds::IntRect &outRect) const;

	void setSize(float width, float height);

	const cpp3ds::Vector2f &getSize() const;

	void setInstalled(bool installed);
	bool isInstalled() const;

	void setVisible(bool visible);
	bool isVisible() const;

	void setInfoVisible(bool visible);
	bool isInfoVisible() const;

	void select();

	void deselect();

	void setMatchTerm(const std::string &string);
	int getMatchScore() const;

	const std::string getJsonFilename() const;
	bool isCached() const;

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);

	void addRegion(Region region);

private:
	gui3ds::NinePatch m_background;

	cpp3ds::RectangleShape m_icon;
	cpp3ds::RectangleShape m_progressBar;

	cpp3ds::Text m_titleText;
	cpp3ds::Text m_filesizeText;

	cpp3ds::Vector2f m_size;

	cpp3ds::String m_title;
	std::string m_normalizedTitle;
	cpp3ds::Uint64 m_filesize;
	std::string m_titleId;
	std::string m_contentId;
	std::string m_uriRegion;
	int m_version;
	std::vector<Region> m_regions;
	std::vector<cpp3ds::Sprite> m_regionFlags;

	int m_matchScore;

	bool m_installed;
	bool m_infoVisible;
	mutable bool m_visible;
};

} // namepace FreeShop


#endif // FREESHOP_APPITEM_HPP
