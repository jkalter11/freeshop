#include <iostream>
#include <stdio.h>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Lock.hpp>
#include "Download.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"

namespace FreeShop {


Download::Download(const std::string &url, const std::string &destination)
: m_thread(&Download::run, this)
, m_progress(0.f)
, m_canSendTop(true)
, m_markedForDelete(false)
, m_cancelFlag(false)
, m_status(Queued)
, m_downloadPos(0)
, m_appItem(nullptr)
{
	setUrl(url);
	setDestination(destination);

//	m_thread.setStackSize(64*1024);
//	m_thread.setAffinity(1);

	setProgressMessage(_("Queued"));

	m_icon.setSize(cpp3ds::Vector2f(48.f, 48.f));
	m_icon.setTexture(&AssetManager<cpp3ds::Texture>::get("images/missing-icon.png"), true);
	m_icon.setPosition(4.f, 4.f);
	m_icon.setScale(0.5f, 0.5f);
	m_icon.setOutlineThickness(1.f);
	m_icon.setFillColor(cpp3ds::Color(180,180,180));
	m_icon.setOutlineColor(cpp3ds::Color(0, 0, 0, 50));

	m_textCancel.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textCancel.setString(L"\uf00d");
	m_textCancel.setCharacterSize(18);
	m_textCancel.setFillColor(cpp3ds::Color::White);
	m_textCancel.setOutlineColor(cpp3ds::Color(0, 0, 0, 200));
	m_textCancel.setOutlineThickness(1.f);
	m_textCancel.setPosition(294.f, 4.f);

	m_textSendTop = m_textCancel;
	m_textSendTop.setString(L"\uf077");
	m_textSendTop.setPosition(272.f, 4.f);

	m_textRestart = m_textCancel;
	m_textRestart.setString(L"\uf01e");
	m_textRestart.setPosition(272.f, 4.f);

	m_textTitle.setCharacterSize(10);
	m_textTitle.setPosition(35.f, 2.f);
	m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.useSystemFont();

	m_textProgress = m_textTitle;
	m_textProgress.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textProgress.setPosition(35.f, 16.f);

	m_background.setTexture(&AssetManager<cpp3ds::Texture>::get("images/itembg.9.png"));
	m_background.setColor(cpp3ds::Color(255, 255, 255, 80));
	m_progressBar.setFillColor(cpp3ds::Color(0, 0, 0, 50));

	setSize(314.f, 32.f);
}


Download::~Download()
{
	cancel();
}


void Download::setUrl(const std::string &url)
{
	size_t pos;

	pos = url.find("://");
	if (pos != std::string::npos) {
		pos = url.find("/", pos + 3);
		if (pos != std::string::npos) {
			m_host = url.substr(0, pos);
			m_uri = url.substr(pos);
		}
	}

	m_http.setHost(m_host);
	m_request.setUri(m_uri);
}


void Download::start()
{
	m_buffer.clear();
	m_thread.launch(); // run()
}


void Download::run()
{
	cpp3ds::Http::Response response;
	size_t bufferSize = 128*1024;

	if (!m_destination.empty())
	{
		m_file = fopen(cpp3ds::FileSystem::getFilePath(m_destination).c_str(), "w");
		if (!m_file)
			return;
	}

	m_status = Downloading;
	m_cancelFlag = false;
	bool failed = false;
	m_request.setField("Range", _("bytes=%u-", m_downloadPos));

	cpp3ds::Http::RequestCallback dataCallback = [&](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response)
	{
		cpp3ds::Lock lock(m_mutex);

		if (!m_destination.empty())
		{
			if (response.getStatus() == cpp3ds::Http::Response::Ok || response.getStatus() == cpp3ds::Http::Response::PartialContent)
			{
				const char *bufdata = reinterpret_cast<const char*>(data);
				m_buffer.insert(m_buffer.end(), bufdata, bufdata + len);

				if (m_buffer.size() > 1024 * 50)
				{
					fwrite(&m_buffer[0], sizeof(char), m_buffer.size(), m_file);
					m_buffer.clear();
				}
			}
		}

		if (getStatus() != Suspended)
			m_downloadPos += len;

		if (!m_cancelFlag && m_onData)
			failed = !m_onData(data, len, processed, response);

		return !m_cancelFlag && !failed && getStatus() == Downloading;
	};

	// Loop in case there are URLs pushed to queue later
	while (1)
	{
		// Follow all redirects
		response = m_http.sendRequest(m_request, cpp3ds::Time::Zero, dataCallback, bufferSize);
		while (response.getStatus() == cpp3ds::Http::Response::MovedPermanently || response.getStatus() == cpp3ds::Http::Response::MovedTemporarily)
		{
			setUrl(response.getField("Location"));
			response = m_http.sendRequest(m_request, cpp3ds::Time::Zero, dataCallback, bufferSize);
		}

		if (response.getStatus() != cpp3ds::Http::Response::Ok && response.getStatus() != cpp3ds::Http::Response::PartialContent)
			break;
		if (m_cancelFlag || failed || getStatus() == Suspended)
			break;

		if (m_urlQueue.size() > 0)
		{
			auto nextUrl = m_urlQueue.front();
			setUrl(nextUrl.first);
			m_urlQueue.pop();
			m_request.setField("Range", _("bytes=%llu-", nextUrl.second));
			m_downloadPos = 0;
		}
		else
			break;
	}

	if (m_urlQueue.size() > 0 && getStatus() != Suspended)
		std::queue<std::pair<std::string,cpp3ds::Uint64>>().swap(m_urlQueue);

	// Write remaining buffer and close downloaded file
	if (!m_destination.empty())
	{
		if (m_buffer.size() > 0)
			fwrite(&m_buffer[0], sizeof(char), m_buffer.size(), m_file);
		fclose(m_file);
	}

	if (getStatus() != Suspended)
	{
		m_canSendTop = false;
		if (m_cancelFlag)
		{
			m_markedForDelete = true;
			m_status = Canceled;
		}
		else if (failed)
			m_status = Failed;
		else
			m_status = Finished;
	}

	if (m_onFinish)
		m_onFinish(m_cancelFlag, failed);

	m_http.close();
}


void Download::cancel(bool wait)
{
	m_cancelFlag = true;
	if (wait)
		m_thread.wait();
}


bool Download::isCanceled()
{
	return m_cancelFlag;
}


void Download::setProgress(float progress)
{
	m_progress = progress;
	m_progressBar.setSize(cpp3ds::Vector2f(m_progress * m_size.x, m_size.y));
}


float Download::getProgress() const
{
	return m_progress;
}


void Download::setProgressMessage(const std::string &message)
{
	m_progressMessage = message;
	m_textProgress.setString(message);
}


const std::string &Download::getProgressMessage() const
{
	return m_progressMessage;
}


void Download::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	target.draw(m_background, states);
	target.draw(m_icon, states);
	target.draw(m_textTitle, states);
	target.draw(m_textProgress, states);
	target.draw(m_textCancel, states);

