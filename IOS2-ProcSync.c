/* 
 * File:   main.c
 * Author: jan
 *
 * Created on April 8, 2013, 5:05 PM
 */
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#define EXIT_SYSERROR 2

typedef struct {
    uint logIndex;
    uint elfsActive;
    FILE* outFp;
    int sharedMemFd;
    sem_t elfsWaitHelpSem;
    sem_t elfsAskHelpSem;
    sem_t elfsEndSem;
    sem_t santaHelpedSem;
} TSharedMem;

int elfMain(TSharedMem *sharedMem, sem_t *sharedMemMutex, uint elfId, uint elfShifts, uint elfWorkMax ) {
    struct timespec elfSleep = {
        .tv_sec = 0,
        .tv_nsec = 0
    };
    uint elfSleepMili = 0;
    srand(time(NULL) + elfId);
    // elf starting
    sem_wait(sharedMemMutex);
    fprintf(sharedMem->outFp, "%u: elf: %i: started\n", (sharedMem->logIndex)++, elfId);
    sem_post(sharedMemMutex);
    while (elfShifts > 0) {
        // simulate work
        // not uniform pseudo random, but who cares
        elfSleepMili = (rand() % elfWorkMax);
        elfSleep.tv_sec = elfSleepMili / 1000;
        elfSleep.tv_nsec = (elfSleepMili % 1000) * 1000000;
        nanosleep(&elfSleep, NULL);
        sem_wait(sharedMemMutex);
        fprintf(sharedMem->outFp, "%u: elf: %i: needed help\n", (sharedMem->logIndex)++, elfId);
        sem_post(sharedMemMutex);
        // check if we can ask for help
        sem_wait(&sharedMem->elfsWaitHelpSem);
        // ask for help
        sem_wait(sharedMemMutex);
        fprintf(sharedMem->outFp, "%u: elf: %i: asked for help\n", (sharedMem->logIndex)++, elfId);
        sem_post(&sharedMem->elfsAskHelpSem);
        sem_post(sharedMemMutex);
        // wait for santa to finish
        sem_wait(&sharedMem->santaHelpedSem);
        sem_wait(sharedMemMutex);
        fprintf(sharedMem->outFp, "%u: elf: %i: got help\n", (sharedMem->logIndex)++, elfId);
        // tell santa we got help
        sem_post(&sharedMem->elfsAskHelpSem);
        sem_post(sharedMemMutex);
        elfShifts--;
    }
    sem_wait(sharedMemMutex);
    fprintf(sharedMem->outFp, "%u: elf: %i: got a vacation\n", (sharedMem->logIndex)++, elfId);
    (sharedMem->elfsActive)--;
    // Am I the last one? If so tell santa
    if (sharedMem->elfsActive == 0) {
        sem_post(&sharedMem->elfsAskHelpSem);
    }
    sem_post(sharedMemMutex);
    sem_wait(&sharedMem->elfsEndSem);
    // wait for all elfs to get vacation
    sem_wait(sharedMemMutex);
    fprintf(sharedMem->outFp, "%u: elf: %i: finished\n", (sharedMem->logIndex)++, elfId);
    sem_post(sharedMemMutex);
    
    // Flush the toilet
    fclose(sharedMem->outFp);
    close(sharedMem->sharedMemFd);
    sem_close(sharedMemMutex);
    
    exit(EXIT_SUCCESS);
}


