#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/mount.h>

#include "SDCard.h"
#include "utils.hpp"

#define LOCAL_DBG_EN			(0)

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
	if (currentSession.empty() == false) {
		videoRecorder->getStop();
		audioRecorder->getStop();
	}

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
	bool ret = false;
	struct statvfs statvFs;

	if (statvfs(mountPoint.c_str(), &statvFs) == 0) {
		if (statvFs.f_fsid != 0) {
			ret = true;
		}
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

	memset(&mCapacity, 0, sizeof(mCapacity));

	if (statfs(mountPoint.c_str(), &fStatfs) != -1) {
		size_t blockSize = (uint64_t)fStatfs.f_bsize;
		mCapacity.total  = (uint64_t)blockSize * (uint64_t)fStatfs.f_blocks;
		mCapacity.free   = (uint64_t)blockSize * (uint64_t)fStatfs.f_bfree;
		mCapacity.used   = (uint64_t)blockSize * ((uint64_t)fStatfs.f_blocks - (uint64_t)fStatfs.f_bfree);
	}
}

int SDCard::getTotalSessionRecords() {
	int counts = 0;

	DIR *dir = opendir(mountPoint.c_str());
    if (!dir) {
        return 0;
    }

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            ++counts;
        }
    }
    closedir(dir);

	return counts;
}

void SDCard::eraseRecord(std::string dateTime, std::string videoDesc) {
	std::string pathToVideoLists = mountPoint + std::string("/video/") + dateTime;
	std::string pathToAudioLists = mountPoint + std::string("/audio/") + dateTime;
	std::string audioDesc = videoDesc;

	audioDesc.replace(audioDesc.find(FILE_VIDEO_RECORD_EXTENSION), 
						strlen(FILE_VIDEO_RECORD_EXTENSION), 
						FILE_AUDIO_RECORD_EXTENSION); /* Change extension ".h264" to ".g711" */

	LOCAL_DBG("Erase file video %s\n", std::string(pathToVideoLists + "/" + videoDesc).c_str());
	LOCAL_DBG("Erase file audio %s\n", std::string(pathToVideoLists + "/" + audioDesc).c_str());

	remove(std::string(pathToVideoLists + "/" + videoDesc).c_str());
	remove(std::string(pathToVideoLists + "/" + audioDesc).c_str());
}

void SDCard::eraseFolder(std::string dateTime) {
	std::string pathToVideoLists = mountPoint + std::string("/video/") + dateTime;
	std::string pathToAudioLists = mountPoint + std::string("/audio/") + dateTime;

	LOCAL_DBG("Erase folder video %s\n", pathToVideoLists.c_str());
	LOCAL_DBG("Erase folder audio %s\n", pathToAudioLists.c_str());

	std::string cmd = "rm -rf ";
	std::system(std::string(cmd + pathToVideoLists).c_str());
	std::system(std::string(cmd + pathToAudioLists).c_str());
}

std::vector<RecordDesc> SDCard::getAllPlaylists(std::string dateTime, eQryPlaylist type) {
	std::vector<RecordDesc> listRecords;

	if (mState == eState::Mounted) {
		qryPlayList(listRecords, dateTime, type);
	}

	return listRecords;
}

static std::string findOldestRecord(std::string pathToList) {
	DIR *dir = opendir(pathToList.c_str());
	if (dir == nullptr) {
		LOCAL_DBG("SD Card opens failure, error: %s", strerror(errno));
		return "";
	}

	struct dirent *ent;
	uint32_t oldestTimestamp = UINT32_MAX;
	std::string oldestString = "";

	/* Query to find oldest file record */
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
			std::string fileDesc(ent->d_name, strlen(ent->d_name));

			uint32_t u32 = getBirthTimestamp(std::string(pathToList + "/" + fileDesc).c_str());
			if (oldestTimestamp > u32) {
				oldestTimestamp = u32;
				oldestString = fileDesc;
			}
		}
	}

	return oldestString;
}

