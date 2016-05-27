#include <iostream>
#include <stdio.h>
#include <cpp3ds/System/FileSystem.hpp>
#include "Download.hpp"
#include "AssetManager.hpp"

namespace FreeShop {


Download::Download(const std::string &url, const std::string &destination)
: m_thread(&Download::run, this)
, m_progress(0.f)
, m_markedForDelete(false)
, m_cancelFlag(false)
, m_isActive(false)
{
	setUrl(url);
	setDestination(destination);

//	m_thread.setStackSize(64*1024);
//	m_thread.setAffinity(1);

	setProgressMessage("Queued");

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
	m_thread.launch();
}


void Download::run()
{
	cpp3ds::Http::Response response;

	if (!m_destination.empty())
	{
		m_file = fopen(cpp3ds::FileSystem::getFilePath(m_destination).c_str(), "w");
		if (!m_file)
			return;
	}

	m_isActive = true;
	m_cancelFlag = false;
	bool failed = false;

	cpp3ds::Http::RequestCallback dataCallback = [&](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response)
	{
		if (!m_destination.empty())
		{
			if (response.getStatus() == 200)
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

		if (!m_cancelFlag && m_onData)
			failed = !m_onData(data, len, processed, response);

		return !m_cancelFlag && !failed;
	};

	// Loop in case there are URLs push to queue
	while (1)
	{
		// Follow all redirects
		response = m_http.sendRequest(m_request, cpp3ds::Time::Zero, dataCallback);
		while (response.getStatus() == 301 || response.getStatus() == 302)
		{
			setUrl(response.getField("Location"));
			response = m_http.sendRequest(m_request, cpp3ds::Time::Zero, dataCallback);
		}

		if (response.getStatus() != 200)
			break;
		if (m_cancelFlag || failed)
			break;

		if (m_urlQueue.size() > 0)
		{
			setUrl(m_urlQueue.front());
			m_urlQueue.pop();
		}
		else
			break;
	}

	if (m_urlQueue.size() > 0)
		std::queue<std::string>().swap(m_urlQueue);

	// Write remaining buffer and close downloaded file
	if (!m_destination.empty())
	{
		if (m_buffer.size() > 0)
			fwrite(&m_buffer[0], sizeof(char), m_buffer.size(), m_file);
		fclose(m_file);
	}

	if (m_onFinish)
		m_onFinish(m_cancelFlag, failed);

	if (m_cancelFlag)
		m_markedForDelete = true;

	m_isActive = false;
}


void Download::cancel(bool wait)
{
	m_cancelFlag = true;
	if (wait)
		m_thread.wait();
	m_isActive = false;
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
	cpp3ds::IntRect textureRect;
	m_icon.setTexture(app->getIcon(textureRect), true);
	m_icon.setTextureRect(textureRect);
	m_textTitle.setString(app->getTitle());
}


void Download::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::TouchBegan)
	{
		cpp3ds::FloatRect cancelBounds = m_textCancel.getGlobalBounds();
		cancelBounds.left += getPosition().x;
		cancelBounds.top += getPosition().y;
		if (cancelBounds.contains(event.touch.x, event.touch.y))
		{
			// If download is already finished, just mark it for deletion
			if (!isActive())
				m_markedForDelete = true;
			else
			{
				setProgressMessage("Canceling...");
				cancel(false);
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

void Download::setField(const std::string &field, const std::string &value)
{
	m_request.setField(field, value);
}

void Download::pushUrl(const std::string &url)
{
	m_urlQueue.push(url);
}

bool Download::isActive() const
{
	return m_isActive;
}

} // namespace FreeShop
