#include "InstalledList.hpp"

namespace FreeShop {

InstalledList::InstalledList()
{
	//
}

InstalledList &InstalledList::getInstance()
{
	static InstalledList installedList;
	return installedList;
}

void InstalledList::refresh()
{
	std::vector<cpp3ds::Uint64> titleIds;
#ifdef EMULATION
	// some hardcoded title IDs for testing
	titleIds.push_back(0x00040000000edf00);
	titleIds.push_back(0x0004000000030100);
	titleIds.push_back(0x0004000000030800);
	titleIds.push_back(0x0004000000041700);
#else
	u32 titleCount;
	AM_GetTitleCount(MEDIATYPE_SD, &titleCount);
	titleIds.resize(titleCount);
	AM_GetTitleList(nullptr, MEDIATYPE_SD, titleIds.size(), &titleIds[0]);
#endif

	int i = 0;
	for (auto& titleId : titleIds)
	{
		try
		{
			std::unique_ptr<InstalledItem> item(new InstalledItem(titleId));
			item->setPosition(0.f, 16.f * i + 30.f);
			m_installedItems.emplace_back(std::move(item));
			i++;
		}
		catch (int e)
		{
			//
		}
	}
}

void InstalledList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& item : m_installedItems)
	{
		target.draw(*item, states);
	}
}

void InstalledList::update(float delta)
{
	m_tweenManager.update(delta);
}

bool InstalledList::processEvent(const cpp3ds::Event &event)
{
	return false;
}


} // namespace FreeShop
