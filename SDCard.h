#ifndef __SDCARD_H
#define __SDCARD_H

#include <vector>
#include <stdint.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "recorder.h"

#define SDCARD_HARD_DRIVE	    		"/dev/mmcblk0"
#define SDCARD_MOUNT_POINT     			"/tmp/sd"

#define SDCARD_FORMAT_TYPE				"vfat"
#define SDCARD_FORMAT_COMMAND			"mkfs.vfat"

#define SDCARD_RETURN_SUCCESS			(0)
#define SDCARD_MOUNT_FAILURE			(-1)
#define SDCARD_UNMOUNT_FAILURE			(-2)
#define SDCARD_FORMAT_FAILURE			(-3)
#define SDCARD_STORAGE_FAILURE			(-4)

typedef struct {
	uint8_t nbOrder;
	uint8_t type;
	uint32_t durationInSecs = 0;
	std::string fileName;
	std::string beginTime;
	std::string endTime;

	/* This structure support sorting */
	struct {
		uint8_t hou, min, sec;
	} sortTime;
} RecordDesc;

typedef struct {
	uint64_t total;
	uint64_t used;
	uint64_t free;
} MemMang_t;

class SDCard {
public:
	enum class eState {
		Removed,
		Inserted,	
		Mounted,
		InProcess,
	};

	enum class eOperations {
		Mount,
		Unmount,
		Format,
	};

	enum class eQryPlaylist {
		All,
		Full,
		Motion,
	};

	SDCard(std::string hardDrive);
	~SDCard();
	
	void assignMountPoint(std::string mountPoint);
	int setOperation(eOperations oper);
	bool isInserted();
	bool hasMountPoint();
	bool isVFatFmt();
	void updateCapacity();
	int getTotalSessionRecords();
	void eraseOldestRecords(std::string dateTime = "");
	std::vector<RecordDesc> getAllPlaylists(std::string dateTime, eQryPlaylist type);

	void lockPOSIXMutex();
	void unLockPOSIXMutex();

private:
	pthread_mutex_t mPOSIXMutex;
	eState mState = eState::Removed;
	MemMang_t mCapacity;

	void qryPlayList(std::vector<RecordDesc> &listRecords, std::string dateTime, eQryPlaylist type);
	void eraseRecord(std::string dateTime, std::string videoDesc);
	void eraseFolder(std::string dateTime);

public:
	std::string hardDrive;
	std::string mountPoint;

	eState &eStatus = mState;
	
	std::string currentSession;
	std::shared_ptr<Recorder> videoRecorder;
	std::shared_ptr<Recorder> audioRecorder;

	uint64_t &totalCapacity = mCapacity.total;
	uint64_t &usedCapacity = mCapacity.used;
	uint64_t &freeCapacity = mCapacity.free;

	/* Function protect safe accesss to SDCard */
	static void ENTRY_ATOMIC(SDCard &sdCard);
	static void EXIT_ATOMIC(SDCard &sdCard);

	/*  All functions below MUST-BE called in ENTRY_ATOMIC() 
		and EXIT_ATOMIC() to protect operations.
	*/
	static bool isSDCardMounted(SDCard &sdCard);
	static void openSessionRecord(SDCard &sdCard, Recorder::eOption option);
	static void closeCurrentSession(SDCard &sdCard);
	static int storageSamples(std::shared_ptr<Recorder> rec, uint8_t *sample, size_t totalSample);
};

#endif /* __SDCARD_H */