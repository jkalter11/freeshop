#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Installer.hpp"


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
	//
}

bool Installer::installTicket(cpp3ds::Uint64 titleId, cpp3ds::Uint16 titleVersion)
{
	return true;
}

bool Installer::installSeed(cpp3ds::Uint64 titleId, const std::string &countryCode)
{
	return true;
}

bool Installer::titleKeyExists(cpp3ds::Uint64 titleId)
{
	return true;
}

void Installer::start()
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
	return true;
}

bool Installer::installTmd(const void *data, size_t size)
{
	return true;
}

bool Installer::installContent(const void *data, size_t size, cpp3ds::Uint16 index)
{
	return true;
}

} // namespace FreeShop
