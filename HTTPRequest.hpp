#ifndef HTTPREQUEST_HPP_
#define HTTPREQUEST_HPP_
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <utility>
#include "Practical.h"
#include "HTTPMessage.hpp"

using namespace std;

const string req_line_regex = "(\\w+)\\s(\\/\\S+.\\w+)\\s(\\w+\\/\\d.\\d)(\r\n)";
const string key_value_regex = "([\\w'-/]+:)\\s([\\w'-/]+)(\r\n)";

class HTTPRequest {
  public:
  HTTPMessage http_msg;
  string http_verb;
  string rel_filepath;
  string abs_filepath;
  int filetype;
  string http_version;
  string root_dir;
  string host;
  int file_size;
  time_t last_modified;
  int content_length;
  bool connectionClose;
  string client_ip;
  vector<pair<string,string>> otherKeyValPairs;
  int error_code;


  HTTPRequest(){}

  HTTPRequest(const HTTPMessage &message, string abs_doc_root, string clientAddress) {
    http_msg = message;
    http_verb = "";
    root_dir = abs_doc_root;
    rel_filepath = "";
    abs_filepath = "";
    http_version = "";
    host = "";
    file_size = 0;
    content_length = 0;
    connectionClose = false;
    client_ip = clientAddress;
    error_code = 200;
  }

  void processHTTPMessage() {
    size_t pos = 0;
    int count = 0;

    string delimiter = "\r\n";
    string msg_copy = http_msg.message;
    while ((pos = msg_copy.find(delimiter)) != string::npos) {
      string line = msg_copy.substr(0, pos + delimiter.length());
      if (count == 0) {
        count++;
        if (checkRequestLineRegex(line) == 0) {
          error_code = 400;
          cerr << "Error : Malformed request from client" <<endl;
          break;
        }
        if (parseRequestLineError(line) == true) {
          error_code = 400;
          cerr << "Error : Malformed request from client" <<endl;
          break;
        }
        if (checkFilePathError() == true) {
          error_code = 404;
          cerr << "Error : Malformed request from client" <<endl;
        } else {
          if (checkFilePermissionsError() == true) {
            error_code = 403;
            cerr << "Error : Malformed request from client" <<endl;
          }
        }
        if (checkClientAddressError() == true) {
          error_code = 403;
          cerr << "Error : Malformed request from client" <<endl;
        }
      } else {
        if (checkKeyValueLineRegex(line) == 0) {
          error_code = 400;
          cerr << "Error : Malformed request from client" <<endl;
          break;
        } else {
          parseKeyValueLine(line);
        }
      }
      msg_copy.erase(0, pos + delimiter.length());
    }
  }

  int checkRequestLineRegex(string request_line) {
    int match = 0;
    if (regex_match(request_line, regex(req_line_regex))) {
      match = 1;
    } else {
      cerr << "Error : Malformed Request Line" << endl;
    }
    return match;
  }

  bool parseRequestLineError(string request_line) {
    bool malformedRequest = false;
    istringstream parser(request_line);
    parser >> http_verb >> rel_filepath>> http_version;
    if (http_verb.compare(GET) != 0) {
      cerr << "Error : Invalid Client HTTP Action (not GET)" << endl;
      malformedRequest = true;
    }
    if (http_version.compare(HTTP_VER) != 0) {
      cerr << "Error: Invalid Client HTTP Version (not HTTP/1.1)" << endl;
      malformedRequest = true;
    }
    if (rel_filepath.back() == '/') {
      rel_filepath = rel_filepath + "/index.html";
    }
    return malformedRequest;
  }

  bool checkFilePathError() {
    bool badPath = false;
    char actual_path[PATH_MAX];
    string path = root_dir + rel_filepath;
    char *ptr = realpath(path.c_str(), actual_path);
    if (ptr != NULL) {
      abs_filepath = string(ptr);
      string copy_filepath = abs_filepath;
      copy_filepath.resize(root_dir.length());
      if(root_dir.compare(copy_filepath) != 0) {
        badPath = true;
        cerr << "Error : Requested file escapes doc_root" << endl;
      }
    }
    else if (errno == ENOENT) {
      badPath = true;
      cerr << "Error : Requested file does not exist" << endl;
    } else {
      badPath = true;
      cerr << "Error : Unable to resolve absolute file path" << endl;
    }
    return badPath;
  }

