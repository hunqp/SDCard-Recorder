#ifndef __UTILITIES_H
#define __UTILITIES_H

#include <string>
#include <vector>
#include <ctime>
#include <cstdio>
#include <cstdarg>

template <typename... Args>
std::string sprintfString(std::string fmt, Args... args);

extern std::vector<std::string> splitString(std::string &s, char delimiter);
extern void epochToUTCTime(time_t epochTime, std::tm &tm);
extern std::string getTodayDateString();
extern uint32_t getCurrentEpochTimestamp();
extern void createDirectory(const char *);
extern uint32_t getBirthTimestamp(const char *);

#endif /* __UTILITIES_H */