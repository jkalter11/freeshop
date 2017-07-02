#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cmath>
#include "DialogState.hpp"
#include "../AssetManager.hpp"
#include "../Theme.hpp"
#include "SleepState.hpp"
#include "BrowseState.hpp"

namespace FreeShop {

DialogState::DialogState(StateStack &stack, Context &context, StateCallback callback)
: State(stack, context, callback)
, m_isClosing(false)
{
	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color(0, 0, 0, 0));

	if (!Theme::isButtonRadius9Themed) {
		cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/button-radius.9.png");
		m_background.setTexture(&texture);
	} else {
		cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/button-radius.9.png");
		m_background.setTexture(&texture);
	}

	m_background.setSize(cpp3ds::Vector2f(280.f, 200.f));
	m_background.setPosition(20.f, 20.f);
	m_background.setColor(cpp3ds::Color(255, 255, 255, 128));

	m_message.setCharacterSize(14);
	m_message.setFillColor(cpp3ds::Color::Transparent);
	m_message.useSystemFont();
	m_message.setPosition(160.f, 100.f);

	m_buttonOkBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	m_buttonOkBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 0));
	m_buttonOkBackground.setOutlineThickness(1.f);
	m_buttonOkBackground.setPosition(165.f, 180.f);
	m_buttonOkBackground.setFillColor(cpp3ds::Color(255, 255, 255, 0));

	m_buttonCancelBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	m_buttonCancelBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 0));
	m_buttonCancelBackground.setOutlineThickness(1.f);
	m_buttonCancelBackground.setPosition(40.f, 180.f);
	m_buttonCancelBackground.setFillColor(cpp3ds::Color(255, 255, 255, 0));

	m_buttonOkText.setString(_("\uE000 Ok"));
	m_buttonOkText.setCharacterSize(14);
	m_buttonOkText.setFillColor(cpp3ds::Color(3, 169, 244, 0));
	m_buttonOkText.useSystemFont();
	m_buttonOkText.setPosition(m_buttonOkBackground.getPosition().x + m_buttonOkBackground.getGlobalBounds().width / 2, m_buttonOkBackground.getPosition().y + m_buttonOkBackground.getGlobalBounds().height / 2);
	m_buttonOkText.setOrigin(m_buttonOkText.getGlobalBounds().width / 2, m_buttonOkText.getGlobalBounds().height / 1.3f);

	m_buttonCancelText = m_buttonOkText;
	m_buttonCancelText.setString(_("\uE001 Cancel"));
	m_buttonCancelText.setCharacterSize(12);
	m_buttonCancelText.setPosition(m_buttonCancelBackground.getPosition().x + m_buttonCancelBackground.getGlobalBounds().width / 2, m_buttonCancelBackground.getPosition().y + m_buttonCancelBackground.getGlobalBounds().height / 2);
	m_buttonCancelText.setOrigin(m_buttonCancelText.getGlobalBounds().width / 2, m_buttonCancelText.getGlobalBounds().height / 1.5f);

	m_bottomView.setCenter(cpp3ds::Vector2f(160.f, 120.f));
	m_bottomView.setSize(cpp3ds::Vector2f(320.f * 0.5f, 240.f * 0.5f));

	cpp3ds::String tmp;
	Event event = {GetText, &tmp};
	if (runCallback(&event)) {
		auto message = reinterpret_cast<cpp3ds::String*>(event.data);
		m_message.setString(*message);
	} else
		m_message.setString(_("Are you sure you want\nto continue?"));
	m_message.setOrigin(std::round(m_message.getLocalBounds().width / 2),
						std::round(m_message.getLocalBounds().height / 2));

#define TWEEN_IN(obj) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.2f).target(255.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.2f).target(150.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.2f).target(255.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_buttonOkBackground, m_buttonOkBackground.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_buttonCancelBackground, m_buttonCancelBackground.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);
	TWEEN_IN(m_message);
	TWEEN_IN(m_buttonOkText);
	TWEEN_IN(m_buttonCancelText);
	TweenEngine::Tween::to(m_bottomView, m_bottomView.SIZE_XY, 0.2f).target(320.f, 240.f).start(m_tweenManager);
}

void DialogState::renderTopScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);
}

void DialogState::renderBottomScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);

	window.setView(m_bottomView);

	window.draw(m_background);
	window.draw(m_message);
	window.draw(m_buttonOkBackground);
	window.draw(m_buttonOkText);
	window.draw(m_buttonCancelBackground);
	window.draw(m_buttonCancelText);

	window.setView(window.getDefaultView());
}

bool DialogState::update(float delta)
{
	m_tweenManager.update(delta);
	return false;
}

bool DialogState::processEvent(const cpp3ds::Event &event)
{
	BrowseState::clockDownloadInactivity.restart();
	SleepState::clock.restart();
	if (m_isClosing)
		return false;

	bool triggerResponse = false;
	bool accepted = false;

	if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y))
			triggerResponse = accepted = true;
		else if (m_buttonCancelBackground.getGlobalBounds().contains(event.touch.x, event.touch.y))
			triggerResponse = true;
	}
	else if (event.type == cpp3ds::Event::KeyPressed)
	{
		if (event.key.code == cpp3ds::Keyboard::A)
			triggerResponse = accepted = true;
		else if (event.key.code == cpp3ds::Keyboard::B)
			triggerResponse = true;
	}

#define TWEEN_OUT(obj) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);

	if (triggerResponse)
	{
		Event evt = {Response, &accepted};

		// Highlight selected button
		if (accepted) {
			m_buttonOkBackground.setFillColor(m_buttonOkBackground.getOutlineColor());
			m_buttonCancelBackground.setOutlineThickness(0);
		} else {
			m_buttonCancelBackground.setFillColor(m_buttonCancelBackground.getOutlineColor());
			m_buttonOkBackground.setOutlineThickness(0);
		}

		if (runCallback(&evt))
		{
			m_isClosing = true;
			m_tweenManager.killAll();

			TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);
			TWEEN_OUT(m_message);
			TWEEN_OUT(m_buttonOkBackground);
			TWEEN_OUT(m_buttonCancelBackground);
			TWEEN_OUT(m_buttonOkText);
			TWEEN_OUT(m_buttonCancelText);
			TweenEngine::Tween::to(m_bottomView, m_bottomView.SIZE_XY, 0.2f).target(320.f * 2.f, 240.f * 2.f).start(m_tweenManager);
			TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.3f).target(0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
					requestStackPop();
				})
				.delay(0.2f).start(m_tweenManager);
		}
	}
	return false;
}


} // namespace FreeShop
