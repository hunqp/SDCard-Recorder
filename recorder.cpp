#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "recorder.hpp"
#include "utilities.hpp"


#define LOCAL_DBG_EN			(1)

#if (LOCAL_DBG_EN == 1)
#define LOCAL_DBG(fmt, ...) 	printf("\x1B[36m" fmt "\x1B[0m", ##__VA_ARGS__)
#else
#define LOCAL_DBG(fmt, ...)
#endif

Recorder::Recorder(std::string pathToRecords,
                   eType type, 
                   eOption option,
                   int durationInSecs)
{
    this->pathToRecords.assign(pathToRecords);
    this->mType = type;
    this->mOption = option;
    this->mDurationInSecs = durationInSecs;
    this->mExtension.assign(mType == eType::Video ? FILE_VIDEO_RECORD_EXTENSION : FILE_AUDIO_RECORD_EXTENSION);
}

Recorder::~Recorder() {

}

int Recorder::getStart() {
    uint32_t u32Timestamp;
    std::tm tm;
    std::string fmt = FILE_RECORD_STRING_FORMAT;
    
    fmt += (mType == eType::Video)      ? "_13"  : "";
    fmt += (mOption == eOption::Motion) ? "_mdt" : "";
    fmt += mExtension;
    fmt += RECORD_TEMPORARY_SUFFIX;

    u32Timestamp = getCurrentEpochTimestamp();
	epochToUTCTime(u32Timestamp, tm);

    mStartTimestamp = u32Timestamp;
    mStopTimestamp  = mStartTimestamp;
    fmt             = sprintfString(fmt, 
                                    tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                                    mStartTimestamp, 
                                    mStopTimestamp);

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

        targetRename.erase(pos, searchString.length());
        rename(mTarget.c_str(), targetRename.c_str());
        ret = RECORD_RETURN_SUCCESS;
        mTarget.clear();
        LOCAL_DBG("Rename %s to %s\n", mTarget.c_str(), targetRename.c_str());
    }

    return ret;
}

int Recorder::getStorage(uint8_t *sample, size_t totalSample) {
	int ret = RECORD_RETURN_FAILURE;
    int fd;
	
    fd = open(mTarget.c_str(), O_RDWR | O_APPEND, 0666);
	if (fd == -1) {
		LOCAL_DBG("[ERR] Open : %s\n", mTarget.c_str());
        return RECORD_RETURN_FAILURE;
    }

    ssize_t nbrOfBytesWritten = write(fd, sample, totalSample);
    close(fd);

	if (nbrOfBytesWritten > 0) {
        updateLastTimestampRecord();
		ret = RECORD_RETURN_SUCCESS;
	}

    return ret;
}

void Recorder::updateLastTimestampRecord() {
    uint32_t currentEpochTimestamp = getCurrentEpochTimestamp();
	std::string replacementString = std::to_string(currentEpochTimestamp);
    auto strings = splitString(mTarget, '_');
   
    if (!IS_MOTION_RECORD(strings.size()) && !IS_FULL_RECORD(strings.size())) {
        return;
    }

    if (mStopTimestamp != currentEpochTimestamp) {
        mStopTimestamp = currentEpochTimestamp;
        strings[2].assign(std::to_string(currentEpochTimestamp));
        
        std::string targetRename;
        for (uint8_t id = 0; id < strings.size(); ++id) {
            targetRename += strings[id];
            targetRename += (id != (strings.size() - 1)) ? "_" : "";
        }

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
