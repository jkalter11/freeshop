#ifndef FREESHOP_INSTALLER_HPP
#define FREESHOP_INSTALLER_HPP

#include <string>
#include <cpp3ds/Config.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

class Installer {
public:
	Installer(cpp3ds::Uint64 titleId);
	~Installer();

	static bool installTicket(cpp3ds::Uint64 titleId, cpp3ds::Uint16 titleVersion);
	static bool titleKeyExists(cpp3ds::Uint64 titleId);

	void start();
//	void resume();
//	void suspend();
	void abort();

	bool installTmd(const void *data, size_t size);
	bool installContent(const void *data, size_t size, cpp3ds::Uint16 index);

	bool finalizeTmd();
	bool finalizeContent();
	bool commit();

private:

private:
	cpp3ds::Uint64 m_titleId;

#ifndef EMULATION
	Handle m_handleTmd;
	Handle m_handleContent;
#endif

	bool m_isInstalling;
	bool m_isInstallingTmd;
	bool m_isInstallingContent;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLER_HPP
