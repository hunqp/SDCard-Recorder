/*
    Typical records format:
        <Year><Month><Day><Hour><Month><Second>_<StartTimestamp>_<StopTimestamp>_<Extension>
    
    <Extension>:
        Video Full Record:          <FPS>.h264
        Video Motion Detect Record: <FPS>_mdt.h264
        Audio Full Record:          .g711a
        Audio Motion Detect Record: mdt.g711a
*/
#ifndef __RECORDER_H
#define __RECORDER_H

#include <stdint.h>
#include <string>

#define FILE_RECORD_STRING_FORMAT           "%d%02d%02d%02d%02d%02d_%d_%d"

#define RECORD_TEMPORARY_SUFFIX             ".tmp"
#define FILE_VIDEO_RECORD_EXTENSION         ".h264"
#define FILE_AUDIO_RECORD_EXTENSION         ".g711"

#define FILE_VIDEO_RECORD_TEMPORARY         FILE_VIDEO_RECORD_EXTENSION RECORD_TEMPORARY_SUFFIX
#define FILE_AUDIO_RECORD_TEMPORARY         FILE_AUDIO_RECORD_EXTENSION RECORD_TEMPORARY_SUFFIX

#define RECORD_RETURN_SUCCESS               (1)
#define RECORD_RETURN_FAILURE               (-1)

#define IS_FORMAT_PARSED_VALID(nbParsed)    (nbParsed >= 3 ? true : false)
#define IS_MOTION_RECORD(nbParsed)          (nbParsed == 5 ? true : false)
#define IS_FULL_RECORD(nbParsed)            (nbParsed == 4 ? true : false)

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
    int getStorage(uint8_t *sample, size_t totalSample, uint32_t lastTimestampUpdate);
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

    void updateLastTimestampRecord(uint32_t lastTimestampUpdate);

public:
    std::string pathToRecords;
};


#endif /* __RECORDER_H */