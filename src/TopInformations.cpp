#include "TopInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "Util.hpp"
#include "Config.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <TweenEngine/Tween.h>
#include <time.h>
#include <stdlib.h>

namespace FreeShop {

TopInformations::TopInformations()
: m_batteryPercent(-1)
, m_textClockMode(1)
, m_isCollapsed(true)
{
	//Start the clock
	m_switchClock.restart();
	
	//Get the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	strftime(timeTextFmt, 12, "%H %M", timeinfo);

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

	//Define texts position
	m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), -50.f);
	
	//Two points in clock
	m_textTwoPoints = m_textClock;
	m_textTwoPoints.setString(":");
	m_textTwoPoints.setCharacterSize(14);
	m_textTwoPoints.setPosition((m_textClock.getPosition().x + (m_textClock.getLocalBounds().width / 2)) - 3, m_textClock.getPosition().y);

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

	TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(4.f).delay(0.5f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = false;}).start(m_tweenManager);
	TWEEN_IN(m_textTwoPoints, 4.f);
	TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
	TWEEN_IN_X(m_signalIcon, 2.f);

	//Two points animation
	TweenEngine::Tween::to(m_textTwoPoints, util3ds::TweenText::FILL_COLOR_ALPHA, 1.f).target(0).repeatYoyo(-1, 0).start(m_tweenManager);
}

TopInformations::~TopInformations()
{

}

void TopInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Draw clock
	target.draw(m_textClock);
	
	//Draw only the two points if the clock text is in clock mode
	if (m_textClockMode == 1)
		target.draw(m_textTwoPoints);

	//Draw battery & text
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

	strftime(timeTextFmt, 12, "%H %M", timeinfo);

	m_tweenManager.update(delta);

	//Update battery and signal icons
	if (skipFrames >= 60) {
		skipFrames = 1;
#ifndef EMULATION
		//Update battery icon and percentage
		cpp3ds::Uint8 batteryChargeState = 0;
		cpp3ds::Uint8 isAdapterPlugged = 0;
		cpp3ds::Uint8 batteryLevel = 0;
		cpp3ds::Uint8 batteryPercentHolder = 0;
		int batteryPercent = -1;
		std::string batteryPath;

		if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState)
        		batteryPath = "battery_charging.png";
        	else if (R_SUCCEEDED(PTMU_GetAdapterState(&isAdapterPlugged)) && isAdapterPlugged)
        		batteryPath = "battery_charging_full.png";
    		else if (R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel))) {
        		batteryPath = "battery" + std::to_string(batteryLevel - 1) + ".png";
        		if (R_SUCCEEDED(MCUHWC_GetBatteryLevel(&batteryPercentHolder)))
        			batteryPercent = static_cast<int>(batteryPercentHolder);
    		} else
        		batteryPath = "battery0.png";
        		
        	m_batteryPercent = batteryPercent;
        	std::cout << m_batteryPercent << std::endl;

		std::string themedBatteryPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + batteryPath);

		if (pathExists(themedBatteryPath.c_str(), false))
			m_textureBattery.loadFromFile(themedBatteryPath);
		else
			m_textureBattery.loadFromFile("images/" + batteryPath);

		//Update signal icon
		uint32_t wifiStatus = 0;
		std::string signalPath;

		if (R_SUCCEEDED(ACU_GetWifiStatus(&wifiStatus)) && wifiStatus)
        		signalPath = "wifi" + std::to_string(osGetWifiStrength()) + ".png";
    		else
        		signalPath = "wifi_disconnected.png";

		std::string themedSignalPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + signalPath);

		if (pathExists(themedSignalPath.c_str(), false))
			m_textureSignal.loadFromFile(themedSignalPath);
		else
			m_textureSignal.loadFromFile("images/" + signalPath);
#else
		//Update battery icon
		std::string batteryPath = "battery" + std::to_string(rand() % 5) + ".png";
		std::string themedBatteryPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + batteryPath);
		
		m_batteryPercent = rand() % 101;

		if (pathExists(themedBatteryPath.c_str(), false))
			m_textureBattery.loadFromFile(themedBatteryPath);
		else
			m_textureBattery.loadFromFile("images/" + batteryPath);

		//Update signal icon
		std::string signalPath = "wifi" + std::to_string(rand() % 4) + ".png";
		std::string themedSignalPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + signalPath);

		if (pathExists(themedSignalPath.c_str(), false))
			m_textureSignal.loadFromFile(themedSignalPath);
		else
			m_textureSignal.loadFromFile("images/" + signalPath);
#endif
		//Change the mode of the clock if enough time passed
		if (m_switchClock.getElapsedTime() >= cpp3ds::seconds(10)) {
			//Reset the clock
			m_switchClock.restart();
			
			//Switch mode
			if (m_textClockMode == 1 && Config::get(Config::ShowBattery).GetBool()) {
				//Battery percentage mode
				m_textClockMode = 2;
				TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.4f).target(0.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {m_textClock.setString(std::to_string(m_batteryPercent) + "%"); if (!m_isCollapsed) m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), 4.f);}).start(m_tweenManager);
				TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.4f).target(255.f).delay(0.5f).start(m_tweenManager);
			} else if (m_textClockMode != 1) {
				//Clock mode
				TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.4f).target(0.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {m_textClock.setString(timeTextFmt); if (!m_isCollapsed) m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), 4.f);}).start(m_tweenManager);
				TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.4f).target(255.f).delay(0.5f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_textClockMode = 1;}).start(m_tweenManager);
			}
		}
	
	}

	skipFrames++;
}

void TopInformations::setCollapsed(bool collapsed)
{
	if (collapsed)
		m_isCollapsed = collapsed;

	if (collapsed) {
		TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(-50.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = true;}).start(m_tweenManager);
		TWEEN_IN_NOWAIT(m_textTwoPoints, -50.f);
		TWEEN_IN_X_NOWAIT(m_batteryIcon, 370.f - m_textureBattery.getSize().x);
		TWEEN_IN_X_NOWAIT(m_signalIcon, -50.f);
	} else {
		TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(4.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = false;}).start(m_tweenManager);
		TWEEN_IN(m_textTwoPoints, 4.f);
		TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
		TWEEN_IN_X(m_signalIcon, 2.f);
	}
}

#ifndef EMULATION
Result TopInformations::PTMU_GetAdapterState(u8 *out)
{
	Handle serviceHandle = 0;
	Result result = srvGetServiceHandle(&serviceHandle, "ptm:u");
	
	u32* ipc = getThreadCommandBuffer();
	ipc[0] = 0x50000;
	Result ret = svcSendSyncRequest(serviceHandle);
	
	svcCloseHandle(serviceHandle);
	
	if(ret < 0) return ret;
	
	*out = ipc[2];
	return ipc[1];
}

// Saw on the awesome 3DShell file explorer homebrew
Result TopInformations::MCUHWC_GetBatteryLevel(u8 *out)
{
	Handle serviceHandle = 0;
	Result result = srvGetServiceHandle(&serviceHandle, "mcu::HWC");
	
	u32* ipc = getThreadCommandBuffer();
	ipc[0] = 0x50000;
	Result ret = svcSendSyncRequest(serviceHandle);
	
	svcCloseHandle(serviceHandle);
	
	if(ret < 0) return ret;
	
	*out = ipc[2];
	return ipc[1];
}
#endif

} // namespace FreeShop
