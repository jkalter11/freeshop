#include "FreeShop.hpp"
#include "States/SleepState.hpp"

#ifndef EMULATION
namespace {

aptHookCookie cookie;

void aptHookFunc(APT_HookType hookType, void *param)
{
	switch (hookType) {
		case APTHOOK_ONSUSPEND:
			if (FreeShop::SleepState::isSleeping && R_SUCCEEDED(gspLcdInit()))
			{
				GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
				gspLcdExit();
			}
			break;
		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			FreeShop::SleepState::isSleeping = false;
			FreeShop::SleepState::clock.restart();
			break;
		default:
			break;
	}
}

}
#endif


int main(int argc, char** argv)
{
#ifndef NDEBUG
	// Console for reading stdout
	cpp3ds::Console::enable(cpp3ds::BottomScreen, cpp3ds::Color::Black);
#endif

	cpp3ds::Service::enable(cpp3ds::Audio);
	cpp3ds::Service::enable(cpp3ds::Network);
	cpp3ds::Service::enable(cpp3ds::SSL);
	cpp3ds::Service::enable(cpp3ds::Httpc);
	cpp3ds::Service::enable(cpp3ds::AM);

#ifndef EMULATION
	aptHook(&cookie, aptHookFunc, nullptr);
	AM_InitializeExternalTitleDatabase(false);
#endif

	FreeShop::FreeShop game;
	game.run();
	return 0;
}
