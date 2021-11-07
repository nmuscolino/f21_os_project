#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "report_record_formats.h"
#include "queue_ids.h"
#include <signal.h>
#include <pthreads.h>

#ifndef darwin
size_t                  /* O - Length of string */
strlcpy(char       *dst,          /* O - Destination string */
	const char *src,	/* I - Source string */
	size_t      size)	/* I - Size of destination string buffer */
{
    size_t    srclen;		/* Length of source string */


    /*
     * Figure out how much room is needed...
     */

    size --;

    srclen = strlen(src);

    /*
     * Copy the appropriate amount...
     */

     if (srclen > size)
	 srclen = size;

     memcpy(dst, src, srclen);
     dst[srclen] = '\0';

     return (srclen);
}

#endif

//Struct to store the number of records sent for each report
typedef struct status {
     int reportIndex;
     int recordsSent;
} status_t;

//Store total number of requests, total records read from stdin, and statuses of records
int requestCount = 0;
int recordsRead = 0;
status_t* statusArray;

pthread_mutex_t mutex;

//Prints status report to stdout on Ctrl+C
void sigintHandler(int sig_num) {
     printf("***Report***\n");
     for(int i = 0; i < requestCount; i++) {
	 printf("Records sent for report index %d: %d\n", statusArray[i].reportIndex, statusArray[i].recordsSent);
     } 
}
     
//Send report record back to ReportGenerator
void sendRecord(record, ) {
     int msqid;
     int msgflg = IPC_CREAT | 0666;
     key_t key;
     report_record_buf sbuf;
     size_t buf_length;

     key = ftok(FILE_IN_HOME_DIRECTORY, QUEUE_NUMBER);
     if (key == 0xffffffff) {
	 fprintf(stderr, "Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
	 return 1;
     }

     if ((msqid = msgget(key, msgflg)) < 0) {
	 int errnum = errno;
	 fprintf(stderr, "Value of errno: %d\n", errno);
	 perror("(msgget)");
	 fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
     }
     else
	 fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

     // We'll send message type 2
     sbuf.mtype = 2;
     strlcpy(sbuf.record, record, RECORD_FIELD_LENGTH); //FIXME: Double check parameters here
     buf_length = strlen(sbuf.record) + sizeof(int)+1;//struct size without
     // Send a message
     if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
	 int errnum = errno;
	 fprintf(stderr, "%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
	 perror("(msgsnd)");
	 fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
	 exit(1);
     }
     else
	 fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);
}

//Receive report request from ReportGenerator
report_request_buf receiveRequest() {
     int msqid;
     int msgflg = IPC_CREAT | 0666;
     key_t key;
     report_request_buf rbuf;
     size_t buf_length;

     key = ftok(FILE_IN_HOME_DIRECTORY, QUEUE_NUMBER);
     if (key == 0xffffffff) {
	 fprintf(stderr, "Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
	 return 1;
     }

     if((msqid = msgget(key, msgflg)) < 0) {
	 int errnum = errno;
	 fprintf(stderr, "Value of errno: %d\n", errno);
	 perror("(msgget)");
	 fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
     }
     else
	 fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

     // msgrcv to receive message
     int ret;
     do {
	 ret = msgrcv(msqid, &rbuf, sizeof(rbuf), 1, 0);//receive type 1 message

	 int errnum = errno;
	 if (ret < 0 && errno !=EINTR){
	   fprintf(stderr, "Value of errno: %d\n", errno);
	   perror("Error printed by perror");
	   fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
	 }
     } while ((ret < 0 ) && (errno == 4));
     //fprintf(stderr, "msgrcv error return code --%d:$d--",ret,errno);

     return rbuf;
}

int main() {
     
     //Initialize mutex to protect shared variables
     pthread_mutex_init(&mutex);

     //Declare singal handler
     signal(SIGINT, sigintHandler);

     report_request_buf request;

     //Receive first request to obtain total number of report requests
     //Store the requests in an array
     request = receiveRequest();
     requestCount = request.report_count;
     report_request_buf requestArray[requestCount];
     //Report indexes start with 1, so must subtract 1 for 0 indexed array
     //Reports will be sorted by index in the array
     requestArray[request.report_idx - 1] = request;

     //Now that the number of requests is known, the status array can be initialized
     //Set the values of the status array corresponding to this report accordingly
     statusArray = (status_t*) malloc(sizeof(status_t) * requestCount);
     pthread_mutex_lock(&mutex);
     statusArray[0].reportIndex = request.report_idx;
     statusArray[0].recordsSent = 0;
     pthread_mutex_unlock(&mutex);

     //Receive all remaining requests and store in the array
     //Start with i = 1 to account for previously received request
     for(int i = 1; i < requestCount; i++) {
	 request = receiveRequest();
	 requestArray[request.report_idx - 1] = request;
	 
	 pthread_mutex_lock(&mutex);
	 statusArray[i].reportIndex = request.report_idx;
	 statusArray[i].recordsSent = 0;
	 pthread_mutex_unlock(&mutex);
     }

     //Read records from stdin and compare to search strings of each request
     char* record;
     while(fgets(record, RECORD_MAX_LENGTH, stdin) != NULL) {
	 for(int i = 0; i < requestCount; i++) {
	      //If search string is contained in the report, send the report back
	      //to ReportGenerator
	      if(strstr(record, requestArray[i].search_string) != NULL) {
		   sendRecord(record, requestArray[i].report_idx);
	      }
	      pthread_mutex_lock(&mutex);
	      recordsRead++;
	      pthread_mutex_unlock(&mutex);
	      if (recordsRead == 10) sleep(5); //FIXME: Main thread will probably wake up on sigint
	 }
     }

     //All records have been processed, send a record of length 0 to ReportGenerator
     sendRecord("", 0);
     return 0;
}

