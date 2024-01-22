/*
---------------------------------------------------------------------------------------------------------
|   Video Full Record
|       <Year><Month><Day><Hour><Month><Second>_<StartTimestamp>_<StopTimestamp>_<FPS>.<Extension>
|
|   Video Motion Detect Record (mdt)
|       <Year><Month><Day><Hour><Month><Second>_<StartTimestamp>_<StopTimestamp>_<FPS>_mdt.<Extension>
|
|   Audio Full Record
|       <Year><Month><Day><Hour><Month><Second>_<StartTimestamp>_<StopTimestamp>.<Extension>
|
|   Audio Motion Detect Record (mdt)
|       <Year><Month><Day><Hour><Month><Second>_<StartTimestamp>_<StopTimestamp>_mdt.<Extension>
---------------------------------------------------------------------------------------------------------
*/
#ifndef __RECORDER_H
#define __RECORDER_H

#include <stdint.h>
#include <string>

#define FILE_RECORD_STRING_FORMAT       "%d%02d%02d%02d%02d%02d_%d_%d"

#define FILE_VIDEO_RECORD_EXTENSION     ".h264"
#define FILE_AUDIO_RECORD_EXTENSION     ".g711"

#define RECORD_TEMPORARY_SUFFIX         ".tmp"

#define RECORD_RETURN_SUCCESS           (1)
#define RECORD_RETURN_FAILURE           (-1)

#define IS_MOTION_RECORD(nbParsed)      (nbParsed == 5 ? true : false)
#define IS_FULL_RECORD(nbParsed)        (nbParsed == 4 ? true : false)

class Recorder {
public:
    enum class eType {
        Video,
        Audio,
    };

	enum class eOption {
        Motion,
        Full,
	};

    Recorder(std::string pathToRecords,
             eType type, 
             eOption option, 
             int durationInSecs = 300);
    ~Recorder();

    int getStart();
    int getStop();
    int getStorage(uint8_t *sample, size_t totalSample);
    bool isCompleted();
    std::string getCurrentInstance();

private:
	eType mType;
    eOption mOption;
    int mDurationInSecs;
	uint32_t mStartTimestamp;
	uint32_t mStopTimestamp;
	std::string mExtension;

    std::string mTarget;

    void updateLastTimestampRecord();

public:
    std::string pathToRecords;
};


#endif /* __RECORDER_H */