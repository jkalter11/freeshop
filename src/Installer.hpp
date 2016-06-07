#ifndef FREESHOP_INSTALLER_HPP
#define FREESHOP_INSTALLER_HPP

#include <string>
#include <cpp3ds/Config.hpp>
#include <cpp3ds/System/Mutex.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

class Installer {
public:
	Installer(cpp3ds::Uint64 titleId, cpp3ds::Uint64 contentPosition = 0, int contentIndex = -1);
	~Installer();

	bool installTicket(cpp3ds::Uint16 titleVersion);
	bool installSeed(const std::string &countryCode);
	static bool titleKeyExists(cpp3ds::Uint64 titleId);

	void start();
	void resume();
	void suspend();
	void abort();

	bool installTmd(const void *data, size_t size);
	bool installContent(const void *data, size_t size, cpp3ds::Uint16 index);

	bool finalizeTmd();
	bool finalizeContent();
	bool commit();

	cpp3ds::Int32 getErrorCode() const;
	const cpp3ds::String &getErrorString() const;

	cpp3ds::Uint16 getCurrentContentIndex() const;
	cpp3ds::Uint64 getCurrentContentPosition() const;

private:

private:
	cpp3ds::Uint64 m_titleId;
	cpp3ds::Uint16 m_currentContentIndex;
	cpp3ds::Uint64 m_currentContentPosition;
	cpp3ds::Mutex m_mutex;
	cpp3ds::String m_errorStr;

#ifndef EMULATION
	Result m_result;
	Handle m_handleTmd;
	Handle m_handleContent;
#endif

	bool m_isSuspended;
	bool m_isInstalling;
	bool m_isInstallingTmd;
	bool m_isInstallingContent;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLER_HPP
