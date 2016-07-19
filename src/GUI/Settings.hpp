#ifndef FREESHOP_SETTINGS_HPP
#define FREESHOP_SETTINGS_HPP

#include <Gwen/Controls.h>
#include <Gwen/Input/cpp3ds.h>
#include <Gwen/Skins/TexturedBase.h>
#include <TweenEngine/Tweenable.h>
#include <cpp3ds/System/Vector2.hpp>


namespace FreeShop
{
	namespace GUI
	{
		class Settings: public Gwen::Event::Handler, public TweenEngine::Tweenable
		{
		public:
			static const int POSITION_XY = 1;

			Settings(Gwen::Skin::TexturedBase *skin);
			~Settings();

			bool processEvent(const cpp3ds::Event &event);
			bool update(float delta);
			void render();

			void setPosition(const cpp3ds::Vector2f &position);
			const cpp3ds::Vector2f &getPosition() const;

			virtual int getValues(int tweenType, float *returnValues);
			virtual void setValues(int tweenType, float *newValues);

		private:
			void selectAll(Gwen::Controls::Base* control);
			void selectNone(Gwen::Controls::Base* control);

			bool m_ignoreCheckboxChange;
			void filterCheckboxChanged(Gwen::Controls::Base* control);
			void filterRegionCheckboxChanged(Gwen::Controls::Base* control);

			Gwen::Controls::ScrollControl *addFilterPage(const std::string &name);
			void fillRegions(Gwen::Controls::Base *page);
			void fillGenres(Gwen::Controls::Base *page);
			void fillPlatforms(Gwen::Controls::Base *page);

			cpp3ds::Vector2f m_position;

			Gwen::Input::cpp3dsInput m_input;
			Gwen::Controls::Canvas *m_canvas;
			Gwen::Controls::Button *m_button;
			Gwen::Controls::ScrollControl *m_scrollBox;
			Gwen::Controls::TabControl *m_tabControl;

			Gwen::Controls::TabControl *m_filterTabControl;

			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterGenreCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterPlatformCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterRegionCheckboxes;
		};
	}
}


#endif //FREESHOP_SETTINGS_HPP
