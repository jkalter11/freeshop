#include <iostream>
#include <string>
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/Sleep.hpp>
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Installer.hpp"

namespace FreeShop {


DownloadQueue::~DownloadQueue()
{
	// Cancel all downloads
	for (auto& download : m_downloads)
		delete download.second;
}


void DownloadQueue::addDownload(AppItem* app)
{
	std::string url = "http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/" + app->getTitleId() + "/tmd";

	Download* download = new Download(url);
	download->fillFromAppItem(app);
	download->setPosition(3.f, 240.f);

	std::vector<char> buf;
	cpp3ds::Clock clock;
	float count = 0;
	cpp3ds::Uint64 titleId = strtoull(app->getTitleId().c_str(), 0, 16);
	Installer *installer = new Installer(titleId);
	cpp3ds::Uint64 titleFileSize = app->getFilesize();
	cpp3ds::Uint64 totalProcessed = 0;
	size_t fileSize = 0;
	int fileIndex = 0;

	// TMD values
	cpp3ds::Uint16 contentCount;
	cpp3ds::Uint16 titleVersion;
	std::vector<cpp3ds::Uint16> contentIndices;

	download->setDataCallback([=](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response) mutable
	{
		if (fileSize == 0) {
			std::string length = response.getField("Content-Length");
			if (!length.empty())
				fileSize = atoi(length.c_str());
		}

		const char *bufdata = reinterpret_cast<const char*>(data);

		if (fileIndex == 0)
		{
			buf.insert(buf.end(), bufdata, bufdata + len);

			if (processed == fileSize && fileSize != 0)
			{
				static int dataOffsets[6] = {0x240, 0x140, 0x80, 0x240, 0x140, 0x80};
				char sigType = buf[0x3];

				titleVersion = *(cpp3ds::Uint16*)&buf[dataOffsets[sigType] + 0x9C];
				contentCount = __builtin_bswap16(*(cpp3ds::Uint16*)&buf[dataOffsets[sigType] + 0x9E]);
				std::cout << _("%016llX", titleId).toAnsiString() << ": " << titleVersion << std::endl;

				for (int i = 0; i < contentCount; ++i)
				{
					char *contentChunk = &buf[dataOffsets[sigType] + 0x9C4 + (i * 0x30)];
					cpp3ds::Uint32 contentId = __builtin_bswap32(*(cpp3ds::Uint32*)&contentChunk[0]);
					cpp3ds::Uint16 contentIndex = __builtin_bswap16(*(cpp3ds::Uint16*)&contentChunk[4]);
					cpp3ds::Uint64 contentSize = __builtin_bswap64(*(cpp3ds::Uint64*)&contentChunk[8]);
					std::cout << "id:" << contentId << " index:" << contentIndex << " size:" << contentSize << std::endl;

					contentIndices.push_back(contentIndex);
					download->pushUrl(_("http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/%08lX", titleId, contentId));
				}

				if (Installer::installTicket(titleId, titleVersion))
					std::cout << "ticket installed" << std::endl;
				else
					std::cout << "ticket failed!" << std::endl;

				installer->start();
				if (!installer->installTmd(&buf[0], dataOffsets[sigType] + 0x9C4 + (contentCount * 0x30)))
					return false;
				if (!installer->finalizeTmd())
					return false;

				buf.clear();
				fileSize = 0;
				fileIndex++;
			}
		}
		else // is a Content file
		{
			totalProcessed += len;
			download->setProgress(static_cast<double>(totalProcessed) / titleFileSize);

			if (!installer->installContent(data, len, contentIndices[fileIndex-1]))
				return false;

			if (processed == fileSize && fileSize != 0)
			{
				if (!installer->finalizeContent())
					return false;
				fileSize = 0;
				fileIndex++;
			}
		}


		// Handle status message and counters
		count += len;
		if (clock.getElapsedTime() > cpp3ds::seconds(1.f))
		{
			download->setProgressMessage(_("Installing... %.1f%% (%.1fkb/s)",
										   download->getProgress() * 100.f,
										   count / clock.getElapsedTime().asSeconds() / 1024.f));
			count = 0;
			clock.restart();
		}

		return true;
	});

	download->setFinishCallback([=](bool canceled, bool failed) mutable
	{
		download->setProgress(1.f);

		cpp3ds::String notification = app->getTitle();

		if (canceled && !failed)
		{
			download->setProgressMessage("Canceled");
		}
		else if (!failed && installer->commit())
		{
			notification.insert(0, "Completed: ");
			Notification::spawn(notification);
			download->setProgressMessage("Installed!");
		}
		else
		{
			notification.insert(0, "Failed: ");
			Notification::spawn(notification);
			download->setProgressMessage("Failed?");
		}

		delete installer;
		// Start next download in queue
		for (auto it = m_downloads.begin(); it < m_downloads.end(); ++it)
			if (it->first == app && ++it != m_downloads.end())
				it->second->start();
	});

	download->setProgressMessage("Queued");

	cpp3ds::String s = app->getTitle();
	s.insert(0, "Queued install: ");
	Notification::spawn(s);

	// Only start download if none others are running
	bool busy = false;
	for (auto& dl : m_downloads)
		if (dl.second->isActive()) {
			busy = true;
			break;
		}
	if (!busy)
		download->start();

	m_downloads.emplace_back(std::make_pair(app, download));
	realign();
}


void DownloadQueue::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& download : m_downloads)
	{
		target.draw(*download.second, states);
	}
}


void DownloadQueue::cancelDownload(AppItem *app)
{
	for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
	{
		if (it->first == app)
		{
//			Notification::spawn(_("Canceled: %s", app->getTitle().c_str()));
			it->second->cancel(false);
			break;
		}
	}
}


bool DownloadQueue::isDownloading(AppItem *app)
{
	for (auto& download : m_downloads)
	{
		if (download.first == app && download.second->getProgress() < 0.5f)
			return true;
	}
	return false;
}


bool DownloadQueue::processEvent(const cpp3ds::Event &event)
{
	for (auto& download : m_downloads)
		download.second->processEvent(event);
	return true;
}

void DownloadQueue::realign()
{
	for (int i = 0; i < m_downloads.size(); ++i)
	{
		TweenEngine::Tween::to(*m_downloads[i].second, util3ds::TweenSprite::POSITION_Y, 0.2f)
			.target(33.f + i * 35.f)
			.start(m_tweenManager);
	};
}

void DownloadQueue::update(float delta)
{
	// Remove downloads marked for delete
	bool changed = false;
	for (auto it = m_downloads.begin(); it != m_downloads.end();)
	{
		if (it->second->markedForDelete())
		{
			delete it->second;
			m_downloads.erase(it);
			changed = true;
		}
		else
			++it;
	}

	if (changed)
		realign();

	m_tweenManager.update(delta);
}

size_t DownloadQueue::getCount()
{
	return m_downloads.size();
}

size_t DownloadQueue::getActiveCount()
{
	size_t count = 0;
	for (auto& download : m_downloads)
		if (download.second->getProgress() < 1.f)
			++count;
	return count;
}

DownloadQueue &DownloadQueue::getInstance()
{
	static DownloadQueue downloadQueue;
	return downloadQueue;
}

} // namespace FreeShop
