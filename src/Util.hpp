#ifndef FREESHOP_UTIL_HPP
#define FREESHOP_UTIL_HPP

#include <sys/stat.h>
#include <string>
#include <cpp3ds/Network/Http.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop
{

bool pathExists(const char* path, bool escape = true);
void makeDirectory(const char *dir, mode_t mode = 0777);
int removeDirectory(const char *path, bool onlyIfEmpty = false);
std::string getCountryCode(int region);
uint32_t getTicketVersion(cpp3ds::Uint64 tid);
#ifndef EMULATION
Result getTitleVersion(uint64_t tid, uint16_t *version);
static const Result tmd_FileNotFoundResult = MAKERESULT(RL_PERMANENT, RS_NOTFOUND, RM_APPLICATION, 5);
#endif

} // namespace FreeShop

#endif // FREESHOP_UTIL_HPP
