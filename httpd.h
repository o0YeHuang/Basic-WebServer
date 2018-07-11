#ifndef HTTPD_H
#define HTTPD_H

#include <iostream>
#include <string>
#include <string.h>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <thread>
#include "Practical.h"
#include "httpd.h"
#include "HTTPMessage.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

using namespace std;

static const int MAXPENDING = 5;

void start_httpd(int port, string doc_root);
string checkDocRoot(string doc_root);
int InitTCPServerSocket(int port);
void PerTCPClient(int clientSocket, string abs_doc_root, string clientAddress);
int AcceptTCPConnection(int serverSocket, struct sockaddr_storage &clntAddr);
void HandleTCPClient(int clientSocket, string doc_root, string clientAddress);
string getClientAddress(struct sockaddr_storage &clntAddr);
#endif // HTTPD_H