void SDCard::eraseOldestRecords(std::string dateTime) {
	std::string pathToVideoLists = mountPoint + std::string("/video") + (dateTime.empty() ? "" : ("/" + dateTime));
	std::string pathToAudioLists = mountPoint + std::string("/audio") + (dateTime.empty() ? "" : ("/" + dateTime));

	std::string oldest = findOldestRecord(pathToVideoLists);

	/* Erase oldest folder if it has found */
	if (oldest.empty() == false) {
		LOCAL_DBG("%s is oldest -> Must be DELETED\n", oldest.c_str());

		if (dateTime.empty()) {
			eraseRecord(dateTime, oldest);
		}
		else {
			eraseFolder(oldest);
		}
	}
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

static RecordDesc parseRecordDesc(std::vector<std::string> strings, SDCard::eQryPlaylist type) {
	RecordDesc recordDesc;
	std::tm tmStart, tmStop;
	std::string fname;

	switch (type) {
	case SDCard::eQryPlaylist::Full: {
		if (IS_FULL_RECORD(strings.size()) == false) {
			return recordDesc;
		}
	}
	break;

	case SDCard::eQryPlaylist::Motion: {
		if (IS_MOTION_RECORD(strings.size()) == false) {
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

	std::string sfmt = (type == SDCard::eQryPlaylist::Motion) ? "%s_%s_%s_mdt" : "%s_%s_%s";
	
	/* Remove extension to get stop timestamp */
	size_t pos = strings[2].find(".");
	if (pos != std::string::npos) {
		strings[2].erase(pos);
	}

	recordDesc.type				= (uint8_t)type;
	recordDesc.fileName 		= sprintfString(sfmt, strings[0].c_str(), strings[1].c_str(), strings[2].c_str());
	recordDesc.beginTime 		= sprintfString("%d.%02d.%02d %02d:%02d:%02d", tmStart.tm_year, tmStart.tm_mon, tmStart.tm_mday, tmStart.tm_hour, tmStart.tm_min, tmStart.tm_sec);
	recordDesc.endTime 			= sprintfString("%d.%02d.%02d %02d:%02d:%02d", tmStop.tm_year, tmStop.tm_mon, tmStop.tm_mday, tmStop.tm_hour, tmStop.tm_min, tmStop.tm_sec);
	recordDesc.durationInSecs 	= stoi(strings[2]) - stoi(strings[1]);

	return recordDesc;
}

static bool isExisted(std::string pathToFolder, std::string desc) {
	return (access(std::string(pathToFolder + "/" + desc).c_str(), F_OK) == 0);
}

void SDCard::qryPlayList(std::vector<RecordDesc> &listRecords, std::string dateTime, eQryPlaylist type) {
	std::string pathToVideoLists = mountPoint + std::string("/video/") + dateTime;
	std::string pathToAudioLists = mountPoint + std::string("/audio/") + dateTime;

	LOCAL_DBG("Path to video lists: %s\n", pathToVideoLists.c_str());
	LOCAL_DBG("Path to audio lists: %s\n", pathToVideoLists.c_str());

	DIR *dir = opendir(pathToVideoLists.c_str());
	if (dir == nullptr) {
		LOCAL_DBG("SD Card opens failure, error: %s", strerror(errno));
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {

			/* Get audio & video records description */
			std::string videoDesc(ent->d_name, strlen(ent->d_name));

			auto strings = splitString(videoDesc, '_');
			if (!IS_FORMAT_PARSED_VALID(strings.size())) {
				continue;
			}

			std::string audioDesc = videoDesc;
			audioDesc.replace(audioDesc.find(FILE_VIDEO_RECORD_EXTENSION), 
							  strlen(FILE_VIDEO_RECORD_EXTENSION), 
							  FILE_AUDIO_RECORD_EXTENSION); /* Change extension ".h264" to ".g711" */

			/* Video record exist but audio not exist -> Ignore it */
			if (isExisted(pathToAudioLists, audioDesc) == false) {
				continue;
			}

			/* Recovery record temporary to valid record if it's not recording (TODO: Remove ".tmp") */
			if (std::stoul(strings[1]) == Recorder::startTimestamp) {
				continue;
			}

			size_t pos = videoDesc.find(RECORD_TEMPORARY_SUFFIX);
			if (pos != std::string::npos) {
				std::string oldVideoDesc = videoDesc;
				std::string oldAudioDesc = audioDesc;

				videoDesc.erase(pos, strlen(RECORD_TEMPORARY_SUFFIX));
				audioDesc.erase(audioDesc.find(RECORD_TEMPORARY_SUFFIX), strlen(RECORD_TEMPORARY_SUFFIX));

				LOCAL_DBG("Rename %s to %s\n", 
									std::string(pathToVideoLists + "/" + oldVideoDesc).c_str(),
									std::string(pathToVideoLists + "/" + videoDesc).c_str());

				rename(std::string(pathToVideoLists + "/" + oldVideoDesc).c_str(), std::string(pathToVideoLists + "/" + videoDesc).c_str());
				rename(std::string(pathToAudioLists + "/" + oldAudioDesc).c_str(), std::string(pathToAudioLists + "/" + audioDesc).c_str());
			}

			auto descParsed = parseRecordDesc(strings, type);
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
		if (sdCard.eStatus != eState::Removed) {
			sdCard.setOperation(eOperations::Unmount);
		}
		sdCard.eStatus = eState::Removed;
		return false;
	}

	sdCard.eStatus = eState::Inserted;

	if (sdCard.hasMountPoint()) {
		sdCard.eStatus = eState::Mounted;
	}
	else {
		if (sdCard.setOperation(eOperations::Mount) == SDCARD_RETURN_SUCCESS) {
			sdCard.eStatus = eState::Mounted;
		}
	}

	if (sdCard.eStatus == eState::Mounted) {
		sdCard.updateCapacity();
		ret = true;
	}

	return ret;
}

void SDCard::openSessionRecord(SDCard &sdCard, Recorder::eOption option) {
	/* Do nothing if current session record is existed */
	if (sdCard.currentSession.empty() == false) {
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

	sdCard.videoRecorder = std::make_shared<Recorder>(videoRecordsTodayPath, Recorder::eType::Video, option);
	sdCard.audioRecorder = std::make_shared<Recorder>(audioRecordsTodayPath, Recorder::eType::Audio, option);
}

void SDCard::closeCurrentSession(SDCard &sdCard) {
	/* Do nothing if current session record isn't existed */
	if (sdCard.currentSession.empty() == true) {
		return;
	}

	sdCard.currentSession.clear();
	sdCard.videoRecorder->getStop();
	sdCard.audioRecorder->getStop();
	sdCard.videoRecorder.reset();
	sdCard.audioRecorder.reset();
}

int SDCard::storageSamples(std::shared_ptr<Recorder> rec, uint8_t *sample, size_t totalSample) {
	std::string currentInstance = rec->getCurrentInstance();

	if (currentInstance.empty()) {
		if (rec->getStart() == RECORD_RETURN_FAILURE) {
			return SDCARD_STORAGE_FAILURE;
		}
	}

	if (rec->getStorage(sample, totalSample) != RECORD_RETURN_SUCCESS) {
		return SDCARD_STORAGE_FAILURE;
	}

	if (rec->isCompleted()) {
		rec->getStop();
	}

	return SDCARD_RETURN_SUCCESS;
}