#ifndef FREESHOP_UTIL_HPP
#define FREESHOP_UTIL_HPP

namespace FreeShop
{

bool pathExists(const char* path, bool escape = true);
int removeDirectory(const char *path, bool onlyIfEmpty = false);

} // namespace FreeShop

#endif // FREESHOP_UTIL_HPP
