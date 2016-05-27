#include "FreeShop.hpp"

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
	AM_InitializeExternalTitleDatabase(false);
#endif

	FreeShop::FreeShop game;
	game.run();
	return 0;
}
