#include "TopInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <time.h>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

TopInformations::TopInformations()
{
	//Get the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	//Cool blinking effect
	if (atoi(tempSec) % 2 == 0)
		strftime(timeTextFmt, 12, "%d/%m %H %M", timeinfo);
	else
		strftime(timeTextFmt, 12, "%d/%m %H:%M", timeinfo);

	m_textClock.setString(timeTextFmt);
	m_textClock.useSystemFont();
	m_textClock.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textClock.setFillColor(Theme::primaryTextColor);
	else
		m_textClock.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textClock.setPosition(200.f, 0.f);
	m_textClock.setOrigin(m_textClock.getLocalBounds().width / 2, 0.f);
}

TopInformations::~TopInformations()
{

}

void TopInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Draw clock
	target.draw(m_textClock);

}

void TopInformations::update()
{
	//Update the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	//Cool blinking effect
	if (atoi(tempSec) % 2 == 0)
		strftime(timeTextFmt, 12, "%d/%m %H %M", timeinfo);
	else
		strftime(timeTextFmt, 12, "%d/%m %H:%M", timeinfo);

	m_textClock.setString(timeTextFmt);
}

} // namespace FreeShop
