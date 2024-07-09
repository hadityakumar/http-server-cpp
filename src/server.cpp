#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <thread>
#include <sstream>
#include <fstream>

void handleClient(int client)
{
  std::string STATUS_OK = "HTTP/1.1 200 OK\r\n\r\n";
  std::string NOT_FOUND = "HTTP/1.1 404 Not Found\r\n\r\n";

  char buffer[1024];
  recv(client, buffer, 1024, 0);
  std::string request(buffer);

  std::istringstream stream(request);
  std::string line;

  std::getline(stream, line);
  std::istringstream requestLine(line);
  std::string method, path, httpVersion;
  requestLine >> method >> path >> httpVersion;

  std::map<std::string, std::string> map;
  while (std::getline(stream, line) && line != "\r")
  {
    std::string key = line.substr(0, line.find(":"));
    std::string value = line.substr(line.find(":") + 2);
    if (!value.empty() && value[0] == ' ')
      value.erase(0, 1);
    if (!value.empty() && value.back() == '\r')
      value.pop_back();
    map[key] = value;
  }

  if (path == "/")
  {
    send(client, STATUS_OK.c_str(), STATUS_OK.size(), 0);
  }
  else if (path.size() >= 5 && path.substr(0, 5) == "/echo")
  {
    size_t slashPos = path.find_last_of('/');
    std::string fileName = path.substr(slashPos + 1);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(fileName.size()) + "\r\n\r\n" + fileName;
    send(client, response.c_str(), response.size(), 0);
  }
  else if (path.size() >= 11 && path.substr(0, 11) == "/user-agent")
  {
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(map["User-Agent"].size()) + "\r\n\r\n" + map["User-Agent"];
    send(client, response.c_str(), response.size(), 0);
  }

  else if(path.size() >= 6 && path == "/files"){
    std::string fileName = path.substr(8);
    std::cout<<dir<<fileName<<"\n";
    std::ifstream file(dir + fileName);

    if(file.good()){
      std::stringstream buffer = file.rdbuf();
      std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "+ buffer.str().size() + "\r\n\r\n" + buffer.str();
      send(client, response.c_str(), response.size(), 0);
    }
    else{
      send(client, NOT_FOUND.c_str(), NOT_FOUND.size(), 0);
    }
  }

  else
  {
    send(client, NOT_FOUND.c_str(), NOT_FOUND.size(), 0);
  }
  close(client);
  return;
}



int main(int argc, char **argv)
{
  std::string dir;

  if(argc == 3 && strcmp(argv[1], "--directory")==0)
    dir = argv[2];

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  // so we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";



  while (true)
  {
    int client = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (client < 0)
    {
      std::cerr << "Failed to accept client";
      continue;
    }
    std::cout << "Client connected\n";
    std::thread(handleClient, client).detach();
  }

  close(server_fd);
  return 0;
}
