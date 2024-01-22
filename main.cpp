#include <bits/stdc++.h>
#include <unistd.h>
#include <pthread.h>

#include "SDCard.hpp"
#include "recorder.hpp"
#include "utilities.hpp"

static SDCard SDCARD("/dev/sdb1", "/home/hungqp/PC/SDCardRecords/records");

static uint8_t samplesH264 = 0;
static const uint8_t sizeOfSamplesH264 = sizeof(samplesH264);

static uint8_t samplesG711 = 0;
static const uint8_t sizeOfSamplesG711 = sizeof(samplesG711);

static pthread_t threadPollingDateTimeId;
static pthread_t threadStorageH264SamplesId;
static pthread_t threadStorageG711SamplesId;

static void signalHandler(int signal) {
    (void)signal;

    SDCard::ENTRY_ATOMIC(SDCARD);
    {
        if (SDCARD.currentSession.empty() == false) {
            SDCARD.videoRecord->getStop();
            SDCARD.audioRecord->getStop();
            SDCard::closeSessionRec(SDCARD);
        }
        
        std::string today = getTodayDateString();
        auto listRecords = SDCARD.getAllPlaylists(today);

        std::cout << "Total records " << listRecords.size() << std::endl;

        if (listRecords.size()) {   
            for (auto it : listRecords) {
                printf("%s, [%s - %s]\n", 
                        it.fileName.c_str(), it.beginTime.c_str(), it.endTime.c_str());
            }
        }
    }

    SDCard::EXIT_ATOMIC(SDCARD);

    std::cout << std::endl;
    std::cout << "Application EXIT !!!" << std::endl;

    std::exit(EXIT_SUCCESS);
}

static void setupBeforeOpenSession();
static void* pollingDateTime(void *arg);
static void* storageH264Samples(void *arg);
static void* storageG711Samples(void *arg);


int main() {
    std::signal(SIGINT, signalHandler);

    setupBeforeOpenSession();

    if (!SDCARD.currentSession.empty()) {
        pthread_create(&threadPollingDateTimeId, NULL, pollingDateTime, NULL);
        pthread_create(&threadStorageH264SamplesId, NULL, storageH264Samples, NULL);
        pthread_create(&threadStorageG711SamplesId, NULL, storageG711Samples, NULL);

        pthread_join(threadPollingDateTimeId, NULL);
        pthread_join(threadStorageH264SamplesId, NULL);
        pthread_join(threadStorageG711SamplesId, NULL);
    }
    else {
        std::cout << "[OPEN] Session Record Opening Failed" << std::endl;
    }

    return 0;
}

// if (SDCard::isSDCardMounted(SDCARD)) {
//     printf("Total capacity :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
//                     (uint32_t)SDCARD.totalCapacity,
//                     (uint32_t)bytesToKiloBytes(SDCARD.totalCapacity),
//                     (uint32_t)bytesToMegaBytes(SDCARD.totalCapacity),
//                     (uint32_t)bytesToGigaBytes(SDCARD.totalCapacity));
    
//     printf("Used capacity  :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
//                     (uint32_t)SDCARD.usedCapacity,
//                     (uint32_t)bytesToKiloBytes(SDCARD.usedCapacity),
//                     (uint32_t)bytesToMegaBytes(SDCARD.usedCapacity),
//                     (uint32_t)bytesToGigaBytes(SDCARD.usedCapacity));

//     printf("Free capacity  :\t%d B\t\t%d KB\t\t%d MB\t%d GB\n", 
//                     (uint32_t)SDCARD.freeCapacity,
//                     (uint32_t)bytesToKiloBytes(SDCARD.freeCapacity),
//                     (uint32_t)bytesToMegaBytes(SDCARD.freeCapacity),
//                     (uint32_t)bytesToGigaBytes(SDCARD.freeCapacity));
// }
// else {
//     std::cout << "SD Card hasn't mounted" << std::endl;
// }

void setupBeforeOpenSession() {
    SDCard::ENTRY_ATOMIC(SDCARD);
    {
        bool isMounted = false;

    #if 0 /* Query SD Card has mounted or unmounted */
        isMounted = SDCard::isSDCardMounted(SDCARD);
    #else
        isMounted = true; /* TODO: Assign for testing */
    #endif

        if (!isMounted) {
            return;
        }

        if (SDCARD.currentSession.empty()) {
            SDCard::openSessionRec(SDCARD);
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
            SDCard::closeSessionRec(SDCARD);
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
            samplesH264 += 1;
            int ret = SDCard::collectSamples(SDCARD.videoRecord, (uint8_t*)&samplesH264, sizeOfSamplesH264);
            if (ret == SDCARD_RETURN_SUCCESS) {
                std::cout << "[COLLECT] Samples video " << unsigned(samplesH264) << " has collected" << std::endl;
            }
            else {
                std::cout << "[COLLECT] Samples video " << unsigned(samplesH264) << " hasn't collected" << std::endl;
            }
        }
        SDCard::EXIT_ATOMIC(SDCARD);

        sleep(1);
    }

    return NULL;
}

void* storageG711Samples(void *arg) {
    (void)arg;

    while (1) {
        SDCard::ENTRY_ATOMIC(SDCARD);
        {
            samplesG711 += 2;
            int ret = SDCard::collectSamples(SDCARD.audioRecord, (uint8_t*)&samplesG711, sizeOfSamplesG711);
            if (ret == SDCARD_RETURN_SUCCESS) {
                std::cout << "[COLLECT] Samples audio " << unsigned(samplesG711) << " has collected" << std::endl;
            }
            else {
                std::cout << "[COLLECT] Samples audio " << unsigned(samplesG711) << " hasn't collected" << std::endl;
            }
        }
        SDCard::EXIT_ATOMIC(SDCARD);

        sleep(1);
    }

    return NULL;
}