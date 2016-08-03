#ifndef FREESHOP_INSTALLEDITEM_HPP
#define FREESHOP_INSTALLEDITEM_HPP

#include "TweenObjects.hpp"
#include "GUI/NinePatch.hpp"
#include "AppItem.hpp"

namespace FreeShop {

class InstalledItem : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	InstalledItem(cpp3ds::Uint64 titleId);

	cpp3ds::Uint64 getTitleId() const;

	void setUpdateStatus(cpp3ds::Uint64 titleId, bool installed);
	void setDLCStatus(cpp3ds::Uint64 titleId, bool installed);

	std::shared_ptr<AppItem> getAppItem() const;

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

private:
	cpp3ds::Uint64 m_titleId;
	gui3ds::NinePatch m_background;

	std::map<cpp3ds::Uint64, bool> m_updates;
	std::map<cpp3ds::Uint64, bool> m_dlc;
	size_t m_updateInstallCount;
	size_t m_dlcInstallCount;

	cpp3ds::Text m_textTitle;
	util3ds::TweenText m_textView;
	util3ds::TweenText m_textDelete;

	util3ds::TweenText m_textWarningUpdate;
	util3ds::TweenText m_textWarningDLC;

	std::shared_ptr<AppItem> m_appItem;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDITEM_HPP
