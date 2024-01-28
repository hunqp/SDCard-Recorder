#include <sys/stat.h>
#include <sys/statfs.h>

#include "utilities.h"


template <typename... Args>
std::string sprintfString(std::string fmt, Args... args) {
    int len = std::snprintf(nullptr, 0, fmt.c_str(), args...);
    if (len < 0) {
        return "";
    }

    char* letters = new char[len + 1];
    std::snprintf(letters, len + 1, fmt.c_str(), args...);
    std::string ret(letters);
    delete[] letters;

    return ret;
}

template std::string sprintfString(std::string, int, int, int, int, int, int, unsigned int, unsigned int);
template std::string sprintfString(std::string, const char*, const char*, const char*);
template std::string sprintfString(std::string, int, int, int, int, int, int);
template std::string sprintfString(std::string, int, int, int);

std::vector<std::string> splitString(std::string &s, char delimeter) {
    std::vector<std::string> stGroups;
    stGroups.clear();
    int startInd = 0, stopInd = 0;

    for (size_t id = 0; id <= s.size(); id++) {
        if (s[id] == delimeter || id == s.size()) {
            stopInd = id;
            std::string tmp;
            tmp.append(s, startInd, stopInd - startInd);
            stGroups.push_back(tmp);
            startInd = stopInd + 1;
        }
    }

    return stGroups;
}

void epochToUTCTime(time_t epochTime, std::tm &tm) {
    tm = *(std::gmtime(&epochTime));
    tm.tm_year  += 1900;  /* Years since 1900 */
    tm.tm_mon   += 1;     /* Months are 0-11 */
    tm.tm_hour  += 7;
}

std::string getTodayDateString() {
	time_t t;
	struct tm* tm = NULL;

    time(&t);
	tm = localtime(&t);
	return sprintfString("%d.%02d.%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

uint32_t getCurrentEpochTimestamp() {
#if 0 /* Can be get from OSD */

#else

	time_t t = time(NULL);

	return (uint32_t)t;
#endif
}

void createDirectory(const char *directory) {
	struct stat fStat;

	if (stat(directory, &fStat) == -1) {
		mkdir(directory, S_IRWXU | S_IRWXG | S_IRWXO);
	}
}

uint32_t getBirthTimestamp(const char *url) {
    struct stat fStat;

    if (stat(url, &fStat) == 0) {
        return (uint32_t)fStat.st_ctime;
    }

    return 0;
}