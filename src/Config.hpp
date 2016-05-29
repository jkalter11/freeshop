#ifndef FREESHOP_CONFIG_HPP
#define FREESHOP_CONFIG_HPP

#include <string>
#include <rapidjson/document.h>


namespace FreeShop {

class Config {
public:
	static Config& getInstance();

	void loadDefaults();

	static bool loadFromFile(const std::string& filename = "sdmc:/freeShop/config.json");
	static void saveToFile(const std::string& filename = "sdmc:/freeShop/config.json");

	static bool keyExists(const std::string& key);

	static const rapidjson::Value &get(const std::string& key);

	template <typename T>
	static void set(const std::string& key, T val)
	{
		rapidjson::Value v(val);
		set(key, v);
	}

	static void set(const std::string& key, const char *val);
	static void set(const std::string& key, rapidjson::Value &val);

	~Config();

private:
	Config();

	rapidjson::Document m_json;
	std::string m_jsonString;
};

} // namespace FreeShop

#endif // FREESHOP_CONFIG_HPP
