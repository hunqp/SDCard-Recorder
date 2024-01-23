#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mount.h>

#include "SDCard.h"
#include "utilities.h"

#define LOCAL_DBG_EN			(1)

#if (LOCAL_DBG_EN == 1)
#define LOCAL_DBG(fmt, ...) 	printf("\x1B[36m" fmt "\x1B[0m", ##__VA_ARGS__)
#else
#define LOCAL_DBG(fmt, ...)
#endif


SDCard::SDCard(std::string hardDrive) {
	mPOSIXMutex = PTHREAD_MUTEX_INITIALIZER;

	this->hardDrive.assign(hardDrive);

	struct statfs fsStat;
    /* Query the f_type field to determine if the filesystem is mounted */
    if (statfs(hardDrive.c_str(), &fsStat) == 0) {
        umount2(hardDrive.c_str(), MNT_FORCE | MNT_DETACH);
    }
}

SDCard::~SDCard() {
	if (mState == eState::Mounted) {
		setOperation(eOperations::Unmount);
	}
	pthread_mutex_destroy(&mPOSIXMutex);
}

void SDCard::assignMountPoint(std::string mountPoint) {
	this->mountPoint.assign(mountPoint);
	
	/* Create directory for mounting SD Card when it has inserted */
	createDirectory(mountPoint.c_str());

	if (hasMountPoint()) {
		umount2(mountPoint.c_str(), MNT_FORCE | MNT_DETACH);
	}
}

void SDCard::lockPOSIXMutex() {
	pthread_mutex_lock(&mPOSIXMutex);
}

void SDCard::unLockPOSIXMutex() {
	pthread_mutex_unlock(&mPOSIXMutex);
}

int SDCard::setOperation(eOperations oper) {
	int ret = SDCARD_RETURN_SUCCESS;

	if (mountPoint.empty()) {
		return SDCARD_MOUNT_FAILURE;
	}

	switch (oper) {
	case eOperations::Mount: {
		ret = mount(hardDrive.c_str(), mountPoint.c_str(), SDCARD_FORMAT_TYPE, MS_RELATIME, "") == 0 ? SDCARD_RETURN_SUCCESS : SDCARD_MOUNT_FAILURE;
	}
	break;

	case eOperations::Unmount: {
		ret = (umount2(mountPoint.c_str(), MNT_FORCE | MNT_DETACH) != -1) ? SDCARD_RETURN_SUCCESS : SDCARD_UNMOUNT_FAILURE;
	}
	break;

	case eOperations::Format: {
		std::string fmtCmd = SDCARD_FORMAT_COMMAND + std::string(" ") + hardDrive;
		LOCAL_DBG("SD Card fmt by command: %s\n", fmtCmd.c_str());
		umount2(mountPoint.c_str(), MNT_FORCE | MNT_DETACH);
		ret = (system(fmtCmd.c_str()) == 0 ? SDCARD_RETURN_SUCCESS : SDCARD_FORMAT_FAILURE); /* The program will wait for this complete */
	}
	break;
	
	default:
	break;
	}

	return ret;
}

bool SDCard::isInserted() {
#if 1
	struct stat fStat;
	bool ret = (stat(hardDrive.c_str(), &fStat) != -1) ? true : false;
#else
	bool ret = access(hardDrive.c_str(), R_OK) == 0 ? true : false;
#endif

	return ret;
}

bool SDCard::hasMountPoint() {
	struct stat pathStat, parentStat;
	bool ret = false;

	if ((stat(mountPoint.c_str(), &pathStat) != -1) && (stat("..", &parentStat) != -1)) {
		ret = (pathStat.st_dev != parentStat.st_dev) ? true : false;
	}

    return ret;
}

bool SDCard::isVFatFmt() {
	struct statfs fStatfs;
	bool ret = false;

    if (statfs(mountPoint.c_str(), &fStatfs) != -1) {
		ret = (fStatfs.f_type == 0x4D44) ? true : false; /* REFERENCE https://man7.org/linux/man-pages/man2/statfs.2.html */
    }

	return ret;
}

void SDCard::updateCapacity() {
	struct statfs fStatfs;

	if (statfs(mountPoint.c_str(), &fStatfs) != -1) {
		memset(&mCapacity, 0, sizeof(mCapacity));
#if 0
		mCapacity.total = (int)(((uint64_t)fStatfs.f_blocks * (uint64_t)fStatfs.f_bsize) >> 10);
		mCapacity.used = (int)((((uint64_t)fStatfs.f_blocks - (uint64_t)fStatfs.f_bfree) * (uint64_t)fStatfs.f_bsize) >> 10);
		mCapacity.free = (int)(((uint64_t)fStatfs.f_bavail * (uint64_t)fStatfs.f_bsize) >> 10);
#else
		size_t blockSize = fStatfs.f_bsize;
		mCapacity.total = blockSize * fStatfs.f_blocks;
		mCapacity.free = blockSize * fStatfs.f_bfree;
		mCapacity.used = blockSize * (fStatfs.f_blocks - fStatfs.f_bfree);
#endif
	}
}