  bool checkFilePermissionsError() {
    bool badPermissions = false;
    struct stat fileInfo;
    int fd = 0;
    if ((fd = open(abs_filepath.c_str(), O_RDONLY)) != -1) {
      fstat(fd, &fileInfo);
      if ((fileInfo.st_mode & S_IROTH) != S_IROTH) {
        error_code = 403;
        badPermissions = true;
      }
      content_length = int(fileInfo.st_size);
      last_modified = fileInfo.st_mtime;
      close(fd);
    } else {
      error_code = 403;
      badPermissions = true;
      cerr << "Error : Issue with opening file to check permissions" << endl;
    }
    return badPermissions;
  }

  bool checkClientAddressError() {
    bool blockedClient = false;

    string htaccess = abs_filepath.substr(0, abs_filepath.rfind("/")) + "/.htaccess";

    ifstream ip_rules(htaccess);
    if (ip_rules.is_open()) {
      string rule;
      while (getline(ip_rules, rule)) {
        istringstream parser(rule);
        string rule, garbage, mask_address;
        parser >> rule >> garbage >> mask_address;
        size_t pos = mask_address.find("/");
        string ip_address = mask_address.substr(0, pos);
        string subnet = mask_address.erase(0, pos + 1);
        cout << "ip: " << ip_address << endl << "subnet: " << subnet << endl;
        if (rule.compare("deny") == 0) {
          if (ip_address.compare("0.0.0.0") == 0) {
            blockedClient = true;
          }
          if (subnet.compare("32") == 0) {
            if (client_ip.compare(ip_address) == 0) {
              blockedClient = true;
            }
          }
          else if (subnet.compare("24") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_24 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_24, cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_24 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_24, cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = true;
            }
          }
          else if (subnet.compare("16") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_16 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_16, cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_16 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_16, cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = true;
            }
          }
          else if (subnet.compare("8") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_8 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_8, cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_8 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_8, cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = true;
            }
          }
        } else {
          if (ip_address.compare("0.0.0.0") == 0) {
            blockedClient = false;
          }
          if (subnet.compare("32") == 0) {
            if (client_ip.compare(ip_address) == 0) {
              blockedClient = false;
            }
          }
          else if (subnet.compare("24") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_24 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_24, cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_24 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_24, cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = false;
            }
          }
          else if (subnet.compare("16") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_16 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_16, cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_16 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_16, cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = false;
            }
          }
          else if (subnet.compare("8") == 0) {
            string cpy_clientAddress = client_ip;
            size_t pos_8 = cpy_clientAddress.rfind(".");
            cpy_clientAddress.erase(pos_8, cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            cpy_clientAddress.erase(cpy_clientAddress.rfind("."), cpy_clientAddress.length());
            string cpy_ipaddr = ip_address;
            pos_8 = cpy_ipaddr.rfind(".");
            cpy_ipaddr.erase(pos_8, cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            cpy_ipaddr.erase(cpy_ipaddr.rfind("."), cpy_ipaddr.length());
            if (cpy_ipaddr.compare(cpy_clientAddress) == 0) {
              blockedClient = false;
            }
          }
        }
      }
    }

    return blockedClient;
  }

  int checkKeyValueLineRegex(string keyValue_line) {
    int match = 0;
    if (regex_match(keyValue_line, regex(key_value_regex))) {
      match = 1;
    } else {
      cerr << "Error : Malformed Request Line" << endl;
    }
    return match;
  }

  void parseKeyValueLine(string keyValue_line) {
    istringstream parser(keyValue_line);
    string key, value;
    parser >> key >> value;

    if (key.compare("Host:") == 0) {
      host = value;
    }
    else if (key.compare("Connection:") == 0) {
      if (value.compare("close") == 0) {
        connectionClose = true;
      } else {
        connectionClose = false;
      }
    } else {
      otherKeyValPairs.emplace_back(make_pair(key, value));
    }
  }
};



#endif // HTTPREQUEST_HPP_
