#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>

#include <pthread.h>
#include <sys/time.h>
#include<iostream>
using namespace std;
int time_up;
FILE *log_file;
char rq[25000][1000];
// user info struct
struct user_info {
  // user id
  int id;

  // socket info
  int portno;
  char *hostname;
  float think_time;

  // user metrics
  int total_count;
  float total_rtt;
};

// error handling function
void error(char *msg) {
  perror(msg);
  //exit(0);
}

// time diff in seconds
float time_diff(struct timeval *t2, struct timeval *t1) {
  return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}

// user thread function
void *user_function(void *arg) { 
  /* get user info */
  struct user_info *info = (struct user_info *)arg;
  int sockfd, n,portno,x=0;
  char buffer[1000];
  struct timeval start, end;
  char hello[6]="hello";
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int y=(*info).id;
  string emsg;
  int l=emsg.length();
  char emsg1[l+1];
  while (1) {
    /* start timer */
    gettimeofday(&start, NULL);

    /* TODO: create socket */
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
     emsg="ERROR opening socket";
     int l=emsg.length();
     char emsg1[l+1];
     strcpy(emsg1,emsg.c_str());
     error(emsg1);
     continue;
    }
    /* TODO: set server attrs */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = (*info).portno;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    /* TODO: connect to server */
    n=connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(n<0)
    {
     emsg="ERROR on binding";
     int l=emsg.length();
     char emsg1[l+1];
     strcpy(emsg1,emsg.c_str());
     error(emsg1);
     close(sockfd);
     continue;
    }
    /* TODO: send message to server */
    n=write(sockfd,rq[y],strlen(rq[y]));
    if(n<=0)
    {
     emsg="ERROR writing to socket";
     int l=emsg.length();
     char emsg1[l+1];
     strcpy(emsg1,emsg.c_str());
     error(emsg1);
     close(sockfd);
     continue;
    }
    /* TODO: read reply from server */
    n=read(sockfd,buffer,1000);
    if(n<=0)
    {
     emsg="ERROR reading from socket";
     int l=emsg.length();
     char emsg1[l+1];
     strcpy(emsg1,emsg.c_str());
     error(emsg1);
     continue;
    }
    //printf("client number %d %s\n",(*info).id,buffer);
    printf("client number %d\n",(*info).id);
    /* TODO: close socket */
    close(sockfd);
    /* end timer */
    gettimeofday(&end, NULL);

    /* if time up, break */
    if (time_up)
      break;

    /* TODO: update user metrics */
    x=x+1;
    (*info).total_count=x;
    (*info).total_rtt+=time_diff(&end,&start);
    /* TODO: sleep for think time */
    usleep(((*info).think_time)*1000000);
  }

  /* exit thread */
  fprintf(log_file, "User #%d finished\n", info->id);
  fflush(log_file);
  pthread_exit(NULL);
  //close(sockfd);
}

int main(int argc, char *argv[]) {
  int user_count,portno, test_duration;
  float think_time;
  char *hostname;

  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
    exit(0);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);
  user_count=atoi(argv[3]);
  think_time = atof(argv[4]);
  test_duration = atoi(argv[5]);

  printf("Hostname: %s\n", hostname);
  printf("Port: %d\n", portno);
  printf("User Count: %d\n", user_count);
  printf("Think Time: %f s\n", think_time);
  printf("Test Duration: %d s\n", test_duration);
  for(int i=0;i<user_count;i=i+3)
  {
   string a="GET /index.html HTTP/1.1";
   strcpy(rq[i],a.c_str());
  }
  for(int i=1;i<user_count;i=i+3)
  {
   string a="GET /apart1/index.html HTTP/1.1";
   strcpy(rq[i],a.c_str());
  }
  for(int i=2;i<user_count;i=i+3)
  {
   string a="GET /apart2/index.html HTTP/1.1";
   strcpy(rq[i],a.c_str());
  }
  /* open log file */
  log_file = fopen("load_gen.log", "w");

  pthread_t threads[user_count];
  struct user_info info[user_count];
  struct timeval start, end;

  /* start timer */
  gettimeofday(&start, NULL);
  time_up = 0;
  for (int i = 0; i < user_count; ++i) {
    /* TODO: initialize user info */
    info[i].id=i;
    info[i].portno=portno;
    info[i].hostname=hostname;
    info[i].think_time=think_time;
    info[i].total_count=0;
    info[i].total_rtt=0;
    /* TODO: create user thread */
    pthread_create(&threads[i],NULL,user_function,(void *)&info[i]);
    fprintf(log_file, "Created thread %d\n", i);
  }

  /* TODO: wait for test duration */
  sleep(test_duration);

  fprintf(log_file, "Woke up\n");

  /* end timer */
  time_up = 1;
  gettimeofday(&end, NULL);

  /* TODO: wait for all threads to finish */
  for(int i=0;i<user_count;i++)
  {
    pthread_join(threads[i],NULL);
    printf("worker %d joined\n",i);
  }

  /* TODO: print results */
  int sum=0;
  float sum1=0.0;
  for(int i=0;i<user_count;i++)
  {
   sum=sum+info->total_count;
   sum1=sum1+info->total_rtt;
  }
  int throughput=sum/test_duration;
  cout<<throughput<<endl;
  float response_time=(sum1*1000)/(float)sum;
  cout<<response_time<<endl;
  /* close log file */
  fclose(log_file);

  return 0;
}