std::vector<RecordDesc> SDCard::getAllPlaylists(std::string dateTime, Recorder::eOption opt) {
	std::vector<RecordDesc> listRecords;

	if (mState == eState::Mounted) {
		qryPlayList(listRecords, dateTime, opt);
	}

	return listRecords;
}

static bool sortListByTime(RecordDesc &t1, RecordDesc &t2) {
	if (t1.sortTime.hou != t2.sortTime.hou) {
		return t1.sortTime.hou > t2.sortTime.hou;
	}

	if (t1.sortTime.min != t2.sortTime.min) {
		return t1.sortTime.min > t2.sortTime.min;
	}

	return (t1.sortTime.sec > t2.sortTime.sec);
}

static RecordDesc parseRecordDesc(std::string &desc, Recorder::eOption opt) {
	RecordDesc recordDesc;
	std::tm tmStart, tmStop;
	auto strings = splitString(desc, '_');

	switch (opt) {
	case Recorder::eOption::Motion: {
		if (IS_MOTION_RECORD(strings.size()) == false) {
			return recordDesc;
		}
	}
	break;
	
	case Recorder::eOption::Full: {
		if (IS_FULL_RECORD(strings.size()) == false) {
			return recordDesc;
		}
	}
	break;

	default:
	break;
	}

	epochToUTCTime(stoi(strings[1]), tmStart);
	epochToUTCTime(stoi(strings[2]), tmStop);
	recordDesc.sortTime.hou = tmStart.tm_hour;
	recordDesc.sortTime.min = tmStart.tm_min;
	recordDesc.sortTime.sec = tmStart.tm_sec;

	recordDesc.type				= (uint8_t)opt;
	recordDesc.fileName 		= sprintfString("%s_%s_%s", strings[0].c_str(), strings[1].c_str(), strings[2].c_str());
	recordDesc.beginTime 		= sprintfString("%d.%02d.%02d %02d:%02d:%02d", tmStart.tm_year, tmStart.tm_mon, tmStart.tm_mday, tmStart.tm_hour, tmStart.tm_min, tmStart.tm_sec);
	recordDesc.endTime 			= sprintfString("%d.%02d.%02d %02d:%02d:%02d", tmStop.tm_year, tmStop.tm_mon, tmStop.tm_mday, tmStop.tm_hour, tmStop.tm_min, tmStop.tm_sec);
	recordDesc.durationInSecs 	= stoi(strings[2]) - stoi(strings[1]);
	
	return recordDesc;
}

static void eraseSuffix(std::string pathTo, std::string &recordDesc, std::string suffixErase) {
	std::string pathToFileNeedRecover = pathTo + "/" + recordDesc;
    size_t pos = recordDesc.find(suffixErase);

	if (pos != std::string::npos) {
		recordDesc.erase(pos, suffixErase.length());
	}
	std::string pathToFileRecovered = pathTo + "/" + recordDesc;
	rename(pathToFileNeedRecover.c_str(), pathToFileRecovered.c_str());
}

void SDCard::qryPlayList(std::vector<RecordDesc> &listRecords, std::string dateTime, Recorder::eOption opt) {
	std::string pathToVideoLists = mountPoint + std::string("/video/") + dateTime;

	LOCAL_DBG("Path to video lists: %s\n", pathToVideoLists.c_str());

	DIR *dir = opendir(pathToVideoLists.c_str());
	if (dir == nullptr) {
		LOCAL_DBG("SD Card opens failure, error: %s", strerror(errno));
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
			std::string videoDesc(ent->d_name, strlen(ent->d_name));

			size_t pos = 0;
			pos = videoDesc.find(".tmp");

			/* Recovery record temporary */
			if (pos != std::string::npos) {
				if (dateTime == getTodayDateString()) {
					continue;
				}
				/* Change video description to audio description */
				std::string audioDesc = videoDesc;
				size_t ind = audioDesc.find("_13");
				audioDesc = audioDesc.erase(ind, strlen("_13"));
				ind = audioDesc.find(FILE_VIDEO_RECORD_EXTENSION);
				audioDesc = audioDesc.replace(ind, strlen(FILE_VIDEO_RECORD_EXTENSION), FILE_AUDIO_RECORD_EXTENSION);
				eraseSuffix(std::string(mountPoint + std::string("/video/") + dateTime), videoDesc, RECORD_TEMPORARY_SUFFIX);
				eraseSuffix(std::string(mountPoint + std::string("/audio/") + dateTime), audioDesc, RECORD_TEMPORARY_SUFFIX);
			}

			pos = videoDesc.find('.');
			videoDesc = videoDesc.substr(0, pos);
			auto descParsed = parseRecordDesc(videoDesc, opt);
			if (descParsed.durationInSecs >= 10) { /* Minimum 10 Seconds */
				listRecords.emplace_back(descParsed);
			}
		}
	}

	if (listRecords.size()) {
		sort(listRecords.begin(), listRecords.end(), sortListByTime);
	}
	closedir(dir);
}

