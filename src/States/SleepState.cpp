#include "SleepState.hpp"
#include <cpp3ds/Window/Window.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

bool SleepState::isSleeping = false;
cpp3ds::Clock SleepState::clock;

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
	isSleeping = true;
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
	isSleeping = false;
}

void SleepState::renderTopScreen(cpp3ds::Window& window)
{
}

void SleepState::renderBottomScreen(cpp3ds::Window& window)
{
}

bool SleepState::update(float delta)
{
	if (!isSleeping)
	{
		requestStackPop();
		clock.restart();
	}
	return false;
}

bool SleepState::processEvent(const cpp3ds::Event& event)
{
	requestStackPop();
	clock.restart();
	return false;
}

} // namespace FreeShop
