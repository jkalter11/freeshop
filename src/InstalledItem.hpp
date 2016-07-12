#ifndef FREESHOP_INSTALLEDITEM_HPP
#define FREESHOP_INSTALLEDITEM_HPP

#include "TweenObjects.hpp"
#include "GUI/NinePatch.hpp"

namespace FreeShop {

class InstalledItem : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	InstalledItem(cpp3ds::Uint64 titleId);

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

private:
	cpp3ds::Uint64 m_titleId;
	gui3ds::NinePatch m_background;

	cpp3ds::Text m_textTitle;
	util3ds::TweenText m_textView;
	util3ds::TweenText m_textDelete;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDITEM_HPP
