#include "FreeShop.hpp"
#include "DownloadQueue.hpp"
#include "TitleKeys.hpp"
#include "States/BrowseState.hpp"
#include "States/SleepState.hpp"
#include "States/SyncState.hpp"

#ifndef EMULATION
#include <3ds.h>
#endif

#ifndef EMULATION
namespace {

aptHookCookie cookie;

void aptHookFunc(APT_HookType hookType, void *param)
{
	switch (hookType) {
		case APTHOOK_ONSUSPEND:
			if (FreeShop::SleepState::isSleeping && R_SUCCEEDED(gspLcdInit()))
			{
				GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_TOP);
				gspLcdExit();
			}
			// Fall through
		case APTHOOK_ONSLEEP:
			FreeShop::DownloadQueue::getInstance().suspend();
			break;
		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			FreeShop::SleepState::isSleeping = false;
			FreeShop::SleepState::clock.restart();
			FreeShop::BrowseState::clockDownloadInactivity.restart();
			FreeShop::DownloadQueue::getInstance().resume();
			break;
		case APTHOOK_ONEXIT:
			FreeShop::SyncState::exitRequired = true;
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
	cpp3ds::Service::enable(cpp3ds::Config);
	cpp3ds::Service::enable(cpp3ds::Network);
	cpp3ds::Service::enable(cpp3ds::SSL);
	cpp3ds::Service::enable(cpp3ds::Httpc);
	cpp3ds::Service::enable(cpp3ds::AM);

#ifndef EMULATION
	aptHook(&cookie, aptHookFunc, nullptr);
	AM_InitializeExternalTitleDatabase(false);
	aptSetSleepAllowed(true);
#endif
	srand(time(NULL));
	auto game = new FreeShop::FreeShop();
	game->run();
	FreeShop::DownloadQueue::getInstance().suspend();
	FreeShop::DownloadQueue::getInstance().save();
#ifndef EMULATION
	nimsExit();
	amExit();
	ptmuExit();
	acExit();
	ptmSysmExit();
	newsExit();
	
	if (FreeShop::g_requestShutdown) {
		//Init the services and turn off the console
		ptmSysmInit();
		PTMSYSM_ShutdownAsync(0);
		ptmSysmExit();
	} else if (FreeShop::g_requestJump != 0) {
		//Var initialization
		Result res = 0;
		u8 hmac[0x20];
		memset(hmac, 0, sizeof(hmac));
		
		//Check on which media launch the title
		FS_MediaType mediaType = ((FreeShop::g_requestJump >> 32) == FreeShop::TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
		
		FS_CardType type;
		bool cardInserted;
		cardInserted = (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&cardInserted)) && cardInserted && R_SUCCEEDED(FSUSER_GetCardType(&type)) && type == CARD_CTR);
		if (cardInserted)
		{
			// Retry a bunch of times. When the card is newly inserted,
			// it sometimes takes a short while before title can be read.
			int retryCount = 100;
			u64 cardTitleId;
			while (retryCount-- > 0)
				if (R_SUCCEEDED(AM_GetTitleList(nullptr, MEDIATYPE_GAME_CARD, 1, &cardTitleId)))
				{
					try
					{
						if (cardTitleId == FreeShop::g_requestJump)
							mediaType = MEDIATYPE_GAME_CARD;
					}
					catch (int e)
					{
						//
					}
					break;
				}
				else
					cpp3ds::sleep(cpp3ds::milliseconds(5));
		}
		
		//Do the application jump
		if (R_SUCCEEDED(res = APT_PrepareToDoApplicationJump(0, FreeShop::g_requestJump, mediaType)))
			res = APT_DoApplicationJump(0, 0, hmac);
	}
#endif
	delete game;
	return 0;
}
