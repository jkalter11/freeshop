#ifndef FREESHOP_INSTALLEDLIST_HPP
#define FREESHOP_INSTALLEDLIST_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include "InstalledItem.hpp"

namespace FreeShop {

class InstalledList : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable>  {
public:
	static InstalledList &getInstance();
	void refresh();

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

protected:
	InstalledList();
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	std::vector<std::unique_ptr<InstalledItem>> m_installedItems;
	TweenEngine::TweenManager m_tweenManager;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDLIST_HPP
