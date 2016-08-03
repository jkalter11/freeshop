#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Config.hpp"
#include "Util.hpp"

namespace FreeShop {

Config::Config()
{
	loadDefaults();
}

Config &Config::getInstance()
{
	static Config config;
	return config;
}

bool Config::loadFromFile(const std::string &filename)
{
	rapidjson::Document &json = getInstance().m_json;
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::string jsonString;
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	jsonString.resize(file.getSize());
	file.read(&jsonString[0], jsonString.size());
	json.Parse(jsonString.c_str());
	getInstance().loadDefaults();
	return !json.HasParseError();
}

void Config::saveToFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::ofstream file(path);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	getInstance().m_json.Accept(writer);
}

bool Config::keyExists(const std::string &key)
{
	return getInstance().m_json.HasMember(key.c_str());
}

const rapidjson::Value &Config::get(const std::string &key)
{
	return getInstance().m_json[key.c_str()];
}

#define ADD_DEFAULT(key, val) \
	if (!m_json.HasMember(key)) \
		m_json.AddMember(key, val, m_json.GetAllocator());

void Config::loadDefaults()
{
	if (!m_json.IsObject())
		m_json.SetObject();

	ADD_DEFAULT("cache_version", "");

	// Update settings
	ADD_DEFAULT("auto-update", true);
	ADD_DEFAULT("download_title_keys", false);

	// Other settings
	ADD_DEFAULT("sleep_mode", true);
}

void Config::set(const std::string &key, const char *val)
{
	rapidjson::Value v(val, getInstance().m_json.GetAllocator());
	set(key, v);
}

void Config::set(const std::string &key, rapidjson::Value &val)
{
	if (keyExists(key))
		getInstance().m_json[key.c_str()] = val;
	else
		getInstance().m_json.AddMember(rapidjson::StringRef(key.c_str()), val, getInstance().m_json.GetAllocator());
}

rapidjson::Document::AllocatorType &Config::getAllocator()
{
	return getInstance().m_json.GetAllocator();
}


} // namespace FreeShop
