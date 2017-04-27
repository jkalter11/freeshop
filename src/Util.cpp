#include "Util.hpp"
#include "Config.hpp"
#include <dirent.h>
#include <sys/unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <inttypes.h>
#include "Download.hpp"
#include <fstream>

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

std::string getCountryCode(int region)
{

	std::string language = Config::get(Config::Language).GetString();
	if (language == "auto")
		language = cpp3ds::I18n::getInstance().getLangString(cpp3ds::I18n::getLanguage());

#ifdef _3DS
	u8 consoleRegion;
	CFGU_SecureInfoGetRegion(&consoleRegion);
	if (language == "pt")
		language = (consoleRegion == CFG_REGION_USA) ? "pt_BR" : "pt_PT";
	else if (language == "es")
		language = (consoleRegion == CFG_REGION_USA) ? "es_US" : "es_ES";
#endif

	if (language == "zh")
		return "HK";
	else if (region == 0)
	{
		if (language == "en") return "US";
		else if (language == "pt_PT") return "PT";
		else if (language == "pt_BR") return "BR";
		else if (language == "es_US") return "MX";
		else if (language == "es_ES") return "ES";
	}
	else
	{
		if (language == "en")
			return (region & (1 << 1)) ? "US" : "GB";
		else if (language == "pt_PT" || language == "pt_BR")
			return (region & (1 << 1)) ? "BR" : "PT";
		else if (language == "es_US" || language == "es_ES")
			return (region & (1 << 1)) ? "MX" : "ES";
	}

	for (auto &c: language)
		c = toupper(c);
	return language;
}

uint32_t getTicketVersion(cpp3ds::Uint64 tid) // len == 4708
{
	char uri[100];
	const char *tmdTempFile = FREESHOP_DIR "/tmp/last.tmd";
	static const uint16_t top = 0x140;
	char titleVersion[2];
	uint32_t titleVer;

	snprintf(uri, 100, "http://ccs.cdn.c.shop.nintendowifi.net/ccs/download/%016llX/tmd", tid);
	Download tmd(uri, tmdTempFile);
	tmd.run();
	
	std::ifstream tmdfs;
	tmdfs.open(FREESHOP_DIR "/tmp/last.tmd", std::ofstream::out | std::ofstream::in | std::ofstream::binary);
	tmdfs.seekg(top + 0x9C, std::ios::beg);
	tmdfs.read(titleVersion, 0x2);
	tmdfs.close();

	titleVer = (uint32_t)titleVersion[0] << 8 | (uint32_t)titleVersion[1];
	std::cout << titleVer << std::endl;

	return titleVer;
}


} // namespace FreeShop
