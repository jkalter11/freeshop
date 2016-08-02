#include <vector>
#include <map>
#include "TitleKeys.hpp"

namespace {

std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> titleKeys;

}


namespace FreeShop {

/* encTitleKeys.bin format

  4 bytes  Number of entries
 12 bytes  Reserved

 entry(32 bytes in size):
   4 bytes  Common key index(0-5)
   4 bytes  Reserved
   8 bytes  Title Id
  16 bytes  Encrypted title key
*/
void TitleKeys::ensureTitleKeys()
{
	if (!titleKeys.size())
	{
		cpp3ds::FileInputStream file;
		file.open("sdmc:/freeShop/encTitleKeys.bin");
		if (isValidFile(file))
		{
			size_t count = file.getSize() / 32;

			cpp3ds::Uint64 titleId;
			cpp3ds::Uint32 titleKey[4];

			for (int i = 0; i < count; ++i)
			{
				file.seek(24 + i * 32);
				file.read(&titleId, 8);
				file.read(titleKey, 16);

				for (int j = 0; j < 4; ++j)
					titleKeys[__builtin_bswap64(titleId)][j] = titleKey[j];
			}
		}
	}
}

std::vector<cpp3ds::Uint64> TitleKeys::getRelated(cpp3ds::Uint64 titleId, TitleType type)
{
	ensureTitleKeys();
	std::vector<cpp3ds::Uint64> related;
	cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
	for (const auto &key : titleKeys)
		if ((titleLower == (key.first & 0xFFFFFFFF) >> 8) && (key.first >> 32 == type))
			related.push_back(key.first);
	return related;
}

std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> &TitleKeys::getList()
{
	ensureTitleKeys();
	return titleKeys;
}

cpp3ds::Uint32 *TitleKeys::get(cpp3ds::Uint64 titleId)
{
	ensureTitleKeys();
	if (titleKeys.find(titleId) == titleKeys.end())
		return nullptr;
	return titleKeys[titleId];
}

bool TitleKeys::isValidFile(cpp3ds::FileInputStream &file)
{
	std::vector<char> data;
	cpp3ds::Int64 size = file.getSize() - 16; // Size minus header

	int count;
	file.seek(0);
	file.read(&count, 4);
	if (size % 32 != 0 || size / 32 != count)
		return false;
	count = (count > 3) ? 3 : count; // Check a max of 3 items

	data.resize(count * 32);
	file.seek(16);
	file.read(&data[0], data.size());

	return isValidData(&data[0], data.size());
}

bool TitleKeys::isValidFile(const std::string &filename)
{
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	return isValidFile(file);
}

bool TitleKeys::isValidData(const void *data, size_t size)
{
	auto ptr = reinterpret_cast<const cpp3ds::Uint8*>(data);
	size_t count = size / 32;
	for (int i = 0; i < count; ++i)
	{
		cpp3ds::Uint64 titleId = *reinterpret_cast<const cpp3ds::Uint64*>(ptr + 8 + i * 32);
		if (__builtin_bswap64(titleId) >> 48 != 4)
			return false;
	}
	return true;
}


} // namespace FreeShop
