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
	// Cancel all downloads
	for (auto& download : m_downloads)
		delete download.second;
	for (auto& installer : m_installers)
		delete installer.second;
}


void DownloadQueue::addDownload(AppItem* app, cpp3ds::Uint64 contentPosition, int contentIndex, float progress)
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
	cpp3ds::Uint64 titleId = strtoull(app->getTitleId().c_str(), 0, 16);
	Installer *installer = new Installer(titleId, contentPosition, contentIndex);
	cpp3ds::Uint64 titleFileSize = app->getFilesize();
	cpp3ds::Uint64 totalProcessed = progress * titleFileSize;
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

				bool foundIndex = false; // For resuming via contentIndex arg
				for (int i = 0; i < contentCount; ++i)
				{
					char *contentChunk = &buf[dataOffsets[sigType] + 0x9C4 + (i * 0x30)];
					cpp3ds::Uint32 contentId = __builtin_bswap32(*(cpp3ds::Uint32*)&contentChunk[0]);
					cpp3ds::Uint16 contentIdx = __builtin_bswap16(*(cpp3ds::Uint16*)&contentChunk[4]);
					cpp3ds::Uint64 contentSize = __builtin_bswap64(*(cpp3ds::Uint64*)&contentChunk[8]);

					if (contentIdx == contentIndex)
						foundIndex = true;
					if (contentIndex == -1 || foundIndex)
					{
						contentIndices.push_back(contentIdx);
						download->pushUrl(_("http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/%08lX", titleId, contentId), (contentIdx == contentIndex) ? contentPosition : 0);
					}
				}

				if (contentIndex == -1)
				{
					download->setProgressMessage(_("Installing ticket..."));
					if (!installer->installTicket(titleVersion))
						return false;
					download->setProgressMessage(_("Installing seed..."));
					if (!installer->installSeed(app->getUriRegion()))
						return false;

					installer->start();
					if (!installer->installTmd(&buf[0], dataOffsets[sigType] + 0x9C4 + (contentCount * 0x30)))
						return false;
					if (!installer->finalizeTmd())
						return false;
				}

				buf.clear();
				fileSize = 0;
				fileIndex++;
			}
		}
		else // is a Content file
		{
			if (!installer->installContent(data, len, contentIndices[fileIndex-1]))
			{
				if (download->getStatus() == Download::Suspended)
				{
					fileSize = 0;
					return true;
				}
				return false;
			}

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
		if (clock.getElapsedTime() > cpp3ds::seconds(1.f))
		{
			download->setProgressMessage(_("Installing... %.1f%% (%.0f KB/s)",
										   download->getProgress() * 100.f,
										   count / clock.getElapsedTime().asSeconds() / 1024.f));
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
				download->setProgressMessage(installer->getErrorString());
				break;
			case Download::Canceled:
				download->setProgressMessage(_("Canceled"));
				break;
		}

		download->setProgress(1.f);

		// Reset CanSendTop state
		for (auto& download : m_downloads)
			download.second->m_canSendTop = true;

		for (auto it = m_installers.begin(); it < m_installers.end(); ++it)
			if (it->second == installer) {
				m_installers.erase(it);
				delete installer;
				break;
			}
	});

	if (progress > 0.f)
	{
		download->setProgress(progress);
		download->setProgressMessage(_("Suspended"));
	}
	else
		download->setProgressMessage(_("Queued"));

	if (contentIndex == -1)
	{
		cpp3ds::String s = app->getTitle();
		s.insert(0, _("Queued install: "));
		Notification::spawn(s);
	}

	m_downloads.emplace_back(std::make_pair(app, download));
	m_installers.emplace_back(std::make_pair(app, installer));
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
	for (auto& pair : m_downloads)
		if (pair.first == app)
		{
			pair.second->cancel(false);
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
	bool processedFirstItem = false;
	for (int i = 0; i < m_downloads.size(); ++i)
	{
		Download *download = m_downloads[i].second;
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
			// Delete matching installer
			for (auto pair = m_installers.begin(); pair != m_installers.end(); ++pair)
				if (pair->first == it->first)
				{
					delete pair->second;
					m_installers.erase(pair);
					break;
				}
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

void DownloadQueue::sendTop(Download *download)
{
	cpp3ds::Lock lock(m_mutexRefresh);
	auto iterTopDownload = m_downloads.end();
	auto iterBottomDownload = m_downloads.end();
	for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
	{
		if (iterTopDownload == m_downloads.end())
		{
			Download::Status status = it->second->getStatus();
			if (status == Download::Downloading || status == Download::Queued || status == Download::Suspended)
				iterTopDownload = it;
		}
		else if (iterBottomDownload == m_downloads.end())
		{
			if (it->second == download)
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
			AppItem *activeDownload = nullptr;
			AppItem *firstQueued = nullptr;
			bool activeNeedsSuspension = false;
			for (auto& download : m_downloads)
			{
				Download::Status status = download.second->getStatus();
				if (!firstQueued && (status == Download::Queued || status == Download::Suspended))
				{
					firstQueued = download.first;
				}
				else if (!activeDownload && status == Download::Downloading)
				{
					activeDownload = download.first;
					if (firstQueued)
						activeNeedsSuspension = true;
				}
			}

			if ((!activeDownload && firstQueued) || activeNeedsSuspension)
			{
				if (activeNeedsSuspension)
				{
					for (auto& download : m_downloads)
						if (download.first == activeDownload)
							download.second->suspend();
					for (auto& installer : m_installers)
						if (installer.first == activeDownload)
							installer.second->suspend();
				}

				for (auto& installer : m_installers)
					if (installer.first == firstQueued)
						installer.second->resume();
				for (auto& download : m_downloads)
					if (download.first == firstQueued)
						download.second->resume();
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
	for (auto& pair : m_downloads)
		pair.second->suspend();
	for (auto& pair : m_installers)
		pair.second->suspend();
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
	for (auto& pair : m_downloads)
	{
		AppItem *item = pair.first;
		Download *download = pair.second;
		Installer *installer;
		Download::Status status = download->getStatus();

		if (status != Download::Downloading && status != Download::Queued && status != Download::Suspended)
			continue;
		
		for (auto& pair : m_installers)
			if (pair.first == item)
			{
				installer = pair.second;
				break;
			}

		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("id", rapidjson::StringRef(item->getTitleId().c_str()), json.GetAllocator());
		obj.AddMember("content_position", rapidjson::Value().SetUint64(installer->getCurrentContentPosition()), json.GetAllocator());
		obj.AddMember("content_index", rapidjson::Value().SetUint(installer->getCurrentContentIndex()), json.GetAllocator());
		obj.AddMember("progress", rapidjson::Value().SetFloat(download->getProgress()), json.GetAllocator());
		json.PushBack(obj, json.GetAllocator());
	}

	json.Accept(writer);
}

void DownloadQueue::load()
{
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
				std::string titleId = item["id"].GetString();
				cpp3ds::Uint16 contentIndex = item["content_index"].GetUint();
				cpp3ds::Uint64 contentPosition = item["content_position"].GetUint64();
				float progress = item["progress"].GetFloat();
				for (auto& app : list)
					if (app->getTitleId() == titleId)
					{
						std::cout << app->getTitle().toAnsiString() << std::endl;
						addDownload(app.get(), contentPosition, contentIndex, progress);
					}
			}
		}
	}
}

} // namespace FreeShop
