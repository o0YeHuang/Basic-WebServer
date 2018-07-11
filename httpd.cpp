#include "httpd.h"

using namespace std;

void start_httpd(int port, string doc_root)
{
	string abs_doc_root = checkDocRoot(doc_root);
	if (abs_doc_root.empty()) {
		perror("doc_root");
		exit(EXIT_FAILURE);
	}

	cerr << "Starting server (port: " << port << ", doc_root: " << abs_doc_root << ")" << endl;

	int serverSocket = InitTCPServerSocket(port);
	if (serverSocket < 0) {
		DieWithUserMessage("SetupTCPServerSocket() failed", to_string(port));
	}

	for (;;) {
		struct sockaddr_storage clntAddr; // Client address
		int clientSocket = AcceptTCPConnection(serverSocket, clntAddr);
		string clientAddress = getClientAddress(clntAddr);
		thread(PerTCPClient, clientSocket, abs_doc_root, clientAddress).detach();
	}
	// NOT REACHED
  close(serverSocket);
}

int InitTCPServerSocket(int port) {
	int serverSocket;
	int optval = 1;
	struct addrinfo addrCriteria;
	struct addrinfo *servAddr; // List of server addresses

	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
  addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol

	// Establish server address for client connection
	int rtnVal = getaddrinfo(NULL, to_string(port).c_str(), &addrCriteria, &servAddr);
	if (rtnVal != 0) {
		cerr << "Error : getaddrinfo() failed - " << gai_strerror(rtnVal) << endl;
		exit(1);
	}

	serverSocket = -1;
	for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) {
		// Create a TCP socket
		serverSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (serverSocket < 0) {
			continue;       // Socket creation failed; try next address
		}

		// Bind and Listen on Server Socket
		if ((bind(serverSocket, addr->ai_addr, addr->ai_addrlen) == 0) &&
				(listen(serverSocket, MAXPENDING) == 0)) {
			// Set Server Socket to reuse port and address
			if (setsockopt(serverSocket, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),
					(char*)&optval,sizeof(optval)) < 0) {
				cerr << "Error : setsockopt() failed" << endl;
				close(serverSocket);
				exit(1);
			}
			break;       // Bind and listen successful
		} else {
			cerr << "Error : bind() and listen() on socket failed" << endl;
			exit(1);
		}

		close(serverSocket);  // Close and try again
		serverSocket = -1;
	}
	// Free address list allocated by getaddrinfo()
	freeaddrinfo(servAddr);
	// Return Server Socket that was succssfully created, binded, and listening on
	return serverSocket;
}

int AcceptTCPConnection(int serverSocket, struct sockaddr_storage &clntAddr) {
	// Set length of client address structure (in-out parameter)
	socklen_t clntAddrLen = sizeof(clntAddr);

	// Accept any Client Socket connections if any
	int clientSocket = accept(serverSocket, (struct sockaddr *) &clntAddr, &clntAddrLen);
	if (clientSocket < 0) {
	 cerr << "Error : accept() failed" << endl;
	 exit(1);
	}
	// Return connected/accepted Client Socket
	return clientSocket;
}

void PerTCPClient(int clientSocket, string abs_doc_root, string clientAddress) {
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) > 0) {
		cerr << "Error : setsockopt() failed" << endl;
	}

	HandleTCPClient(clientSocket, abs_doc_root, clientAddress);
}

void HandleTCPClient(int clientSocket, string abs_doc_root, string clientAddress) {
	char buf[1024];
	ssize_t numBytesRcvd;

	//init the tmp_buf as an empty string
	string tmp_buf = "";

	memset(buf, 0, 1024);
	// HTTPMessage http_msg;

	while ((numBytesRcvd = recv(clientSocket, buf, 1024, 0)) > 0) {
		//convert buf to string
		string buffer(buf);
		//always append the new buf to tmp_buf
		buffer = tmp_buf.append(buffer);
		//parse the buffer string
		size_t pos = 0;
		string delimiter = CRLF;
		while ((pos = buffer.find(delimiter)) != string::npos) {
			string token = buffer.substr(0, pos);
			cout << "Recevied request from client {" << clientAddress << "}:" << endl;
			cout << token << endl;
			HTTPMessage http_msg;
			http_msg.setMessage(token);
			HTTPRequest http_req(http_msg, abs_doc_root, clientAddress);
			http_req.processHTTPMessage();
			HTTPResponse http_resp(http_req, clientSocket);
			http_resp.processResponse();
	    buffer.erase(0, pos + delimiter.length());
			//check if reach the end of the buffer
			if(buffer.find(delimiter) == string::npos){
				//set the tmp_buf
				tmp_buf = buffer;
				break;
			}
		}
	}
}

string checkDocRoot(string doc_root) {
	char abs_path[PATH_MAX];
	char *resolved_path = realpath(doc_root.c_str(), abs_path);
	string abs_doc_root;
	if (resolved_path != NULL) {
		return string(resolved_path);
	}
	return "";
}

string getClientAddress(struct sockaddr_storage &clntAddr) {
	char ipstr[INET6_ADDRSTRLEN];
	// Get the Client's IP Address
	if (clntAddr.ss_family == AF_INET) {
		// AF_INET
		struct sockaddr_in *s = (struct sockaddr_in *)&clntAddr;
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else {
		// AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clntAddr;
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	return string(ipstr);
}
