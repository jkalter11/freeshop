#ifndef FREESHOP_TITLEKEYS_HPP
#define FREESHOP_TITLEKEYS_HPP

#include <cpp3ds/System/FileInputStream.hpp>

namespace FreeShop {

class TitleKeys {
public:
	enum TitleType {
		Game = 0x40000,
		Update = 0x4000E,
		Demo = 0x40002,
		DLC = 0x4008C,
		DSiWare = 0x48004,

		SystemApplet = 0x40030,
	};

	static cpp3ds::Uint32 *get(cpp3ds::Uint64);
	static std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> &getList();
	static std::vector<cpp3ds::Uint64> getRelated(cpp3ds::Uint64 titleId, TitleType type);

	static bool isValidFile(cpp3ds::FileInputStream &file);
	static bool isValidFile(const std::string &filename);
	static bool isValidData(const void *data, size_t size);

private:
	// Load title keys from file if not done already
	static void ensureTitleKeys();
};

} // namespace FreeShop


#endif // FREESHOP_TITLEKEYS_HPP
