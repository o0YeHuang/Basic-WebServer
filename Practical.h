#ifndef PRACTICAL_H_
#define PRACTICAL_H_

#include <iostream>
#include <string.h>
#include <string>
#include <limits.h>
#include <stdlib.h>


#define CRLF "\r\n\r\n"
#define GET "GET"
#define HTTP_VER "HTTP/1.1"

using namespace std;

void DieWithUserMessage(string msg, string detail);
void DieWithSystemMessage(string msg);


#endif // PRACTICAL_H
