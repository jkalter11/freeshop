#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Installer.hpp"

namespace {

std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> titleKeys;

// Load title keys from file if not done already
void ensureTitleKeys()
{
	if (!titleKeys.size())
	{
		cpp3ds::FileInputStream file;
		if (file.open("sdmc:/freeShop/encTitleKeys.bin"))
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

}

namespace FreeShop {

Installer::Installer(cpp3ds::Uint64 titleId, int contentIndex)
: m_titleId(titleId)
, m_isSuspended(false)
, m_isInstalling(false)
, m_isInstallingTmd(false)
, m_isInstallingContent(false)
, m_currentContentIndex(0)
, m_currentContentPosition(0)
{

}

Installer::~Installer()
{
	//
}

bool Installer::installTicket(cpp3ds::Uint16 titleVersion)
{
	return true;
}

bool Installer::installSeed(const void *seed)
{
	return true;
}

bool Installer::titleKeyExists(cpp3ds::Uint64 titleId)
{
	ensureTitleKeys();
	return titleKeys.find(titleId) != titleKeys.end();
}

std::vector<cpp3ds::Uint64> Installer::getRelated(cpp3ds::Uint64 titleId, TitleType type)
{
	ensureTitleKeys();
	std::vector<cpp3ds::Uint64> related;
	cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
	for (const auto &key : titleKeys)
		if ((titleLower == (key.first & 0xFFFFFFFF) >> 8) && (key.first >> 32 == type))
			related.push_back(key.first);
	return related;
}

bool Installer::start(bool deleteTitle)
{
	return true;
}


bool Installer::resume()
{
	return true;
}

void Installer::suspend()
{
	//
}

void Installer::abort()
{
	//
}

bool Installer::commit()
{
	return true;
}

bool Installer::finalizeTmd()
{
	return true;
}

bool Installer::finalizeContent()
{
	m_currentContentPosition = 0;
	return true;
}

bool Installer::importContents(size_t count, cpp3ds::Uint16 *indices)
{
	if (m_isInstalling && !commit())
		return false;

	return start(false);
}

bool Installer::installTmd(const void *data, size_t size)
{
	return true;
}

bool Installer::installContent(const void *data, size_t size, cpp3ds::Uint16 index)
{
	m_currentContentPosition += size;
	m_currentContentIndex = index;
	return true;
}

cpp3ds::Int32 Installer::getErrorCode() const
{
	return 0;
}

const cpp3ds::String &Installer::getErrorString() const
{
	return m_errorStr;
}

int Installer::getCurrentContentIndex() const
{
	return m_currentContentIndex;
}

cpp3ds::Uint64 Installer::getCurrentContentPosition() const
{
	return m_currentContentPosition;
}

} // namespace FreeShop
