#include <iostream>
#include <string>
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/Sleep.hpp>
#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/Lock.hpp>
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "AppList.hpp"

namespace FreeShop {


DownloadItem::DownloadItem(AppItem *appItem, Download *download, Installer *installer)
: appItem(appItem)
, download(download)
, installer(installer)
{

}

DownloadItem::~DownloadItem()
{
	if (installer)
		delete installer;
	if (download)
		delete download;
}


DownloadQueue::DownloadQueue()
: m_threadRefresh(&DownloadQueue::refresh, this)
, m_refreshEnd(false)
{
	load();
	m_threadRefresh.launch();
}

DownloadQueue::~DownloadQueue()
{
	m_refreshEnd = true;
	cpp3ds::Lock lock(m_mutexRefresh);
	m_downloads.clear();
}

void DownloadQueue::addDownload(AppItem* app, int contentIndex, float progress)
{
	cpp3ds::Lock lock(m_mutexRefresh);
	std::string url = "http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/" + app->getTitleId() + "/tmd";

	Download* download = new Download(url);
	download->fillFromAppItem(app);
	download->setPosition(3.f, 240.f);
	download->setSendTopCallback([this](Download *d){
		sendTop(d);
	});

	std::vector<char> buf;
	cpp3ds::Clock clock;
	float count = 0;
	size_t fileSize = 0;
	int fileIndex = 0;

	cpp3ds::Uint64 titleId = strtoull(app->getTitleId().c_str(), 0, 16);
	Installer *installer = new Installer(titleId, contentIndex);
	cpp3ds::Uint64 titleFileSize = app->getFilesize();
	cpp3ds::Uint64 totalProcessed = progress * titleFileSize;

	// Is resuming from saved queue
	bool isResuming = contentIndex >= 0;

	// TMD values
	cpp3ds::Uint16 contentCount;
	cpp3ds::Uint16 titleVersion;
	std::vector<cpp3ds::Uint16> contentIndices;

	download->setDataCallback([=](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response) mutable
	{
		if (fileSize == 0) {
			std::string length = response.getField("Content-Length");
			if (!length.empty())
				fileSize = strtoul(length.c_str(), 0, 10);
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

				if (isResuming)
					installer->resume();

				bool foundIndex = false; // For resuming via contentIndex arg
				for (int i = 0; i < contentCount; ++i)
				{
					char *contentChunk = &buf[dataOffsets[sigType] + 0x9C4 + (i * 0x30)];
					cpp3ds::Uint32 contentId = __builtin_bswap32(*(cpp3ds::Uint32*)&contentChunk[0]);
					cpp3ds::Uint16 contentIdx = __builtin_bswap16(*(cpp3ds::Uint16*)&contentChunk[4]);
					cpp3ds::Uint64 contentSize = __builtin_bswap64(*(cpp3ds::Uint64*)&contentChunk[8]);

					if (contentIdx == contentIndex)
						foundIndex = true;
					if (!isResuming || foundIndex)
					{
						contentIndices.push_back(contentIdx);
						download->pushUrl(_("http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/%08lX", titleId, contentId), (contentIdx == contentIndex) ? installer->getCurrentContentPosition() : 0);
					}
				}

				if (!isResuming)
				{
					// Check for cancel at each stage in case it changes
					if (!download->isCanceled())
					{
						download->setProgressMessage(_("Installing ticket..."));
						if (!installer->installTicket(titleVersion))
							return false;
					}
					if (!download->isCanceled() && !app->getSeed().empty())
					{
						download->setProgressMessage(_("Installing seed..."));
						if (!installer->installSeed(&app->getSeed()[0]))
							return false;
					}
					if (!download->isCanceled())
					{
						if (!installer->start())
							return false;
						download->setProgressMessage(_("Installing TMD..."));
						if (!installer->installTmd(&buf[0], dataOffsets[sigType] + 0x9C4 + (contentCount * 0x30)))
							return false;
						if (!installer->finalizeTmd())
							return false;
					}
				}

				buf.clear();
				fileSize = 0;
				fileIndex++;
			}
		}
		else // is a Content file
		{
			// Conditions indicate download issue (e.g. internet is down)
			// with either an empty length or one not 64-byte aligned
			if (len == 0 || len % 64 > 0)
			{
				suspend();
				m_refreshEnd = false; // Refresh to resume after suspension
				fileSize = 0;
				return true;
			}

			int oldIndex = installer->getCurrentContentIndex();
			if (!installer->installContent(data, len, contentIndices[fileIndex-1]))
			{
				if (download->getStatus() == Download::Suspended)
				{
					fileSize = 0;
					return true;
				}
				return false;
			}

			// Save index change to help recover queue from crash
			if (oldIndex != installer->getCurrentContentIndex())
				save();

			totalProcessed += len;
			download->setProgress(static_cast<double>(totalProcessed) / titleFileSize);

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
		download->setProgressMessage(_("Installing... %.1f%% (%.0f KB/s)",
									   download->getProgress() * 100.f,
									   count / clock.getElapsedTime().asSeconds() / 1024.f));
		if (clock.getElapsedTime() > cpp3ds::seconds(5.f))
		{
			count = 0;
			clock.restart();
		}

		return true;
	});

	download->setFinishCallback([=](bool canceled, bool failed) mutable
	{
		cpp3ds::String notification = app->getTitle();

		switch (download->getStatus())
		{
			case Download::Suspended:
				download->setProgressMessage(_("Suspended"));
				return;
			case Download::Finished:
				if (installer->commit())
				{
					notification.insert(0, _("Completed: "));
					Notification::spawn(notification);
					download->setProgressMessage(_("Installed"));
					break;
				}
				download->m_status = Download::Failed;
				// Fall through
			case Download::Failed:
				notification.insert(0, _("Failed: "));
				Notification::spawn(notification);
				if (installer->getErrorCode() != 0)
					download->setProgressMessage(installer->getErrorString());
				else
					download->setProgressMessage(_("Failed"));
				break;
			case Download::Canceled:
				break;
		}

		download->setProgress(1.f);

		// Reset CanSendTop state
		for (auto& download : m_downloads)
			download->download->m_canSendTop = true;
	});

	if (progress > 0.f)
	{
		download->setProgress(progress);
		download->setProgressMessage(_("Suspended"));
	}
	else
		download->setProgressMessage(_("Queued"));

	std::unique_ptr<DownloadItem> downloadItem(new DownloadItem(app, download, installer));
	m_downloads.emplace_back(std::move(downloadItem));
	realign();
}


void DownloadQueue::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& item : m_downloads)
	{
		target.draw(*item->download, states);
	}
}


