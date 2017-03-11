/** 
*   @file wiring.c
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


extern int cnt;             /**< Log counter */
extern GMutex mutex_cnt;    /**< Mutex for counter */
extern GMutex mutex_socket; /**< Mutex for socket */
extern zlog_category_t *c;  /**< category for zlog */
extern void *context;       /**< zmq context */

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

    zlog_info(c, "Current 0MQ version is %d.%d.%d\n", major, minor, patch);
    context = zmq_ctx_new();      //zmq 3.3
    // context = zmq_init(1);        //zmq 2.2
    if(!context)
    {
        zlog_fatal(c,"Failed to creates a new ØMQ context");
        exit(-1);
    }
    void *sender = zmq_socket(context, ZMQ_PUSH);
    if(!sender)
    {
        zlog_fatal(c, "Error: %s",strerror(errno));
        exit(-1);
    }
    assert (sender);

    int rc = zmq_connect (sender, "tcp://localhost:5502");
    assert (rc == 0);

//  Wait for the worker to connect so that when we send a message
//  with routing envelope, it will actually match the worker.
//    sleep (1);

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
void send_event_with_mid(void *sender, json_t *event, const char *normalizer, const char *repo){
    /* sends event to upper layer using sender socket*/
    start_benchmarker_processing();
#ifdef BENCHMARK
        g_mutex_lock (&mutex_cnt);
        cnt += 1;
        if(cnt % 1000 == 0)
        {
            struct timeval end_time;
            gettimeofday(&end_time, NULL);
            printf("benchmark data: count=%u time= ", cnt);
            timeval_print(&end_time);
        }
        g_mutex_unlock(&mutex_cnt);
#endif
    char *event_st;
    event_st = json_dumps(event, JSON_COMPACT);
    if(event_st == NULL){
        zlog_error(c,"Unable to convert json to string");
        return;
    }
    int str_len = strlen(normalizer) + strlen(repo) + strlen(event_st) + 3;

    char *event_with_mid = (char *) malloc(str_len);
    memset(event_with_mid, 0, str_len);
    sprintf(event_with_mid, "%s%s%s%s%s", normalizer, "\n", repo, "\n", event_st );
    event_with_mid[str_len-1]='\0';
#ifndef BENCHMARK
#ifndef PROFILE
    //zmq 3.3
    g_mutex_lock(&mutex_socket);
    // int rc = zmq_send(sender, event_with_mid, strlen(event_with_mid), 0);
    int rc = benchmarker_zmq_send(sender, event_with_mid);
    g_mutex_unlock(&mutex_socket);
    if(rc == -1)
    {
        zlog_error(c,"Error: zmq_send: %d : %s", errno, zmq_strerror(errno));
    }
#endif
#endif
    //zmq 2.2
    /* Create a new message, allocating some bytes for message content */
    // zmq_msg_t msg;
    // int rc = zmq_msg_init_size (&msg, strlen(event_with_mid));
    // assert (rc == 0);
    
    // memcpy (zmq_msg_data (&msg), event_with_mid, strlen(event_with_mid));
    
    // #ifndef BENCHMARK
    //      rc = zmq_send (sender, &msg, 0);
    // #endif
    free(event_st);
    free(event_with_mid);
}

/**
*   @brief  Closes the zmq socket and destroys the context
*   @param[in]  context zmq_context
*   @param[in]  socket  zmq_socket 
*/
void free_zmq(void *context, void *socket)
{
    zmq_close(socket);
    zmq_ctx_destroy(context);
}
