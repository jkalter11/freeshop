#include "SleepState.hpp"
#include <cpp3ds/Window/Window.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

SleepState::SleepState(StateStack& stack, Context& context)
: State(stack, context)
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTH);
		gspLcdExit();
	}
#endif
}

SleepState::~SleepState()
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
		gspLcdExit();
	}
#endif
}

void SleepState::renderTopScreen(cpp3ds::Window& window)
{
}

void SleepState::renderBottomScreen(cpp3ds::Window& window)
{
}

bool SleepState::update(float delta)
{
	return false;
}

bool SleepState::processEvent(const cpp3ds::Event& event)
{
	requestStackPop();
	return true;
}

} // namespace FreeShop
