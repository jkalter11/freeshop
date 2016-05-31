#ifndef FREESHOP_SLEEPSTATE_HPP
#define FREESHOP_SLEEPSTATE_HPP

#include "State.hpp"

namespace FreeShop {

class SleepState : public State
{
public:
	SleepState(StateStack& stack, Context& context);
	~SleepState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);
};

} // namespace FreeShop

#endif // FREESHOP_SLEEPSTATE_HPP