	if (m_canSendTop && (m_status == Queued || m_status == Suspended || m_status == Downloading))
		target.draw(m_textSendTop, states);
	else if (m_status == Failed)
		target.draw(m_textRestart, states);
	if (m_progress > 0.f && m_progress < 1.f)
		target.draw(m_progressBar, states);
}


void Download::setSize(float width, float height)
{
	m_size.x = width;
	m_size.y = height;

	m_background.setContentSize(m_size.x + m_background.getPadding().width - m_background.getTexture()->getSize().x + 2.f,
								m_size.y + m_background.getPadding().height - m_background.getTexture()->getSize().y + 2.f);
	setProgress(m_progress); // Resize progress bar
}


void Download::fillFromAppItem(AppItem *app)
{
	m_appItem = app;
	cpp3ds::IntRect textureRect;
	m_icon.setTexture(app->getIcon(textureRect), true);
	m_icon.setTextureRect(textureRect);
	m_textTitle.setString(app->getTitle());
}


void Download::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::TouchBegan)
	{
		cpp3ds::FloatRect bounds = m_textCancel.getGlobalBounds();
		bounds.left += getPosition().x;
		bounds.top += getPosition().y;
		if (bounds.contains(event.touch.x, event.touch.y))
		{
			// If download is already finished, just mark it for deletion
			if (getStatus() != Downloading)
				m_markedForDelete = true;
			else
			{
				setProgressMessage(_("Canceling..."));
				cancel(false);
			}
		}
		else if (m_canSendTop && m_onSendTop && (getStatus() == Queued || getStatus() == Suspended || getStatus() == Downloading))
		{
			bounds = m_textSendTop.getGlobalBounds();
			bounds.left += getPosition().x;
			bounds.top += getPosition().y;
			if (bounds.contains(event.touch.x, event.touch.y))
			{
				m_onSendTop(this);
			}
		}
		else if (getStatus() == Failed)
		{
			bounds = m_textRestart.getGlobalBounds();
			bounds.left += getPosition().x;
			bounds.top += getPosition().y;
			if (bounds.contains(event.touch.x, event.touch.y))
			{
				// Restart failed download
				DownloadQueue::getInstance().restartDownload(m_appItem);
			}
		}
	}
}

bool Download::markedForDelete()
{
	return m_markedForDelete;
}

void Download::setDestination(const std::string &destination)
{
	m_destination = destination;
}

void Download::setDataCallback(cpp3ds::Http::RequestCallback onData)
{
	m_onData = onData;
}

void Download::setFinishCallback(DownloadFinishCallback onFinish)
{
	m_onFinish = onFinish;
}

void Download::setSendTopCallback(SendTopCallback onSendTop)
{
	m_onSendTop = onSendTop;
}

void Download::setField(const std::string &field, const std::string &value)
{
	m_request.setField(field, value);
}

void Download::pushUrl(const std::string &url, cpp3ds::Uint64 position)
{
	m_urlQueue.push(std::make_pair(url, position));
}

Download::Status Download::getStatus() const
{
	return m_status;
}

void Download::suspend()
{
	if (getStatus() != Downloading)
		return;
	cpp3ds::Lock lock(m_mutex);
	m_status = Suspended;
}

void Download::resume()
{
	if (getStatus() == Suspended || getStatus() == Queued)
	{
		start();
	}
}

} // namespace FreeShop
