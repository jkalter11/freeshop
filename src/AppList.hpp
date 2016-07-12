#ifndef FREESHOP_APPLIST_HPP
#define FREESHOP_APPLIST_HPP

#include <vector>
#include <string>
#include <memory>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/System/Thread.hpp>
#include "GUI/AppItem.hpp"
#include "RichText.hpp"


namespace FreeShop
{

class AppList : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	enum SortType {
		AlphaNumericAsc,
		AlphaNumericDesc,
	};

	static AppList &getInstance();

	~AppList();
	void refresh();
	void setSortType(SortType sortType);
	SortType getSortType() const;

	void setSelectedIndex(int index);
	int getSelectedIndex() const;

	size_t getCount() const;
	size_t getVisibleCount() const;

	GUI::AppItem* getSelected();
	std::vector<std::unique_ptr<GUI::AppItem>> &getList();

	void setCollapsed(bool collapsed);
	bool isCollapsed() const;

	void filterBySearch(const std::string& searchTerm, std::vector<util3ds::RichText> &textMatches);

	void update(float delta);

protected:
	AppList(std::string jsonFilename);
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	void sort();
	void resize();

private:
	SortType m_sortType;
	std::string m_jsonFilename;
	int m_selectedIndex;
	std::vector<std::shared_ptr<AppItem>> m_appItems;
	std::vector<std::unique_ptr<GUI::AppItem>> m_guiAppItems;
	std::vector<cpp3ds::Texture*> m_iconTextures;
	bool m_collapsed;
	TweenEngine::TweenManager m_tweenManager;
};

} // namespace FreeShop

#endif // FREESHOP_APPLIST_HPP
