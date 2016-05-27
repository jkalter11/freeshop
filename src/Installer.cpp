#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Installer.hpp"
#include "ticket.h"

namespace {

std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> titleKeys;

// Load title keys from file if not done already
void ensureTitleKeys()
{
	if (!titleKeys.size())
	{
		cpp3ds::FileInputStream file;
		file.open("sdmc:/freeShop/encTitleKeys.bin");
		size_t count = file.getSize() / 32;

		cpp3ds::Uint64 titleId;
		cpp3ds::Uint32 titleKey[4];

		for (int i = 0; i < count; ++i)
		{
			file.seek(24 + i * 32);
			file.read(&titleId, 8);
			file.read(titleKey, 16);

			for (int j = 0; j < 4; ++j)
				titleKeys[titleId][j] = titleKey[j];
		}
	}
}

}


namespace FreeShop {

Installer::Installer(cpp3ds::Uint64 titleId)
: m_titleId(titleId)
, m_isInstalling(false)
, m_isInstallingTmd(false)
, m_isInstallingContent(false)
{

}

Installer::~Installer()
{
	abort();
}

void Installer::abort()
{
	if (m_isInstallingContent)
		AM_InstallContentCancel(m_handleContent);
	if (m_isInstallingTmd)
		AM_InstallTmdAbort(m_handleTmd);
	if (m_isInstalling)
		AM_InstallTitleAbort();

	m_isInstallingContent = false;
	m_isInstallingTmd = false;
	m_isInstalling = false;
}

bool Installer::installTicket(cpp3ds::Uint64 titleId, cpp3ds::Uint16 titleVersion)
{
	Result ret;
	Handle ticket;
	u8 tikData[sizeof(tikTemp)];
	const u16 sigSize = 0x140;
	titleId = __builtin_bswap64(titleId);

	ensureTitleKeys();

	// Build ticket
	memcpy(tikData, tikTemp, sizeof(tikData));
	memcpy(tikData + sigSize + 0x9C, &titleId, 8);
	memcpy(tikData + sigSize + 0xA6, &titleVersion, 2);
	memcpy(tikData + sigSize + 0x7F, titleKeys[titleId], 16);

	AM_QueryAvailableExternalTitleDatabase(nullptr);

	if (R_SUCCEEDED(ret = AM_InstallTicketBegin(&ticket)))
	{
		FSFILE_Write(ticket, nullptr, 0, tikData, sizeof(tikData)/2, 0);
		if (R_SUCCEEDED(ret = FSFILE_Write(ticket, nullptr, 0, tikData+sizeof(tikData)/2, sizeof(tikData)/2, 0)))
		{
			if (R_SUCCEEDED(ret = AM_InstallTicketFinish(ticket)))
				return true;
		}
		AM_InstallTicketAbort(ticket);
	}

	cpp3ds::err() << _("Failed to install ticket: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

void Installer::start()
{
	Result ret;

	if (!m_isInstalling)
	{
		AM_DeleteTitle(MEDIATYPE_SD, m_titleId);
		AM_QueryAvailableExternalTitleDatabase(nullptr);
		if (R_SUCCEEDED(ret = AM_InstallTitleBegin(MEDIATYPE_SD, m_titleId, false)))
			std::cout << _("Begin installing %016llX", m_titleId).toAnsiString() << std::endl;
		m_isInstalling = true;
	}
}

bool Installer::commit()
{
	Result ret;

	if (!m_isInstalling || m_isInstallingTmd || m_isInstallingContent)
		return false;

	m_isInstalling = false;
	if (R_SUCCEEDED(ret = AM_InstallTitleFinish()))
		if (R_SUCCEEDED(ret = AM_CommitImportTitles(MEDIATYPE_SD, 1, false, &m_titleId)))
			return true;

	AM_InstallTitleAbort();
	cpp3ds::err() << _("Failed to commit title install: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

bool Installer::finalizeTmd()
{
	Result ret;

	if (!m_isInstallingTmd)
		return false;

	m_isInstallingTmd = false;
	if (R_SUCCEEDED(ret = AM_InstallTmdFinish(m_handleTmd, true)))
		return true;

	AM_InstallTmdAbort(m_handleTmd);
	cpp3ds::err() << _("Failed to finalize TMD install: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

bool Installer::finalizeContent()
{
	Result ret;

	if (!m_isInstallingContent)
		return false;

	m_isInstallingContent = false;
	if (R_SUCCEEDED(ret = AM_InstallContentFinish(m_handleContent)))
		return true;

	AM_InstallContentCancel(m_handleContent);
	cpp3ds::err() << _("Failed to finalize Content install: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

bool Installer::installTmd(const void *data, size_t size)
{
	Result ret;
	u32 bytesWritten;

	if (!m_isInstallingTmd && R_SUCCEEDED(ret = AM_InstallTmdBegin(&m_handleTmd)))
		m_isInstallingTmd = true;

	if (m_isInstallingTmd)
		if (R_SUCCEEDED(ret = FSFILE_Write(m_handleTmd, &bytesWritten, 0, data, size, 0)))
			return true;

	cpp3ds::err() << _("Failed to install TMD: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

bool Installer::installContent(const void *data, size_t size, cpp3ds::Uint16 index)
{
	Result ret;

	if (!m_isInstallingContent && R_SUCCEEDED(ret = AM_InstallContentBegin(&m_handleContent, index)))
		m_isInstallingContent = true;

	if (m_isInstallingContent)
		if (R_SUCCEEDED(ret = FSFILE_Write(m_handleContent, nullptr, 0, data, size, 0)))
			return true;

	cpp3ds::err() << _("Failed to install Content: 0x%08lX", ret).toAnsiString() << std::endl;
	return false;
}

} // namespace FreeShop
