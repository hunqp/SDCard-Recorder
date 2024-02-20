#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <iostream>

#include "recorder.h"
#include "utilitiesd.hpp"


#define LOCAL_DBG_EN			(1)

#if (LOCAL_DBG_EN == 1)
#define LOCAL_DBG(fmt, ...) 	printf("\x1B[36m" fmt "\x1B[0m", ##__VA_ARGS__)
#else
#define LOCAL_DBG(fmt, ...)
#endif

uint32_t Recorder::startTimestamp = 0;
uint32_t Recorder::endTimestamp = 0;

Recorder::Recorder(std::string pathToRecords,
                   eType type, 
                   eOption option,
                   int durationInSecs)
{
    this->pathToRecords.assign(pathToRecords);
    this->mType = type;
    this->mOption = option;
    this->mDurationInSecs = durationInSecs;

    this->mExtension += (mType == eType::Video)      ? "_13"  : "";
    this->mExtension += (mOption == eOption::Motion) ? "_mdt" : "";
    this->mExtension += (mType == eType::Video)      ? FILE_VIDEO_RECORD_TEMPORARY : FILE_AUDIO_RECORD_TEMPORARY;
}

Recorder::~Recorder() {

}

int Recorder::getStart() {
    std::tm tm;
    std::string fmt = FILE_RECORD_STRING_FORMAT + mExtension;

    if (Recorder::startTimestamp == 0) {
        Recorder::startTimestamp = getCurrentEpochTimestamp();
        Recorder::endTimestamp = mLastTimestampUpdated = Recorder::startTimestamp;
    }
	epochToUTCTime(startTimestamp, tm);

    fmt = sprintfString(fmt, 
                        tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                        Recorder::startTimestamp, 
                        Recorder::endTimestamp);

    mTarget.assign(pathToRecords + "/" + fmt);

    int fd = open(mTarget.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
	if (fd == -1) {
        mTarget.clear();
		return RECORD_RETURN_FAILURE;
    }

    close(fd);

    LOCAL_DBG("[START] Target : %s\n", mTarget.c_str());

    return RECORD_RETURN_SUCCESS;
}

int Recorder::getStop() {
    int ret = RECORD_RETURN_FAILURE;
    const std::string searchString = std::string(".tmp");
    std::string targetRename = mTarget;

    if (!targetRename.empty()) {
        size_t pos = targetRename.find(searchString);
        if (pos != std::string::npos) {
            targetRename.erase(pos, searchString.length());
            rename(mTarget.c_str(), targetRename.c_str());
            ret = RECORD_RETURN_SUCCESS;
            mTarget.clear();
            LOCAL_DBG("Rename %s to %s\n", mTarget.c_str(), targetRename.c_str());
        }
    }

    Recorder::startTimestamp = 0;

    return ret;
}

int Recorder::getStorage(uint8_t *sample, size_t totalSample) {
	int ret = RECORD_RETURN_FAILURE, fd;
    ssize_t nbOfBytes = 0;
	
    fd = open(mTarget.c_str(), O_RDWR | O_APPEND, 0666);
	if (fd == -1) {
		LOCAL_DBG("[ERROR] Open : %s\n", mTarget.c_str());
        return RECORD_RETURN_FAILURE;
    }

    try {
        nbOfBytes = write(fd, sample, totalSample);
        fsync(fd);
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
	}

    close(fd);

	if (nbOfBytes > 0) {
        updateLastTimestampRecord();
		ret = RECORD_RETURN_SUCCESS;
	}

    return ret;
}

void Recorder::updateLastTimestampRecord() {
	std::string replacementString = std::to_string(endTimestamp);
    auto strings = splitString(mTarget, '_');
   
    if (!IS_FORMAT_PARSED_VALID(strings.size())) {
        return;
    }

    if (mLastTimestampUpdated != Recorder::endTimestamp) {
        mLastTimestampUpdated = Recorder::endTimestamp;
        strings[2].assign(std::to_string(mLastTimestampUpdated));
        
        std::string targetRename = strings[0] + "_" + strings[1] + "_" + strings[2] + mExtension;
        rename(mTarget.c_str(), targetRename.c_str());
        mTarget.assign(targetRename);
    }
}

std::string Recorder::getCurrentInstance() {
    return mTarget;
}

bool Recorder::isCompleted() {
    auto strings = splitString(mTarget, '_');
    auto startTimestamp = stoi(strings[1]);
    auto stopTimestamp = stoi(strings[2]);
    auto durationInSecs = stopTimestamp - startTimestamp;

    return (durationInSecs >= mDurationInSecs) ? true : false;
}

