#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include "HTTPMessage.hpp"
#include "HTTPRequest.hpp"

using namespace std;

class HTTPResponse {
  public:
    int clientSocket;
    HTTPRequest http_req;
    const string server_header = "Server: Pantheon\r\n";
    string content_type;
    int content_length;
    HTTPMessage http_response;

    HTTPResponse(const HTTPRequest &request, int socket) {
      http_req = request;
      clientSocket = socket;
      http_response = HTTPMessage();
    }

    void processResponse() {
      createHTTPResponse();
      sendResponseHeader();
      if (http_req.error_code == 200) {
        sendResponseContent();
      }
      if (http_req.connectionClose || http_req.error_code == 400) {
        close(clientSocket);
      }
    }

    void createHTTPResponse() {

      //Creating the response header string
      string response_header = "HTTP/1.1 ";

      //Append the error code to the header
      if(http_req.error_code == 200) {
        response_header.append("200 OK\r\n");
      } else if(http_req.error_code == 400) {
        response_header.append("400 Client Error\r\n");
      } else if(http_req.error_code == 403) {
        response_header.append("403 Forbidden\r\n");
      } else if(http_req.error_code == 404) {
        response_header.append("404 Not Found\r\n");
      } else {
        cerr <<"Getting invalid error_code from request process"<< endl;
      }

      //Append server name
      response_header.append(server_header);

      //Append the corresponding key:value pair if request succssfully
      if(http_req.error_code == 200) {

        //Setting last_modified
      	struct tm *tmp = localtime(&(http_req.last_modified));
      	char outstr[200];
      	strftime(outstr, sizeof(outstr),"%a, %d %m %Y %H:%M:%S GMT\r\n",tmp);
        response_header.append("Last-Modified: ");
      	response_header.append(string(outstr));

        //Setting content_type
        response_header.append("Content-Type: ");
        //Check the ending of the file name to indicate the file types
        if(checkIfEnding(http_req.abs_filepath, ".jpg")) {
          response_header.append("image/jpeg\r\n");
        } else if(checkIfEnding(http_req.abs_filepath, ".png")) {
          response_header.append("image/png\r\n");
        } else if(checkIfEnding(http_req.abs_filepath, ".html")) {
          response_header.append("text/html\r\n");
        } else {
          cerr << "Incorrect file type" << endl;
        }

        //Setting content_length
        response_header.append("Content-Length: ");
        response_header.append(to_string(http_req.content_length));
        response_header.append("\r\n");
      }

      //Append the ending delimiter
      response_header.append("\r\n");

      //Set the HTTPMessage
      http_response.setMessage(response_header);
    }

    void sendResponseHeader() {
      http_response.sendResponse(clientSocket);
    }

    void sendResponseContent() {
      struct stat stat_buf;
      off_t offset = 0;
      cout << http_req.abs_filepath << endl;
      int fd = open(http_req.abs_filepath.c_str(), O_RDONLY);
      if (fd == -1) {
        	cerr << "Error : Unable to open file for sending" << endl;
      }
      fstat(fd, &stat_buf);
      if (sendfile(clientSocket, fd, &offset, stat_buf.st_size) == -1) {
  			cerr << "Error : sendfile() failed" << endl;
		  }
      close(fd);
    }

    int checkIfEnding(string str, string pattern) {
    	if (0 == str.compare(str.length() - pattern.length(), pattern.length(), pattern)) {
    		return 1; 	// ending with pattern
    	} else {
    		return 0;		// NOT ending with the pattern
    	}
    }

};

#endif // HTTPRESPONSE_HPP
