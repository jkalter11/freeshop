#ifndef FREESHOP_FREESHOP_HPP
#define FREESHOP_FREESHOP_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Graphics.hpp>
#include <cpp3ds/Network.hpp>
#include "States/StateStack.hpp"


namespace FreeShop {

class FreeShop: public cpp3ds::Game {
public:
	FreeShop();
	~FreeShop();
	void update(float delta);
	void processEvent(cpp3ds::Event& event);
	void renderTopScreen(cpp3ds::Window& window);
	void renderBottomScreen(cpp3ds::Window& window);

private:
	StateStack m_stateStack;
	cpp3ds::Text textFPS;

	// Shared State context variables
	cpp3ds::String m_name;
};

}

#endif // FREESHOP_FREESHOP_HPP
