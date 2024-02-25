#include <bits/stdc++.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "SDCard.h"
#include "recorder.h"
#include "utils.hpp"

static SDCard SDCARD("/dev/sdb1");

static uint8_t samplesH264 = 0;
static uint8_t samplesG711 = 0;
static pthread_t threadPollingDateTimeId;
static pthread_t threadStorageH264SamplesId;
static pthread_t threadStorageG711SamplesId;


static void signalHandler(int signal);
static void setupBeforeOpenSession();
static void* pollingDateTime(void *arg);
static void* storageH264Samples(void *arg);
static void* storageG711Samples(void *arg);


int main() {
    std::signal(SIGINT, signalHandler);

    char pwd[256];
    assert(getcwd(pwd, sizeof(pwd)) != NULL);
    std::string pathToRecords(pwd, strlen(pwd));
    pathToRecords += "/records";

    std::cout << "Path to records : " << pathToRecords << std::endl;

#if 0
    SDCARD.assignMountPoint(pathToRecords);
    SDCARD.eStatus = SDCard::eState::Mounted;

    setupBeforeOpenSession();

    if (!SDCARD.currentSession.empty()) {
        pthread_create(&threadPollingDateTimeId,    NULL, pollingDateTime,      NULL);
        pthread_create(&threadStorageH264SamplesId, NULL, storageH264Samples,   NULL);
        pthread_create(&threadStorageG711SamplesId, NULL, storageG711Samples,   NULL);

        pthread_join(threadPollingDateTimeId,    NULL);
        pthread_join(threadStorageH264SamplesId, NULL);
        pthread_join(threadStorageG711SamplesId, NULL);
    }
    else {
        std::cout << "[OPEN] Session Record Opening Failed" << std::endl;
    }
#else
    std::string url = pathToRecords + "/video/2024.02.25/20240225195708_1708865828_1708865844.h264";
    struct stat STAT;

    printf("URL: %s\n", url.c_str());

    if (stat(url.c_str(), &STAT) == 0) {
        time_t lastModifiedTime = STAT.st_mtime;

        printf("Last Modification Time:\t%d\n", lastModifiedTime);
        printf("Last Modification Time:\t%s\n", ctime(&lastModifiedTime));
    }
    else {
        printf("SIG -> FAILED\n");
    }

#endif

    return 0;
}

void signalHandler(int signal) {
    (void)signal;

    SDCard::ENTRY_ATOMIC(SDCARD);
    {
        if (SDCARD.currentSession.empty() == false) {
            SDCARD.videoRecorder->getStop();
            SDCARD.audioRecorder->getStop();
            SDCard::closeCurrentSession(SDCARD);
        }
        
        std::string today = getTodayDateString();
        auto listRecords = SDCARD.getAllPlaylists(today, SDCard::eQryPlaylist::Full);

        printf("Total full records : %ld\n", listRecords.size());
        if (listRecords.size()) {   
            for (auto it : listRecords) {
                printf("%s, [%s - %s]\n", 
                                    it.fileName.c_str(), 
                                it.beginTime.c_str(), 
                                it.endTime.c_str());
            }
        }

        listRecords = SDCARD.getAllPlaylists(today, SDCard::eQryPlaylist::All);
        printf("Total motion records : %ld\n", listRecords.size());
        if (listRecords.size()) {   
            for (auto it : listRecords) {
                printf("%s, [%s - %s]\n", 
                                it.fileName.c_str(), 
                                it.beginTime.c_str(), 
                                it.endTime.c_str());
            }
        }
    }

    SDCard::EXIT_ATOMIC(SDCARD);

    std::cout << std::endl;
    std::cout << "Application exit !!!" << std::endl;

    std::exit(EXIT_SUCCESS);
}

void setupBeforeOpenSession() {
    SDCard::ENTRY_ATOMIC(SDCARD);
    {
        bool isMounted = false;

    #if 0 /* Query SD Card has mounted or unmounted */
        isMounted = SDCard::isSDCardMounted(SDCARD);
    #else
        isMounted = true; /* TODO: Assign for testing */
        SDCARD.eStatus = SDCard::eState::Mounted;
    #endif

        if (!isMounted) {
            return;
        }

        if (SDCARD.currentSession.empty()) {
            SDCard::openSessionRecord(SDCARD, Recorder::eOption::Full);
            std::cout << "[START] Record Session " << SDCARD.currentSession << std::endl;
        }
    }
    SDCard::EXIT_ATOMIC(SDCARD);
}

void* pollingDateTime(void *arg) {
    (void)arg;

    while (1) {
        std::cout << "[QUERY] Datetime Session Record" << std::endl;

        if (getTodayDateString() != SDCARD.currentSession) {
            SDCard::closeCurrentSession(SDCARD);
            setupBeforeOpenSession();
        }

        sleep(60);
    }

    return NULL;
}

void* storageH264Samples(void *arg) {
    (void)arg;

    while (1) {
        SDCard::ENTRY_ATOMIC(SDCARD);
        {
        #if 0
            bool isMounted = SDCard::isSDCardMounted(SDCARD);
        #else
            bool isMounted = true;
        #endif
            if (isMounted && SDCARD.currentSession.empty() == false) {
                samplesH264 += 1;

                {
                    /* 
                        TODO: 
                        Code segment check capacity of SD Card (Clear oldest directory or file oldest)
                    */
                }

                int ret = SDCard::storageSamples(SDCARD.videoRecorder, (uint8_t*)&samplesH264, sizeof(samplesH264));
                if (ret == SDCARD_RETURN_SUCCESS) {
                    std::cout << "[STORAGE] " << SDCARD.videoRecorder->getCurrentInstance() << std::endl;
                }
            }
        }
        SDCard::EXIT_ATOMIC(SDCARD);

        sleep(1);
    }

    return NULL;
}

static void updateEndTimestamp() {
    uint32_t u32 = getCurrentEpochTimestamp();
    if (Recorder::endTimestamp != u32) {
        Recorder::endTimestamp = u32;
    }
}

void* storageG711Samples(void *arg) {
    (void)arg;

    while (1) {
        SDCard::ENTRY_ATOMIC(SDCARD);
        {
            

        #if 0
            bool isMounted = SDCard::isSDCardMounted(SDCARD);
        #else
            bool isMounted = true;
        #endif
    
            if (isMounted && SDCARD.currentSession.empty() == false) {
                updateEndTimestamp();

                samplesG711 += 2;
                int ret = SDCard::storageSamples(SDCARD.audioRecorder, (uint8_t*)&samplesG711, sizeof(samplesG711));
                if (ret == SDCARD_RETURN_SUCCESS) {
                    std::cout << "[STORAGE] " << SDCARD.audioRecorder->getCurrentInstance() << std::endl;
                }
            }
        }
        SDCard::EXIT_ATOMIC(SDCARD);

        sleep(1);
    }

    return NULL;
}