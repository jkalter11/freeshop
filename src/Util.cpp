#include "Util.hpp"
#include <dirent.h>
#include <sys/unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cpp3ds/System/FileSystem.hpp>

namespace FreeShop
{

bool pathExists(const char* path, bool escape)
{
	struct stat buffer;
	if (escape)
		return stat(cpp3ds::FileSystem::getFilePath(path).c_str(), &buffer) == 0;
	else
		return stat(path, &buffer) == 0;
}

void makeDirectory(const char *dir, mode_t mode)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp),"%s",dir);
	len = strlen(tmp);
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++)
		if(*p == '/') {
			*p = 0;
			mkdir(tmp, mode);
			*p = '/';
		}
	mkdir(tmp, mode);
}

int removeDirectory(const char *path, bool onlyIfEmpty)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d) {
		struct dirent *p;
		r = 0;

		while (!r && (p = readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
				continue;

			len = path_len + strlen(p->d_name) + 2;
			buf = (char*) malloc(len);

			if (buf) {
				struct stat sb;
				snprintf(buf, len, "%s/%s", path, p->d_name);

				if (!stat(buf, &sb)) {
					if (S_ISDIR(sb.st_mode))
						r2 = removeDirectory(buf, onlyIfEmpty);
					else if (!onlyIfEmpty)
						r2 = unlink(buf);
				}
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	if (!r)
		r = rmdir(path);

	return r;
}

} // namespace FreeShop
