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
#include <pthread.h>
#include <unistd.h>

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
int printStatusFlag = 0;
status_t* statusArray;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condStatus = PTHREAD_COND_INITIALIZER;

//Prints status report to stdout on Ctrl+C
void sigintHandler(int sig_num) {
     printStatusFlag = 1;
     pthread_cond_signal(&condStatus); 
}
     
//Send report record back to ReportGenerator
void sendRecord(char* record, int index) {
     int msqid;
     int msgflg = IPC_CREAT | 0666;
     key_t key;
     report_record_buf sbuf;
     size_t buf_length;

     key = ftok(FILE_IN_HOME_DIR, index);
     if (key == 0xffffffff) {
	 fprintf(stderr, "Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
	 //return 1;
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
report_request_buf receiveRequest(int msqid) {
     //int msqid;
     int msgflg = IPC_CREAT | 0666;
     key_t key;
     report_request_buf rbuf;
     size_t buf_length;

     key = ftok(FILE_IN_HOME_DIR, QUEUE_NUMBER);
     if (key == 0xffffffff) {
	 fprintf(stderr, "Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
	 //return 1;
     }

     //if((msqid = msgget(key, msgflg)) < 0) {
	// int errnum = errno;
	// fprintf(stderr, "Value of errno: %d\n", errno);
	// perror("(msgget)");
	// fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
     //}
     //else
	// fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

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

//Status thread function. Prints status report upon Ctrl+C and end of program
void *printStatus(void *statusArray) {
     status_t* statuses = (status_t *) statusArray;
     while(1) {
	 pthread_mutex_lock(&mutex);
	 while(printStatusFlag == 0) pthread_cond_wait(&condStatus, &mutex);
	 printf("***Report***\n");
	 printf("%d records read for %d reports\n", recordsRead, requestCount);
	 for (int i = 0; i < requestCount; i++) {
	     printf("Records sent for report index %d: %d\n", statuses[i].reportIndex, statuses[i].recordsSent);
	 }
	 //Flag is set to 2 to signify end of program
	 //If triggered by Ctrl+C, flag will be 1
	 if (printStatusFlag == 2) break;
	 printStatusFlag = 0;
	 pthread_mutex_unlock(&mutex);
     }
}

int main() {

     //Declare singal handler
     signal(SIGINT, sigintHandler);

     pthread_t statusThread;

     report_request_buf request;

     int msqid;
     int msgflg = IPC_CREAT | 0666;
     key_t key;

     key = ftok(FILE_IN_HOME_DIR, QUEUE_NUMBER);
     if (key == 0xffffffff) {
	 fprintf(stderr, "Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
	 return 1;
     }

     //Create a queue to receive report requests
     fprintf(stderr, "Creating message queue\n");
     if((msqid = msgget(key, msgflg)) < 0) {
	 int errnum = errno;
	 fprintf(stderr, "Value of errno: %d\n", errno);
	 perror("(msgget)");
	 fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
     }
     else
	 fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

     //Receive first request to obtain total number of report requests
     //Store the requests in an array
     request = receiveRequest(msqid);
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
     fprintf(stderr, "Receiving remaining requests\n");
     for(int i = 1; i < requestCount; i++) {
	 request = receiveRequest(msqid);
	 requestArray[request.report_idx - 1] = request;
	 
	 pthread_mutex_lock(&mutex);
	 statusArray[i].reportIndex = request.report_idx;
	 statusArray[i].recordsSent = 0;
	 pthread_mutex_unlock(&mutex);
     }

     //Start thread after processing report requests
     pthread_create(&statusThread, NULL, printStatus, statusArray);

     //Read records from stdin and compare to search strings of each request
     fprintf(stderr, "Reading records from stdin\n");
     char* record = (char*)malloc(sizeof(char) * RECORD_MAX_LENGTH);
     while(fgets(record, RECORD_MAX_LENGTH, stdin) != NULL) {
	 for(int i = 0; i < requestCount; i++) {
	      //If search string is contained in the report, send the report back
	      //to ReportGenerator
	      //fprintf(stderr, "%s\n", record);
	      if(strstr(record, requestArray[i].search_string) != NULL) {
		   sendRecord(record, requestArray[i].report_idx);
		   pthread_mutex_lock(&mutex);
		   statusArray[i].recordsSent++;
		   pthread_mutex_unlock(&mutex);
	      }
	 }
	 pthread_mutex_lock(&mutex);
	 if (strcmp(record, "\n") != 0) recordsRead++;
	 //fprintf(stderr, "recordsRead is %d\n", recordsRead);
	 pthread_mutex_unlock(&mutex);
	
	 if (recordsRead == 10) sleep(5);
     }
     fprintf(stderr, "Finished reading records\n");
     //All records have been processed, send a record of length 0 to ReportGenerator
     record[0] = 0;
     for (int i = 0; i <  requestCount; i++) {
         sendRecord(record, requestArray[i].report_idx);
     }
     free(record);

     //End of program. Print status report and join statusThread
     printStatusFlag = 2;
     pthread_cond_signal(&condStatus);
     pthread_join(statusThread, NULL);
     return 0;
}