void DownloadQueue::cancelDownload(AppItem *app)
{
	for (auto& item : m_downloads)
		if (item->appItem == app && item->download->getStatus() != Download::Failed && item->download->getStatus() != Download::Canceled)
		{
			item->download->cancel(false);
			break;
		}
}


void DownloadQueue::restartDownload(AppItem *app)
{
	cancelDownload(app);
	addDownload(app);
}


bool DownloadQueue::isDownloading(AppItem *app)
{
	for (auto& item : m_downloads)
	{
		if (item->appItem == app)
			return true;
	}
	return false;
}


bool DownloadQueue::processEvent(const cpp3ds::Event &event)
{
	for (auto& item : m_downloads)
		item->download->processEvent(event);
	return true;
}

void DownloadQueue::realign()
{
	bool processedFirstItem = false;
	for (int i = 0; i < m_downloads.size(); ++i)
	{
		Download *download = m_downloads[i]->download;
		Download::Status status = download->getStatus();
		if (processedFirstItem)
		{
			download->m_canSendTop = (status == Download::Queued || status == Download::Suspended || status == Download::Downloading);
		}
		else if (status == Download::Queued || status == Download::Suspended || status == Download::Downloading)
		{
			processedFirstItem = true;
			download->m_canSendTop = false;
		}

		TweenEngine::Tween::to(*download, util3ds::TweenSprite::POSITION_Y, 0.2f)
			.target(33.f + i * 35.f)
			.start(m_tweenManager);
	}
	save();
}

