/**
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 * AUTHOR:		HungPNQ
 * 
 * DATE:		11/01/2024
 * 
 * DESCRIPTION:	The SDCard class provides an abstraction for handling operations
 * 				related to an SD card, such as mounting, unmounting, formatting, 
 * 				and retrieving information about playlists. It also includes 
 * 				functionalities to query and update the capacity of the SD card.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifndef __SDCARD_H
#define __SDCARD_H

#include <vector>
#include <stdint.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "recorder.hpp"

#define SDCARD_HARD_DRIVE	    		"/dev/mmcblk0"
#define SDCARD_MOUNT_PATH     			"/tmp/sdCard"

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
	uint32_t durationInSecs;
	std::string fileName;
	std::string beginTime;
	std::string endTime;

	/* This structure support sorting */
	struct {
		uint8_t hou, min, sec;
	} sortTime;
} RecordDesc;

class SDCard {
public:
	enum class eState {
		Removed,
		Inserted,	
		Mounted,
	};

	enum class eOperations {
		Mount,
		Unmount,
		Format,
	};

	enum class eSessionRecord {
		Open,
		Close
	};

	SDCard(std::string hardDrive, std::string mountPoint);
	~SDCard();
	
	int setOperation(eOperations oper);
	bool isInserted();
	bool hasMountPoint();
	bool isVFatFmt();
	void updateCapacity();
	std::vector<RecordDesc> getAllPlaylists(std::string dateTime);
	
	void lockPOSIXMutex();
	void unLockPOSIXMutex();

private:
	pthread_mutex_t mPOSIXMutex;
	eState mState = eState::Mounted;

	struct {
		size_t total;
		size_t used;
		size_t free;
	} mCapacity;

	void qryPlayList(std::vector<RecordDesc> &listRecords, std::string dateTime);

public:
	std::string hardDrive;
	std::string mountPoint;

	eState &eStateSD = mState;
	
	std::string currentSession;
	std::shared_ptr<Recorder> videoRecord;
	std::shared_ptr<Recorder> audioRecord;

	size_t &totalCapacity = mCapacity.total;
	size_t &usedCapacity = mCapacity.used;
	size_t &freeCapacity = mCapacity.free;

	/* Function protect safe accesss to SDCard */
	static void ENTRY_ATOMIC(SDCard &sdCard);
	static void EXIT_ATOMIC(SDCard &sdCard);

	/*  All functions below MUST-BE called in ENTRY_ATOMIC() 
		and EXIT_ATOMIC() to protect operations.
	*/
	static bool isSDCardMounted(SDCard &sdCard);
	static void openSessionRec(SDCard &sdCard);
	static void closeSessionRec(SDCard &sdCard);
	static int collectSamples(std::shared_ptr<Recorder> rec, uint8_t *sample, size_t totalSample);
};

#define gigaBytesToMegaBytes(n)	((size_t)n * 1024)
#define gigaBytesToKiloBytes(n)	((size_t)n * 1024 * 1024)
#define gigaBytesToBytes(n)		((size_t)n * 1024 * 1024 * 1024)
#define bytesToKiloBytes(n)		((size_t)n / 1024)
#define bytesToMegaBytes(n)		((size_t)n / 1024 / 1024)
#define bytesToGigaBytes(n)		((size_t)n / 1024 / 1024 / 1024)

#endif /* __SDCARD_H */