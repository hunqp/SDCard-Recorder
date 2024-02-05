#include <bits/stdc++.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "SDCard.hpp"
#include "recorder.hpp"
#include "utilitiesd.hpp"

static SDCard SDRecords("/dev/sdb1");

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

    char currentPath[256];
    assert(getcwd(currentPath, sizeof(currentPath)) != NULL);
    std::string pathToRecords(currentPath, strlen(currentPath));
    pathToRecords += "/records";

    std::cout << "Path to records : " << pathToRecords << std::endl;

    SDRecords.assignMountPoint(pathToRecords);
    SDRecords.eStateSD = SDCard::eState::Mounted;

#if 1
    setupBeforeOpenSession();

    if (!SDRecords.currentSession.empty()) {
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
#endif

#if 1
    SDCard::ENTRY_ATOMIC(SDRecords);
    {
        if (SDRecords.currentSession.empty() == false) {
            SDRecords.videoRecorder->getStop();
            SDRecords.audioRecorder->getStop();
            SDCard::closeSessionFullRec(SDRecords);
        }
        
        std::string qDay = "2024.01.25";
        auto listRecords = SDRecords.getAllPlaylists(qDay, Recorder::eOption::Full);

        printf("Total full records : %ld\n", listRecords.size());
        if (listRecords.size()) {   
            for (auto it : listRecords) {
                printf("%s, [%s - %s]\n", 
                                it.fileName.c_str(), 
                                it.beginTime.c_str(), 
                                it.endTime.c_str());
            }
        }

        listRecords = SDRecords.getAllPlaylists(qDay, Recorder::eOption::Motion);
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

    SDCard::EXIT_ATOMIC(SDRecords);
#endif

#if 0 /* Test QUERY brith timestamp */
    std::cout << std::endl;
    std::string QFolder = "/home/hungqp/PC/SDCard-Recorder/records/video/2024.01.25";
    std::string QFile1 = "bbb.h264";
    std::string QFile2 = "aaa.h264";

    std::string qDay25 = pathToRecords + "/video/" + "2024.01.25";
    std::string qDay28 = pathToRecords + "/video/" + "2024.01.28";

    uint32_t birthOfQDay25 = getBirthTimestamp(qDay25.c_str());
    uint32_t birthOfQDay28 = getBirthTimestamp(qDay28.c_str());

    printf("[2024.01.25] %d\t%s\n", birthOfQDay25, ctime((time_t*)(time_t)&birthOfQDay25));
    printf("[2024.01.28] %d\t%s\n", birthOfQDay28, ctime((time_t*)(time_t)&birthOfQDay28));

    if (birthOfQDay25 < birthOfQDay28) {
        std::cout << "2024.01.25 must be DELETED" << std::endl;
    }
    else {
        std::cout << "2024.01.28 must be DELETED" << std::endl;
    }
#endif

    return 0;
}

/*
    if (SDCard::isSDCardMounted(SDRecords)) {
        printf("Total capacity :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
                        (uint32_t)SDRecords.totalCapacity,
                        (uint32_t)bytesToKiloBytes(SDRecords.totalCapacity),
                        (uint32_t)bytesToMegaBytes(SDRecords.totalCapacity),
                        (uint32_t)bytesToGigaBytes(SDRecords.totalCapacity));
        
        printf("Used capacity  :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
                        (uint32_t)SDRecords.usedCapacity,
                        (uint32_t)bytesToKiloBytes(SDRecords.usedCapacity),
                        (uint32_t)bytesToMegaBytes(SDRecords.usedCapacity),
                        (uint32_t)bytesToGigaBytes(SDRecords.usedCapacity));

        printf("Free capacity  :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
                        (uint32_t)SDRecords.freeCapacity,
                        (uint32_t)bytesToKiloBytes(SDRecords.freeCapacity),
                        (uint32_t)bytesToMegaBytes(SDRecords.freeCapacity),
                        (uint32_t)bytesToGigaBytes(SDRecords.freeCapacity));
    }
    else {
        std::cout << "SD Card hasn't mounted" << std::endl;
    }
*/

void signalHandler(int signal) {
    (void)signal;

    SDCard::ENTRY_ATOMIC(SDRecords);
    {
        if (SDRecords.currentSession.empty() == false) {
            SDRecords.videoRecorder->getStop();
            SDRecords.audioRecorder->getStop();
            SDCard::closeSessionFullRec(SDRecords);
        }
        
        std::string today = getTodayDateString();
        auto listRecords = SDRecords.getAllPlaylists(today, Recorder::eOption::Full);

        printf("Total full records : %ld\n", listRecords.size());
        if (listRecords.size()) {   
            for (auto it : listRecords) {
                printf("%s, [%s - %s]\n", 
                                    it.fileName.c_str(), 
                                it.beginTime.c_str(), 
                                it.endTime.c_str());
            }
        }

        listRecords = SDRecords.getAllPlaylists(today, Recorder::eOption::Motion);
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

    SDCard::EXIT_ATOMIC(SDRecords);

    std::cout << std::endl;
    std::cout << "Application exit !!!" << std::endl;

    std::exit(EXIT_SUCCESS);
}

void setupBeforeOpenSession() {
    SDCard::ENTRY_ATOMIC(SDRecords);
    {
        bool isMounted = false;

    #if 0 /* Query SD Card has mounted or unmounted */
        isMounted = SDCard::isSDCardMounted(SDRecords);
    #else
        isMounted = true; /* TODO: Assign for testing */
        SDRecords.eStateSD = SDCard::eState::Mounted;
    #endif

        if (!isMounted) {
            return;
        }

        if (SDRecords.currentSession.empty()) {
            SDCard::openSessionFullRec(SDRecords);
            std::cout << "[START] Record Session " << SDRecords.currentSession << std::endl;
        }
    }
    SDCard::EXIT_ATOMIC(SDRecords);
}

void* pollingDateTime(void *arg) {
    (void)arg;

    while (1) {
        std::cout << "[QUERY] Datetime Session Record" << std::endl;

        if (getTodayDateString() != SDRecords.currentSession) {
            SDCard::closeSessionFullRec(SDRecords);
            setupBeforeOpenSession();
        }

        sleep(60);
    }

    return NULL;
}

void* storageH264Samples(void *arg) {
    (void)arg;

    while (1) {
        SDCard::ENTRY_ATOMIC(SDRecords);
        {
        #if 0
            bool isMounted = SDCard::isSDCardMounted(SDRecords);
        #else
            bool isMounted = true;
        #endif
            if (isMounted && SDRecords.currentSession.empty() == false) {
                samplesH264 += 1;

                {
                    /* 
                        TODO: 
                        Code segment check capacity of SD Card (Clear oldest directory or file oldest)
                    */
                }

                int ret = SDCard::storageSamples(SDRecords.videoRecorder, (uint8_t*)&samplesH264, sizeof(samplesH264));
                if (ret == SDCARD_RETURN_SUCCESS) {
                    std::cout << "[STORAGE] " << SDRecords.videoRecorder->getCurrentInstance() << std::endl;
                }
            }
        }
        SDCard::EXIT_ATOMIC(SDRecords);

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
        SDCard::ENTRY_ATOMIC(SDRecords);
        {
            

        #if 0
            bool isMounted = SDCard::isSDCardMounted(SDRecords);
        #else
            bool isMounted = true;
        #endif
    
            if (isMounted && SDRecords.currentSession.empty() == false) {
                updateEndTimestamp();

                samplesG711 += 2;
                int ret = SDCard::storageSamples(SDRecords.audioRecorder, (uint8_t*)&samplesG711, sizeof(samplesG711));
                if (ret == SDCARD_RETURN_SUCCESS) {
                    std::cout << "[STORAGE] " << SDRecords.audioRecorder->getCurrentInstance() << std::endl;
                }
            }
        }
        SDCard::EXIT_ATOMIC(SDRecords);

        sleep(1);
    }

    return NULL;
}