void DownloadQueue::update(float delta)
{
	// Remove downloads marked for delete
	bool changed = false;
	for (auto it = m_downloads.begin(); it != m_downloads.end();)
	{
		Download *download = it->get()->download;
		if (download->markedForDelete())
		{
			if (download->getStatus() == Download::Suspended)
				it->get()->installer->abort();
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
	for (auto& item : m_downloads)
		if (item->download->getProgress() < 1.f)
			++count;
	return count;
}

DownloadQueue &DownloadQueue::getInstance()
{
	static DownloadQueue downloadQueue;
	return downloadQueue;
}

void DownloadQueue::sendTop(Download *download)
{
	cpp3ds::Lock lock(m_mutexRefresh);
	auto iterTopDownload = m_downloads.end();
	auto iterBottomDownload = m_downloads.end();
	for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
	{
		if (iterTopDownload == m_downloads.end())
		{
			Download::Status status = it->get()->download->getStatus();
			if (status == Download::Downloading || status == Download::Queued || status == Download::Suspended)
				iterTopDownload = it;
		}
		else if (iterBottomDownload == m_downloads.end())
		{
			if (it->get()->download == download)
				iterBottomDownload = it;
		}
	}

	if (iterTopDownload != m_downloads.end() && iterBottomDownload != m_downloads.end())
	{
		std::rotate(iterTopDownload, iterBottomDownload, iterBottomDownload + 1);
		realign();
		m_clockRefresh.restart();
	}
}

void DownloadQueue::refresh()
{
	while (!m_refreshEnd)
	{
		{
			cpp3ds::Lock lock(m_mutexRefresh);
			DownloadItem *activeDownload = nullptr;
			DownloadItem *firstQueued = nullptr;
			bool activeNeedsSuspension = false;
			for (auto& item : m_downloads)
			{
				Download::Status status = item->download->getStatus();
				if (!firstQueued && (status == Download::Queued || status == Download::Suspended))
				{
					firstQueued = item.get();
				}
				else if (!activeDownload && status == Download::Downloading)
				{
					activeDownload = item.get();
					if (firstQueued)
						activeNeedsSuspension = true;
				}
			}

			if ((!activeDownload && firstQueued) || activeNeedsSuspension)
			{
				if (activeNeedsSuspension)
				{
					activeDownload->download->suspend();
					activeDownload->installer->suspend();
				}

				firstQueued->installer->resume();
				firstQueued->download->resume();
				realign();
			}
		}

		m_clockRefresh.restart();
		while (m_clockRefresh.getElapsedTime() < cpp3ds::seconds(1.f))
			cpp3ds::sleep(cpp3ds::milliseconds(200));
	}
}

void DownloadQueue::suspend()
{
	cpp3ds::Lock lock(m_mutexRefresh);
	m_refreshEnd = true;
	for (auto& item : m_downloads)
	{
		item->download->suspend();
		item->installer->suspend();
	}
}

void DownloadQueue::resume()
{
	m_refreshEnd = false;
	m_threadRefresh.launch();
}

void DownloadQueue::save()
{
	rapidjson::Document json;
	std::string filepath = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop/queue.json");
	std::ofstream file(filepath);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	json.SetArray();
	for (auto& item : m_downloads)
	{
		Download::Status status = item->download->getStatus();

		if (status != Download::Downloading && status != Download::Queued && status != Download::Suspended)
			continue;

		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("id", rapidjson::StringRef(item->appItem->getTitleId().c_str()), json.GetAllocator());
		obj.AddMember("content_index", rapidjson::Value().SetInt(status == Download::Queued ? -1 : item->installer->getCurrentContentIndex()), json.GetAllocator());
		obj.AddMember("progress", rapidjson::Value().SetFloat(item->download->getProgress()), json.GetAllocator());
		json.PushBack(obj, json.GetAllocator());
	}

	json.Accept(writer);
}

void DownloadQueue::load()
{
	uint32_t pendingTitleCount = 0;
	uint64_t *pendingTitleIds = nullptr;
#ifndef EMULATION
	Result res = 0;
	if (R_SUCCEEDED(res = AM_GetPendingTitleCount(&pendingTitleCount, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING)))
		if (pendingTitleIds = new uint64_t[pendingTitleCount])
			res = AM_GetPendingTitleList(&pendingTitleCount, pendingTitleCount, MEDIATYPE_SD, AM_STATUS_MASK_INSTALLING, pendingTitleIds);
#endif
	cpp3ds::FileInputStream file;
	if (file.open("sdmc:/freeShop/queue.json"))
	{
		rapidjson::Document json;
		std::string jsonStr;
		jsonStr.resize(file.getSize());
		file.read(&jsonStr[0], jsonStr.size());
		json.Parse(jsonStr.c_str());
		if (!json.HasParseError())
		{
			auto &list = AppList::getInstance().getList();
			for (auto it = json.Begin(); it != json.End(); ++it)
			{
				auto item = it->GetObject();
				std::string strTitleId = item["id"].GetString();
				int contentIndex = item["content_index"].GetInt();
				float progress = item["progress"].GetFloat();

				uint64_t titleId = strtoull(strTitleId.c_str(), 0, 16);
				for (int i = 0; i < pendingTitleCount; ++i)
					if (contentIndex == -1 || pendingTitleIds[i] == titleId)
					{
						for (auto& app : list)
							if (app->getTitleId() == strTitleId)
								addDownload(app.get(), contentIndex, progress);
						break;
					}
			}
		}
	}

	if (pendingTitleIds)
		delete[] pendingTitleIds;
}

} // namespace FreeShop
