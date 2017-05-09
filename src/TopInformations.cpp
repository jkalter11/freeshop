#include "TopInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <time.h>
#include <stdlib.h>
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
		strftime(timeTextFmt, 12, "%H %M", timeinfo);
	else
		strftime(timeTextFmt, 12, "%H:%M", timeinfo);

	m_textClock.setString(timeTextFmt);
	m_textClock.useSystemFont();
	m_textClock.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textClock.setFillColor(Theme::primaryTextColor);
	else
		m_textClock.setFillColor(cpp3ds::Color(80, 80, 80, 255));

	//Battery icon
	if (fopen(FREESHOP_DIR "/theme/images/battery0.png", "rb"))
		m_textureBattery.loadFromFile(FREESHOP_DIR "/theme/images/battery0.png");
	else
		m_textureBattery.loadFromFile("images/battery0.png");

	m_batteryIcon.setPosition(370.f - m_textureBattery.getSize().x, 5.f);
	m_batteryIcon.setTexture(m_textureBattery, true);

	//Signal icon
	if (fopen(FREESHOP_DIR "/theme/images/wifi0.png", "rb"))
		m_textureSignal.loadFromFile(FREESHOP_DIR "/theme/images/wifi0.png");
	else
		m_textureSignal.loadFromFile("images/wifi0.png");

	m_signalIcon.setPosition(-50.f, 5.f);
	m_signalIcon.setTexture(m_textureSignal, true);

	//Define clock position
	m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), -50.f);

	//Used for frame skipping in battery and signal icons updates
	skipFrames = 60;

#define TWEEN_IN(obj, posY) \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.6f).target(posY).delay(0.5f).start(m_tweenManager);

#define TWEEN_IN_NOWAIT(obj, posY) \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.6f).target(posY).start(m_tweenManager);

#define TWEEN_IN_X(obj, posX) \
	TweenEngine::Tween::to(obj, obj.POSITION_X, 0.6f).target(posX).delay(0.5f).start(m_tweenManager);

#define TWEEN_IN_X_NOWAIT(obj, posX) \
	TweenEngine::Tween::to(obj, obj.POSITION_X, 0.6f).target(posX).start(m_tweenManager);

	TWEEN_IN(m_textClock, 4.f);
	TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
	TWEEN_IN_X(m_signalIcon, 2.f);
}

TopInformations::~TopInformations()
{

}

void TopInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Draw clock
	target.draw(m_textClock);

	//Draw battery
	target.draw(m_batteryIcon);

	//Draw signal
	target.draw(m_signalIcon);

}

void TopInformations::update(float delta)
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
		strftime(timeTextFmt, 12, "%H %M", timeinfo);
	else
		strftime(timeTextFmt, 12, "%H:%M", timeinfo);

	m_textClock.setString(timeTextFmt);
	m_tweenManager.update(delta);

	//Update battery and signal icons
	if (skipFrames >= 60) {
		skipFrames = 1;
#ifndef EMULATION
		//Update battery icon
		cpp3ds::Uint8 batteryChargeState = 0;
		cpp3ds::Uint8 batteryLevel = 0;
		std::string batteryPath;

		if(R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState)
        		batteryPath = "battery_charging.png";
    		else if(R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel)))
        		batteryPath = "battery" + std::to_string(batteryLevel - 1) + ".png";
    		else
        		batteryPath = "battery0.png";

		std::string themedBatteryPath = FREESHOP_DIR "/theme/images/" + batteryPath;

		if (fopen(themedBatteryPath.c_str(), "rb"))
			m_textureBattery.loadFromFile(FREESHOP_DIR "/theme/images/" + batteryPath);
		else
			m_textureBattery.loadFromFile("images/" + batteryPath);

		//Update signal icon
		uint32_t wifiStatus = 0;
		std::string signalPath;

		if(R_SUCCEEDED(ACU_GetWifiStatus(&wifiStatus)) && wifiStatus)
        		signalPath = "wifi" + std::to_string(osGetWifiStrength()) + ".png";
    		else
        		signalPath = "wifi_disconnected.png";

		std::string themedSignalPath = FREESHOP_DIR "/theme/images/" + signalPath;

		if (fopen(themedSignalPath.c_str(), "rb"))
			m_textureSignal.loadFromFile(FREESHOP_DIR "/theme/images/" + signalPath);
		else
			m_textureSignal.loadFromFile("images/" + signalPath);
#else
		//Update battery icon
		std::string batteryPath = "battery" + std::to_string(rand() % 5) + ".png";
		std::string themedBatteryPath = FREESHOP_DIR "/theme/images/" + batteryPath;

		if (fopen(themedBatteryPath.c_str(), "rb"))
			m_textureBattery.loadFromFile(FREESHOP_DIR "/theme/images/" + batteryPath);
		else
			m_textureBattery.loadFromFile("images/" + batteryPath);

		//Update signal icon
		std::string signalPath = "wifi" + std::to_string(rand() % 4) + ".png";
		std::string themedSignalPath = FREESHOP_DIR "/theme/images/" + signalPath;

		if (fopen(themedSignalPath.c_str(), "rb"))
			m_textureSignal.loadFromFile(FREESHOP_DIR "/theme/images/" + signalPath);
		else
			m_textureSignal.loadFromFile("images/" + signalPath);
#endif
	}

	skipFrames++;
}

void TopInformations::setCollapsed(bool collapsed)
{
	if (collapsed) {
		TWEEN_IN_NOWAIT(m_textClock, -50.f);
		TWEEN_IN_X_NOWAIT(m_batteryIcon, 370.f - m_textureBattery.getSize().x);
		TWEEN_IN_X_NOWAIT(m_signalIcon, -50.f);
	} else {
		TWEEN_IN(m_textClock, 4.f);
		TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
		TWEEN_IN_X(m_signalIcon, 2.f);
	}
}

} // namespace FreeShop
