#ifndef FREESHOP_TOPINFORMATIONS_HPP
#define FREESHOP_TOPINFORMATIONS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "TitleKeys.hpp"
#include <TweenEngine/Tween.h>

namespace FreeShop {

class TopInformations : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	void update();

	TopInformations();
	~TopInformations();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	util3ds::TweenText m_textClock;

};

} // namespace FreeShop

#endif // FREESHOP_TOPINFORMATIONS_HPP
