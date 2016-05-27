#ifndef FREESHOP_DOWNLOADQUEUE_HPP
#define FREESHOP_DOWNLOADQUEUE_HPP

#include <bits/unique_ptr.h>
#include <cpp3ds/Window/Event.hpp>
#include <TweenEngine/TweenManager.h>
#include "Download.hpp"
#include "AppItem.hpp"

namespace FreeShop {

class DownloadQueue : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	~DownloadQueue();

	static DownloadQueue &getInstance();

	void addDownload(AppItem* app);
	void cancelDownload(AppItem* app);
	bool isDownloading(AppItem* app);

	size_t getCount();
	size_t getActiveCount();

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

protected:
	DownloadQueue() {}

	void realign();
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	std::vector<std::pair<AppItem*, Download*>> m_downloads;
	TweenEngine::TweenManager m_tweenManager;
};

} // namespace FreeShop

#endif // FREESHOP_DOWNLOADQUEUE_HPP
