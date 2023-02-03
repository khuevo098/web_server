#include "stdafx.h"
#include "Demo_Server.h"
#include "afxsock.h" // For CSocket
#include <string>
#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PORT 8080
#define SIZEBUFF 1000

// The one and only application object
CWinApp theApp;
using namespace std;

struct info {
	string uname;
	string psw;
};

struct Request {
	string method, path;
	info user;
};

struct Response {
	int statusCode;
	size_t contentLength;
	string contentType;
	string statusText;
	string content;
};

/* Cac ham parse thong tin request */
/* Parse user's info */
void ParseInfo(stringstream& ss, info& user)
{
	string label;
	getline(ss, label, '=');
	getline(ss, user.uname, '&');
	getline(ss, label, '=');
	getline(ss, user.psw, '&');
}
/* Parse client's request */
void ParseRequest(Request& req, stringstream& ss)
{
	// Parse request-line
	getline(ss, req.method, ' ');
	getline(ss, req.path, ' ');

	// Parse body neu method la POST
	if (req.method == "POST") {
		string line;
		while (getline(ss, line));

		stringstream body;
		body << line;
		ParseInfo(body, req.user);
	}
}
/* Parse path, find content-type */
void ParsePath(string path, string& type) {
	// Find position of '.'
	int pos = path.find(".");
	// Copy substring after pos
	string filename = path.substr(pos + 1);

	if (filename == "html")
		type = "text/html";
	else if (filename == "css")
		type = "text/css";
	else if (filename == "png")
		type = "image/png";
	else if (filename == "jpg" || filename == "jpeg")
		type = "image/jpeg";
	else type = "application/octet-stream";
}

/* Ham tao response */
void CreateRes(stringstream& ss, Response res)
{
	// Status line
	ss << "HTTP/1.1 " << res.statusCode << " " << res.statusText << "\r\n";
	// Header fields
	ss << "Content-Length: " << res.contentLength << "\r\n";
	ss << "Content-Type: " << res.contentType << "\r\n";
	ss << "Connection: keep-alive\r\n"; // For multiple requests
	ss << "\r\n";
	// Body
	ss << res.content;
}

/* Handle error */
void ErrorMsg(string msg)
{
	cout << msg << ": " << GetLastError() << "\n";
}

DWORD WINAPI ThreadFunc(LPVOID arg) 
{
	SOCKET* hConnected = (SOCKET*)arg;
	CSocket mysock;
	mysock.Attach(*hConnected);

	int iRecv = 0; // Check receive
	do
	{
		stringstream output;
		Response res;
		char buffer[SIZEBUFF]; // Buffer nhan request
		memset(buffer, 0, SIZEBUFF);
		
		// Nhan request tu client
		iRecv = mysock.Receive(buffer, SIZEBUFF, 0);
		
		// Nhan thanh cong
		if (iRecv > 0) 
		{ 
			// In request nhan duoc tu client
			cout << "\n====================REQUEST====================\n";
			cout << buffer << "\n";
			
			// Luu vao stream va parse request
			stringstream ssReq;
			Request req;
			ssReq << buffer;
			ParseRequest(req, ssReq);

			if (req.path == "/")
				req.path = "/index.html";

			// Content-Type
			ParsePath(req.path, res.contentType);

			// GET method
			ifstream file;
			if (req.method == "GET")
			{
				// Open file
				file.open("web_src" + req.path, ios::in | ios::binary);
				if (file.is_open())
				{
					string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
					file.close();
					res.statusCode = 200;
					res.statusText = "OK";
					res.content = content;
					res.contentLength = content.size();
				}
				else
				{
					file.open("web_src/404_error.html", ios::in | ios::binary);
					if (file.is_open())
					{
						string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
						file.close();
						res.statusCode = 404;
						res.statusText = "Not Found";
						res.contentType = "text/html";
						res.content = content;
						res.contentLength = content.size();
					}
				}
			}

			// POST method
			else
			{
				if (req.user.uname == "admin" && req.user.psw == "123456")
				{
					// Open file
					file.open("web_src/images.html", ios::in | ios::binary);
					if (file.is_open())
					{
						string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
						file.close();
						res.statusCode = 200;
						res.statusText = "OK";
						res.content = content;
						res.contentLength = content.size();
					}
				}
				else
				{
					file.open("web_src/401_error.html", ios::in | ios::binary);
					if (file.is_open())
					{
						string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
						file.close();
						res.statusCode = 401;
						res.statusText = "Unauthorized";
						res.contentType = "text/html";
						res.content = content;
						res.contentLength = content.size();
					}
				}
			}
	
			// Send data
			CreateRes(output, res);
			int iSend = mysock.Send(output.str().c_str(), output.str().size()); // Check send
			if (iSend > 0)
				cout << "\nServer: " << req.path << " has been sent\n";
			else ErrorMsg("Server: Send failed");
		}
		else if (iRecv == 0)
			cout << "\nClient: Closed connection\n";
		else
			ErrorMsg("Server: Receive failed");

	} while (iRecv > 0);
	mysock.Close();
	delete hConnected;
	return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL); //Handle module

	if (hModule != NULL)
	{
		if (hModule != NULL)
		{
			// initialize MFC and print an error on failure
			if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
				ErrorMsg("MFC initialization failed");
			else
			{
				if (AfxSocketInit(NULL) == FALSE) 
					ErrorMsg("Cannot create socket library");
				
				CSocket server;
				if (server.Create(PORT, SOCK_STREAM, NULL) == 0)
					ErrorMsg("Socket failed with error"); 
				else
				{
					cout << "\nServer is listening...\n";
					if (server.Listen() == 0) 
						ErrorMsg("Listen failed");
					while (1) 
					{
						CSocket s;
						if (server.Accept(s))
						{
							cout << "\nAccept connection\n";
							DWORD threadID;
							HANDLE threadStatus;
							SOCKET* hConnected = new SOCKET();
							*hConnected = s.Detach();
							threadStatus = CreateThread(NULL, 0, ThreadFunc, hConnected, 0, &threadID);
						}
						else ErrorMsg("Accept failed");
					} 
				}
			}
		}
		else
			ErrorMsg("Fatal Error: GetModuleHandle failed");
		return nRetCode;
	}
}