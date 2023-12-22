//
// Created by maxime on 18/12/23.
//

#ifndef MILESTONE3_CONNMGR_H
#define MILESTONE3_CONNMGR_H
#include <pthread.h>
void *handleConnection(void *argument, pthread_mutex_t mutex);
int serverStartup(int argc, char *argv[]);
#endif //MILESTONE3_CONNMGR_H
