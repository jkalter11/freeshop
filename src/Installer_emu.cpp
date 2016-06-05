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

bool Installer::installTicket(cpp3ds::Uint16 titleVersion)
{
	return true;
}

bool Installer::installSeed(const std::string &countryCode)
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


void Installer::resume()
{
	//
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

cpp3ds::Int32 Installer::getErrorCode() const
{
	return 0;
}

const cpp3ds::String &Installer::getErrorString() const
{
	return m_errorStr;
}

} // namespace FreeShop