#define ELFS_ACTIVE_HIGH 3
#define ELFS_ACTIVE_HIGH_HELP 3
int santaMain(TSharedMem *sharedMem, sem_t *sharedMemMutex, uint santaWorkMax) {
    struct timespec santaSleep = {
        .tv_sec = 0,
        .tv_nsec = 0
    };
    uint elfsTotal = 0;
    uint elfsToHelp = 0;
    uint santaSleepMili = 0;
    sem_wait(sharedMemMutex);
    fprintf(sharedMem->outFp, "%u: santa: started\n", (sharedMem->logIndex)++);
    elfsTotal = sharedMem->elfsActive;
    sem_post(sharedMemMutex);
    while (1) {
        // elfs can ask
        sem_wait(sharedMemMutex);
        if (sharedMem->elfsActive >= ELFS_ACTIVE_HIGH) {
            elfsToHelp = ELFS_ACTIVE_HIGH_HELP;
        } else {
            elfsToHelp = 1;
        }
        sem_post(sharedMemMutex);
        for (uint j = 0; j < elfsToHelp; j++) sem_post(&sharedMem->elfsWaitHelpSem);
        // wait for them to ask
        for (uint j = 0; j < elfsToHelp; j++) sem_wait(&sharedMem->elfsAskHelpSem);
        sem_wait(sharedMemMutex);
        fprintf(sharedMem->outFp, "%u: santa: checked state: %u: %u\n", (sharedMem->logIndex)++, sharedMem->elfsActive, elfsToHelp);
        if (sharedMem->elfsActive == 0) {
            break;
        }
        fprintf(sharedMem->outFp, "%u: santa: can help\n", (sharedMem->logIndex)++);
        sem_post(sharedMemMutex);
        // simulate work
        santaSleepMili = (rand() % santaWorkMax);
        santaSleep.tv_sec = santaSleepMili / 1000;
        santaSleep.tv_nsec = (santaSleepMili % 1000) * 1000000;
        nanosleep(&santaSleep, NULL);
        // we are done
        for (uint j = 0; j < elfsToHelp; j++) sem_post(&sharedMem->santaHelpedSem);
        // wait for them to tell us they finished
        for (uint j = 0; j < elfsToHelp; j++) sem_wait(&sharedMem->elfsAskHelpSem);
    }
    fprintf(sharedMem->outFp, "%u: santa: finished\n", (sharedMem->logIndex)++);
    // Set them free!
    for (uint j = 0; j < elfsTotal; j++) sem_post(&sharedMem->elfsEndSem);
    sem_post(sharedMemMutex);
    
    // Clean up a little bit
    fclose(sharedMem->outFp);
    close(sharedMem->sharedMemFd);
    sem_close(sharedMemMutex);
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    int retCode = EXIT_SUCCESS;
    long int elfsShifts = 0;
    long int elfsCount = 0;
    long int elfsWorkMax = 0;
    long int santaWorkMax = 0;
    
    // Basic argument handling
    if (argc < 5) {
        fprintf(stderr, "Not enough arguments. Usage ./santa C E H S\n");
        retCode = EXIT_FAILURE;
        goto mainReturn;
    }
    errno = 0;
    elfsShifts = strtol(argv[1],NULL,10);
    elfsCount = strtol(argv[2],NULL,10);
    elfsWorkMax = strtol(argv[3],NULL,10);
    santaWorkMax = strtol(argv[4],NULL,10);
    if ((elfsShifts < 1) || ((uint)elfsShifts > UINT_MAX) ||
        (elfsCount < 1) || ((uint)elfsCount > UINT_MAX) ||
        (elfsWorkMax < 0) || ((uint)elfsWorkMax > UINT_MAX) ||
        (santaWorkMax < 0) || ((uint)santaWorkMax > UINT_MAX))
        errno = EINVAL;
    if ( errno != 0 ) {
        fprintf(stderr, "Incorrect arguments. Usage ./santa C E H S\n");
        retCode = EXIT_FAILURE;
        goto mainReturn;
    }
    
    // Create mutex
    sem_t *sharedMemMutex = sem_open("/xspisi03_sharedMemMutex", O_CREAT | O_EXCL, 0666, 1);
    if (sharedMemMutex == SEM_FAILED) {
        fprintf(stderr,"System error sem_open(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
        goto mainReturn;
    }
    // Create shared memory
    int sharedMemFd = shm_open("/xspisi03_sharedMem", O_CREAT | O_RDWR | O_EXCL, 0666);
    if (sharedMemFd == -1) {
        fprintf(stderr,"System error shm_open(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
        goto mainSharedMemMutexCleanUp;
    }
    // Allocate some space
    if (ftruncate(sharedMemFd, sizeof(TSharedMem)) == -1) {
        fprintf(stderr,"System error ftruncate(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
        goto mainSharedMemFdCleanUp;
    }
    // Map it!
    TSharedMem *sharedMem = mmap(NULL, sizeof(TSharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemFd, 0);
    if (sharedMem == MAP_FAILED) {
        fprintf(stderr,"System error mmap(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
        goto mainSharedMemFdCleanUp;
    }
    
    // Init
    sharedMem->sharedMemFd = sharedMemFd;
    sharedMem->logIndex = 1;
    sharedMem->elfsActive = elfsCount;
    // Our output
    sharedMem->outFp = fopen("santa.out", "w");
    if (sharedMem->outFp == NULL) {
        fprintf(stderr,"System error fopen(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
        goto mainSharedMemCleanUp;
    }
    setvbuf(sharedMem->outFp, (char *) NULL, _IOLBF, 0);
    
    // no need for error checking
    sem_init(&sharedMem->elfsWaitHelpSem, 1, 0);
    sem_init(&sharedMem->elfsAskHelpSem, 1, 0);
    sem_init(&sharedMem->elfsEndSem, 1, 0);
    sem_init(&sharedMem->santaHelpedSem, 1, 0);
    
    // Spawn those little basta.. elfs
    int forkRet = 0;
    int spawnedProc = 1;
    for (spawnedProc = 1; spawnedProc <= elfsCount; spawnedProc++) {
        if ((forkRet = fork()) == 0) {
            elfMain(sharedMem, sharedMemMutex, spawnedProc, elfsShifts, elfsWorkMax);
        } else if (forkRet == -1) {
            spawnedProc--;
            fprintf(stderr,"System error fork(): %i %s\n",errno,strerror(errno));
            retCode = EXIT_SYSERROR;
            break;
        }
    }
    
    // can we spawn santa, if so do so
    if (spawnedProc == elfsCount + 1) {
        if ((forkRet = fork()) == 0) {
            santaMain(sharedMem, sharedMemMutex, santaWorkMax);
            spawnedProc++;
        } else if (forkRet == -1) {
            fprintf(stderr,"System error fork(): %i %s\n",errno,strerror(errno));
            retCode = EXIT_SYSERROR;
            kill(0, SIGTERM);
        }
    } else {
        // kill 'em all
        kill(0, SIGTERM);
    }
    
    // pick up zombies
    for (int i = 1; i < spawnedProc; i++) {
        waitid(P_ALL, 0, NULL, WEXITED);
    }
    
    sem_destroy(&sharedMem->elfsWaitHelpSem);
    sem_destroy(&sharedMem->elfsAskHelpSem);
    sem_destroy(&sharedMem->elfsEndSem);
    sem_destroy(&sharedMem->santaHelpedSem);
    
    //mainSharedMemOutFdCleanUp:
    fclose(sharedMem->outFp);
    
    mainSharedMemCleanUp:
    munmap(sharedMem, sizeof(TSharedMem));
    
    mainSharedMemFdCleanUp:
    close(sharedMemFd);
    if (shm_unlink("/sharedMem") == -1) {
        fprintf(stderr,"System error shm_unlink(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
    }
    
    mainSharedMemMutexCleanUp:
    sem_close(sharedMemMutex);
    if (sem_unlink("/sharedMemMutex") == -1) {
        fprintf(stderr,"System error shm_unlink(): %i %s\n",errno,strerror(errno));
        retCode = EXIT_SYSERROR;
    }
    
    mainReturn:
    return retCode;
}

