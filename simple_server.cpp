/* run using ./server <port> */
#include "http_server.hh"
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <queue>
using namespace std;
queue <int> fq;
int queue_max=20;
pthread_mutex_t mutex;
pthread_cond_t conde;
pthread_cond_t condf;
int *worker_thread_id=new int[15];
pthread_t *worker_thread=new pthread_t[15];
void error(char *msg) {
  perror(msg);
  //exit(1);
}
void siginthandler(int sig_num)
{
 delete [] worker_thread_id;
 delete [] worker_thread;
 exit(0);
}
void *workerthread(void *data)
{
 string emsg;
 int thread_id = *((int *)data);
 int val;
 while(1)
 {
  pthread_mutex_lock(&mutex);
  while(fq.empty())
  {
   pthread_cond_wait(&conde,&mutex);
  }
  int clientsocket=fq.front();
  fq.pop();
  pthread_cond_signal(&condf);
  pthread_mutex_unlock(&mutex);
  char buffer[1000]={};
  int n;
  bzero(buffer, 1000);
  n = read(clientsocket, buffer, 999);
  if (n <= 0)
  {
   emsg="ERROR reading from socket";
   int l=emsg.length();
   char emsg1[l+1];
   strcpy(emsg1,emsg.c_str());
   error(emsg1);
   continue;
   //close(clientsocket);
   //return 0;
  }
  cout<<"Here is the message "<<buffer<<endl;
  HTTP_Response *r = handle_request(buffer);
  string message=r->get_string();
  //cout<<message<<endl;
  printf("success\n");
  strcpy(buffer,message.c_str());
  n=write(clientsocket,buffer,strlen(buffer));
  if(n<=0)
  {
   emsg="ERROR writing to socket";
   int l=emsg.length();
   char emsg1[l+1];
   strcpy(emsg1,emsg.c_str());
   error(emsg1);
   close(clientsocket);
   return 0;
  }
  shutdown(clientsocket, SHUT_RDWR);
  close(clientsocket);
 }
 return 0;
}   

vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */
  this->method=first_line[0];
  this->url=first_line[1];
  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req) {
  HTTP_Request *request = new HTTP_Request(req);
  HTTP_Response *response = new HTTP_Response();
  string url = string("html_files")+request->url;
  response->HTTP_version = "1.0";
  time_t now=time(0);
  char* dt=ctime(&now);
  response->t=string("Date: ") + dt + string("IST");
  struct stat sb;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  { 
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";
    if (S_ISDIR(sb.st_mode)) {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      url = url+"/index.html";
      
    }

    /*
    TODO : open the file and read its contents
    */
     ifstream file(url);
     string b;
     if(file)
     {
      ostringstream o;
      o << file.rdbuf();
      b=o.str();
     }
     response->body=b;
     response->content_length=to_string(b.length());
     file.close();
    /*
    TODO : set the remaining fields of response appropriately
    */
  }

  else {
    response->status_code = "404";
    response->status_text = "Not Found";
    response->content_type = "text/html";
    string b="<!DOCTYPE html><html><head><title>error</title></head><body><h1>Page not found</h1></body></html>";
    response->body=b;
    response->content_length=to_string(b.length());
    /*
    TODO : set the remaining fields of response appropriately
    */
  }

  delete request;

  return response;
}
string HTTP_Response::get_string() {
  /*
  TODO : implement this function
  */
 string op="HTTP/"+this->HTTP_version+" "+this->status_code+" "+this->status_text+"\n"+this->t+"\n"+"Content_type: "+this->content_type+"\n"+"Content_length: "+this->content_length+"\n"+"\n"+this->body;
 return op;
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  string emsg;
  int l=emsg.length();
  char emsg1[l+1];
  pthread_mutex_init(&mutex,NULL);
  pthread_cond_init(&conde,NULL);
  pthread_cond_init(&condf,NULL);
  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  /* create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
   emsg="ERROR opening socket";
   int l=emsg.length();
   char emsg1[l+1];
   strcpy(emsg1,emsg.c_str());
   error(emsg1);
   exit(1);
  }

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    emsg="ERROR on binding";
    int l=emsg.length();
    char emsg1[l+1];
    strcpy(emsg1,emsg.c_str());
    error(emsg1);
    exit(1);
  }
  /* listen for incoming connection requests */
  //int *worker_thread_id;
  //pthread_t *worker_thread;
  //worker_thread_id=new int[15];
  //worker_thread=new pthread_t[15];
  int i;
  for(i=0;i<15;i++)
  {
   worker_thread_id[i]=i;
  }
  for(i=0;i<15;i++)
  {
   pthread_create(&worker_thread[i],NULL,workerthread,(void
   *)&worker_thread_id[i]);
  }
  listen(sockfd, 10000);
  clilen = sizeof(cli_addr);
  signal(SIGINT,siginthandler);
  while(1)
  {
  /* accept a new request, create a newsockfd */
   newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
   if (newsockfd < 0)
   {
    emsg="ERROR on accept";
    int l=emsg.length();
    char emsg1[l+1];
    strcpy(emsg1,emsg.c_str());
    error(emsg1);
   }
   pthread_mutex_lock(&mutex);
   while((fq.size())==queue_max)
   {
    pthread_cond_wait(&condf,&mutex);
   }
   fq.push(newsockfd);
   pthread_cond_signal(&conde);
   pthread_mutex_unlock(&mutex);
  }
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  //free(worker_thread_id);
  //free(worker_thread);
  return 0;
}
