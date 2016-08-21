#ifndef FREESHOP_SETTINGS_HPP
#define FREESHOP_SETTINGS_HPP

#include <Gwen/Controls.h>
#include <Gwen/Controls/RadioButtonController.h>
#include <Gwen/Input/cpp3ds.h>
#include <Gwen/Skins/TexturedBase.h>
#include <TweenEngine/Tweenable.h>
#include <cpp3ds/System/Vector2.hpp>
#include "../States/State.hpp"


namespace FreeShop
{
	namespace GUI
	{
		class Settings: public Gwen::Event::Handler, public TweenEngine::Tweenable
		{
		public:
			static const int POSITION_XY = 1;

			Settings(Gwen::Skin::TexturedBase *skin, State *state);
			~Settings();

			bool processEvent(const cpp3ds::Event &event);
			bool update(float delta);
			void render();

			void setPosition(const cpp3ds::Vector2f &position);
			const cpp3ds::Vector2f &getPosition() const;

			void saveToConfig();
			void loadConfig();

			virtual int getValues(int tweenType, float *returnValues);
			virtual void setValues(int tweenType, float *newValues);

		private:
			void updateEnabledStates();

			Gwen::Controls::ScrollControl *addFilterPage(const std::string &name);
			void fillFilterRegions(Gwen::Controls::Base *parent);
			void fillFilterGenres(Gwen::Controls::Base *parent);
			void fillFilterPlatforms(Gwen::Controls::Base *parent);
			void fillFilterLanguages(Gwen::Controls::Base *parent);

			void fillSortPage(Gwen::Controls::Base *page);
			void fillUpdatePage(Gwen::Controls::Base *page);
			void fillOtherPage(Gwen::Controls::Base *page);

			// Event Callback functions
			void selectAll(Gwen::Controls::Base* control);
			void selectNone(Gwen::Controls::Base* control);

			bool m_ignoreCheckboxChange;
			void filterCheckboxChanged(Gwen::Controls::Base* control);
			void filterRegionCheckboxChanged(Gwen::Controls::Base* control);

			void updateUrlSelected(Gwen::Controls::Base *combobox);
			void updateQrClicked(Gwen::Controls::Base *button);
			void updateKeyboardClicked(Gwen::Controls::Base *button);
			void updateUrlDeleteClicked(Gwen::Controls::Base *button);
			void updateCheckUpdateClicked(Gwen::Controls::Base *button);

			void updateEnabledState(Gwen::Controls::Base* control);
			void updateSorting(Gwen::Controls::Base* control);

		private:
			cpp3ds::Vector2f m_position;

			State *m_state;
			Gwen::Input::cpp3dsInput m_input;
			Gwen::Controls::Canvas *m_canvas;
			Gwen::Controls::TabControl *m_tabControl;
			Gwen::Controls::TabControl *m_filterTabControl;

			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterGenreCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterPlatformCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterRegionCheckboxes;
			std::vector<Gwen::Controls::CheckBoxWithLabel*> m_filterLanguageCheckboxes;

			// Sort controls
			Gwen::Controls::RadioButtonController *m_radioSortType;
			Gwen::Controls::RadioButtonController *m_radioSortDirection;

			// Update controls
			Gwen::Controls::CheckBoxWithLabel *m_checkboxAutoUpdate;
			Gwen::Controls::CheckBoxWithLabel *m_checkboxDownloadKeys;
			Gwen::Controls::ComboBox *m_comboBoxUrls;
			Gwen::Controls::Button *m_buttonUrlQr, *m_buttonUrlKeyboard, *m_buttonUrlDelete;
			Gwen::Controls::Button *m_buttonUpdate;
			Gwen::Controls::Label *m_labelLastUpdated;
		};
	}
}


#endif //FREESHOP_SETTINGS_HPP
