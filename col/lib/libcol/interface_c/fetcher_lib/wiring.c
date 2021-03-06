/** 
*   @file wiring._c
*   @author ritesh, hari
*   @date 8/22/2013
*/
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <glib.h>

/* External Libraries */
#include <zmq.h>
#include <jansson.h>
#include <zlog.h>
#include "benchmarker.h"
#include "wiring.h"



/** @brief prints time value for benchmarking 
*/
void timeval_print(struct timeval *tv){
    printf("%ld.%06ld\n", (long int) tv->tv_sec, (long int) tv->tv_usec);
}

/**
*   @brief  Returns collector_out socket
*   @return void
*/
void *get_collector_out_socket(char *service_name){
    /* returns collector_out socket*/
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);

    zlog_info(_c, "Current 0MQ version is %d.%d.%d\n", major, minor, patch);
    _context = zmq_ctx_new();      //zmq 3.3
    // _context = zmq_init(1);        //zmq 2.2
    if(!_context)
    {
        zlog_fatal(_c,"Failed to creates a new ØMQ _context");
        exit(-1);
    }
    void *sender = zmq_socket(_context, ZMQ_PUSH);
    if(!sender)
    {
        zlog_fatal(_c, "Error: %s",strerror(errno));
        exit(-1);
    }
    assert (sender);

    int rc = zmq_connect (sender, "tcp://localhost:5502");
    assert (rc == 0);

    initialize_benchamrker(service_name); //for benchmark initialization
    return sender;
}

/**
*   @brief  It will send event containing message id to upper layer
*   @param[in]  sender Socket
*   @param[in]  event Event to be send 
*   @param[in]  normalizer Normalizer
*   @param[in]  repo Repo
*   @return void
*/
void send_event_with_mid(void *sender, json_t *event,const char *normalizer,const char *repo){
    /* sends event to upper layer using sender socket*/
    // start_benchmarker_processing();
    char *event_st;
    event_st = json_dumps(event, JSON_COMPACT);
    if(event_st == NULL){
        zlog_error(_c,"Unable to convert json to string");
        return;
    }
    int str_len = strlen(normalizer) + strlen(repo) + strlen(event_st) + 3;

    char *event_with_mid = (char *) malloc(str_len);
    memset(event_with_mid, 0, str_len);
    sprintf(event_with_mid, "%s%s%s%s%s", normalizer, "\n", repo, "\n", event_st );
    event_with_mid[str_len-1]='\0';
    zlog_debug(_c,"final event \n%s\n",event_with_mid);
#ifndef BENCHMARK
    //zmq 3.3
    pthread_mutex_lock(&_mutex_socket);
    // int rc = zmq_send(sender, event_with_mid, strlen(event_with_mid), 0);
    int rc = benchmarker_zmq_send(sender, event_with_mid);
    pthread_mutex_unlock(&_mutex_socket);
    if(rc == -1)
    {
        zlog_error(_c,"Error: zmq_send: %d : %s", errno, zmq_strerror(errno));
    }
#endif
    free(event_st);
    free(event_with_mid);
}

/**
*   @brief  Closes the zmq socket and destroys the _context
*   @param[in]  _context zmq_context
*   @param[in]  socket  zmq_socket 
*/
void free_zmq(void *_context, void *socket)
{
    zmq_close(socket);
    zmq_ctx_destroy(_context);
}
