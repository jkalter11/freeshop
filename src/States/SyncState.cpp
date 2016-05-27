#include "SyncState.hpp"
#include "../Download.hpp"
#include "../Installer.hpp"
#include "../version.h"
#include "../Util.hpp"
#include "../AssetManager.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <sys/stat.h>
#include <fstream>
#include <cpp3ds/System/Service.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <cpp3ds/System/FileInputStream.hpp>

namespace {

int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	while (1)
	{
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r < ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(aw, buff, size, offset);
		if (r < ARCHIVE_OK)
		{
			fprintf(stderr, "%s\n", archive_error_string(aw));
			return (r);
		}
	}
}

int extract(const std::string &filename, const std::string &destDir)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r = 0;

	a = archive_read_new();
	archive_read_support_format_zip(a);
	archive_read_support_compression_compress(a);
	ext = archive_write_disk_new();
	if ((r = archive_read_open_filename(a, cpp3ds::FileSystem::getFilePath(filename).c_str(), 10240))) {
		std::cout << "failure! " << r << std::endl;
		return r;
	}

	while(1)
	{
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(a));
		// TODO: handle these fatal error
		std::string path = cpp3ds::FileSystem::getFilePath(destDir + archive_entry_pathname(entry));

		archive_entry_set_pathname(entry, path.c_str());
		r = archive_write_header(ext, entry);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
		else if (archive_entry_size(entry) > 0)
		{
			r = copy_data(a, ext);

			if (r < ARCHIVE_OK)
				fprintf(stderr, "%s\n", archive_error_string(ext));
		}
		r = archive_write_finish_entry(ext);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return r;
}

}

namespace FreeShop {

SyncState::SyncState(StateStack& stack, Context& context)
: State(stack, context)
, m_threadSync(&SyncState::sync, this)
, m_threadStartupSound(&SyncState::startupSound, this)
{
	m_soundStartup.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/startup.ogg"));

	m_soundLoading.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/loading.ogg"));
	m_soundLoading.setLoop(true);

	m_textStatus.setCharacterSize(14);
	m_textStatus.setFillColor(cpp3ds::Color::White);
	m_textStatus.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textStatus.setOutlineThickness(2.f);
	m_textStatus.setPosition(160.f, 155.f);
	TweenEngine::Tween::to(m_textStatus, util3ds::TweenText::OUTLINE_COLOR_ALPHA, 0.15f)
			.target(90)
			.repeatYoyo(-1, 0)
			.start(m_tweenManager);

	m_threadSync.launch();
	m_threadStartupSound.launch();
//	sync();
}


SyncState::~SyncState()
{
	m_threadSync.wait();
	m_threadStartupSound.wait();
}


void SyncState::renderTopScreen(cpp3ds::Window& window)
{
	// Nothing
}

void SyncState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_textStatus);
}

bool SyncState::update(float delta)
{
	m_tweenManager.update(delta);
	return true;
}

bool SyncState::processEvent(const cpp3ds::Event& event)
{
	return true;
}

void SyncState::sync()
{
	m_timer.restart();

	if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc))
	{
		setStatus("No internet connection.\nClosing BrewMan...");
		while (m_timer.getElapsedTime() < cpp3ds::seconds(4.f))
			cpp3ds::sleep(cpp3ds::milliseconds(50));
		requestStackClear();
		return;
	}

	// Set up directory structure, if necessary
	std::string path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop/tmp");
	if (pathExists(path.c_str(), false))
		removeDirectory(path.c_str());
	mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath("sdmc:/freeShop/cache");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	// If auto-dated, boot into newest BrewMan
	if (autoUpdate())
	{
		// Install CIA update and boot to it.
		requestStackClear();
		return;
	}

	setStatus(_("Checking latest cache..."));
	const char *url = "https://api.github.com/repos/Repo3DS/shop-cache/releases/latest";
	const char *latestJsonFilename = "sdmc:/freeShop/tmp/latest.json";
	Download cache(url, latestJsonFilename);
	cache.run();

	cpp3ds::FileInputStream jsonFile;
	if (jsonFile.open(latestJsonFilename))
	{
		std::string json;
		rapidjson::Document doc;
		int size = jsonFile.getSize();
		json.resize(size);
		jsonFile.read(&json[0], size);
		doc.Parse(json.c_str());

		std::string tag = doc["tag_name"].GetString();
		if (!tag.empty())
		{
			std::string cacheFile = "sdmc:/freeShop/tmp/cache.zip";
			std::string cacheUrl = _("https://github.com/Repo3DS/shop-cache/releases/download/%s/cache-%s-etc1.zip", tag.c_str(), tag.c_str());
			setStatus(_("Downloading latest cache: %s...", tag.c_str()));
			Download cacheDownload(cacheUrl, cacheFile);
			cacheDownload.run();

			setStatus(_("Extracting latest cache..."));
			extract(cacheFile, "sdmc:/freeShop/cache/");
		}
	}

	std::cout << "time: " << m_timer.getElapsedTime().asSeconds() << std::endl;

	setStatus(_("Loading game list..."));

	// Give the Title animation time to finish if necessary
	while (m_timer.getElapsedTime() < cpp3ds::seconds(7.f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));

	requestStackClear();
	requestStackPush(States::Browse);
}


bool SyncState::autoUpdate()
{
	return false;
}


void SyncState::setStatus(const std::string &message)
{
	m_textStatus.setString(message);
	cpp3ds::FloatRect rect = m_textStatus.getLocalBounds();
	m_textStatus.setOrigin(rect.width / 2.f, rect.height / 2.f);
}

void SyncState::startupSound()
{
	cpp3ds::Clock clock;
	while (clock.getElapsedTime() < cpp3ds::seconds(3.5f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));
	m_soundStartup.play();
	while (clock.getElapsedTime() < cpp3ds::seconds(7.f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));
	m_soundLoading.play();
}

} // namespace FreeShop
