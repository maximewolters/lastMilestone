//
// Created by maxime on 18/12/23.
//

#ifndef MILESTONE3_CONNMGR_H
#define MILESTONE3_CONNMGR_H
#include <pthread.h>
#include "sbuffer.h"
void *handleConnection(void *argument, pthread_mutex_t mutex);
int connmgrMain(int port, int max_conn, sbuffer_t *sbuffer, pthread_cond_t cond);
#endif //MILESTONE3_CONNMGR_H
