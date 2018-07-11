#ifndef HTTPMESSAGE_HPP_
#define HTTPMESSAGE_HPP_
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <string.h>
#include <string>

using namespace std;

class HTTPMessage {
  public:
  string message;

  HTTPMessage() {
    message = "";
  }

  void setMessage(string msg) {
    message = msg;
  }

  void clearMessage() {
    message = "";
  }

  int sendResponse(int clientSocket) {
    if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
      cerr << "Error : Unable to send response header" << endl;
      return 1;
    }
    return 0;
  }


};

#endif // HTTPMESSAGE_HPP
