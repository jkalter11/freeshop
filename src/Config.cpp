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

Config::~Config()
{
	saveToFile();
}

Config &Config::getInstance()
{
	static Config config;
	return config;
}

bool Config::loadFromFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	getInstance().m_jsonString.resize(file.getSize());
	file.read(&getInstance().m_jsonString[0], getInstance().m_jsonString.size());
	getInstance().m_json.Parse(getInstance().m_jsonString.c_str());
	return !getInstance().m_json.HasParseError();
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

void Config::loadDefaults()
{
	m_json.SetObject();
	m_json.AddMember("version", rapidjson::StringRef(FREESHOP_VERSION), m_json.GetAllocator());
	m_json.AddMember("cache_version", "", m_json.GetAllocator());
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

} // namespace FreeShop
