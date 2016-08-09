#ifndef FREESHOP_INSTALLEDLIST_HPP
#define FREESHOP_INSTALLEDLIST_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include "InstalledItem.hpp"
#include "GUI/Scrollable.hpp"

namespace FreeShop {

class InstalledList : public cpp3ds::Drawable, public Scrollable, public util3ds::TweenTransformable<cpp3ds::Transformable>  {
public:
	static InstalledList &getInstance();
	void refresh();

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

	virtual void setScroll(float position);
	virtual float getScroll();
	virtual const cpp3ds::Vector2f &getScrollSize();

protected:
	InstalledList();
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	void repositionItems();

private:
	std::vector<std::unique_ptr<InstalledItem>> m_installedItems;
	TweenEngine::TweenManager m_tweenManager;
	float m_scrollPos;
	cpp3ds::Vector2f m_size;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDLIST_HPP