void SDCard::ENTRY_ATOMIC(SDCard &sdCard) {
	sdCard.lockPOSIXMutex();
}

void SDCard::EXIT_ATOMIC(SDCard &sdCard) {
	sdCard.unLockPOSIXMutex();
}

bool SDCard::isSDCardMounted(SDCard &sdCard) {
	bool ret = false;

	if (!sdCard.isInserted()) {
		if (sdCard.eStateSD != eState::Removed) {
			sdCard.setOperation(eOperations::Unmount);

			std::cout << "Stop new session recording !!!" << std::endl;
			closeSessionFullRec(sdCard);
		}
		sdCard.eStateSD = eState::Removed;
		return false;
	}

	sdCard.eStateSD = eState::Inserted;

	if (sdCard.hasMountPoint()) {
		sdCard.eStateSD = eState::Mounted;
	}
	else {
		if (sdCard.setOperation(eOperations::Mount) == SDCARD_RETURN_SUCCESS) {
			sdCard.eStateSD = eState::Mounted;
			openSessionFullRec(sdCard);
		}
	}

	if (sdCard.eStateSD == eState::Mounted) {
		sdCard.updateCapacity();
		ret = true;
	}

	return ret;
}

void SDCard::openSessionFullRec(SDCard &sdCard) {
	/* QUERY: If current session record has existed -> OPENING
			  Else -> DO NOTHING
	*/
	if (sdCard.currentSession.empty() != true) {
		return;
	}

	sdCard.currentSession = getTodayDateString();

	/* Create parent directories (Video and audio) */
	std::string parentVideoDirectory = sdCard.mountPoint + "/video";
	std::string parentAudioDirectory = sdCard.mountPoint + "/audio";
	createDirectory(parentVideoDirectory.c_str());
	createDirectory(parentAudioDirectory.c_str());

	/* Create child directories (Current datetime) */
	std::string videoRecordsTodayPath = parentVideoDirectory + "/" + sdCard.currentSession;
	std::string audioRecordsTodayPath = parentAudioDirectory + "/" + sdCard.currentSession;
	createDirectory(videoRecordsTodayPath.c_str());
	createDirectory(audioRecordsTodayPath.c_str());

	sdCard.videoRecorder = std::make_shared<Recorder>(videoRecordsTodayPath, Recorder::eType::Video, Recorder::eOption::Full);
	sdCard.audioRecorder = std::make_shared<Recorder>(audioRecordsTodayPath, Recorder::eType::Audio, Recorder::eOption::Full);
}

void SDCard::closeSessionFullRec(SDCard &sdCard) {
	/* QUERY: If current session record has existed -> CLOSING
			  Else -> DO NOTHING
	*/
	if (sdCard.currentSession.empty() == true) {
		return;
	}

	sdCard.currentSession.clear();
	sdCard.videoRecorder->getStop();
	sdCard.audioRecorder->getStop();
	sdCard.videoRecorder.reset();
	sdCard.audioRecorder.reset();
}

int SDCard::storageSamples(std::shared_ptr<Recorder> rec, uint8_t *sample, size_t totalSample, uint32_t lastTimestampUpdate) {
	std::string recPresent = rec->getCurrentInstance();

	if (recPresent.empty()) {
		if (rec->getStart() == RECORD_RETURN_FAILURE) {
			return SDCARD_STORAGE_FAILURE;
		}
	}

	if (rec->getStorage(sample, totalSample, lastTimestampUpdate) != RECORD_RETURN_SUCCESS) {
		return SDCARD_STORAGE_FAILURE;
	}

	if (rec->isCompleted()) {
		rec->getStop();
	}

	return SDCARD_RETURN_SUCCESS;
}