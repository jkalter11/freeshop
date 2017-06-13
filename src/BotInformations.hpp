#ifndef FREESHOP_BOTINFORMATIONS_HPP
#define FREESHOP_BOTINFORMATIONS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "TitleKeys.hpp"
#include <TweenEngine/Tween.h>
#include <TweenEngine/TweenManager.h>

namespace FreeShop {

class BotInformations : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	void update(float delta);

	BotInformations();
	~BotInformations();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	cpp3ds::Text m_textSD;
	cpp3ds::Text m_textSDStorage;
	cpp3ds::Text m_textNAND;
	cpp3ds::Text m_textNANDStorage;

	cpp3ds::Text m_textSleepDownloads;

	gui3ds::NinePatch m_backgroundNAND;
	gui3ds::NinePatch m_backgroundSD;
	util3ds::TweenRectangleShape m_progressBarNAND;
	util3ds::TweenRectangleShape m_progressBarSD;

	TweenEngine::TweenManager m_tweenManager;

	//Booleans used for transitions
	bool m_isProgressSDTransitioning;
	bool m_isProgressNANDTransitioning;

};

} // namespace FreeShop

#endif // FREESHOP_BOTINFORMATIONS_HPP
