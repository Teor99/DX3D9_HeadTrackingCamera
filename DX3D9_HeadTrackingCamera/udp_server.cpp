#include <iostream>
#include <winsock2.h>
#include <chrono>
#include <thread>
#include "upd_server.h"

using namespace std;

#pragma comment(lib,"ws2_32.lib") // Winsock Library
#pragma warning(disable:4996) 

#define BUFLEN 512
#define PORT 8888

CameraCoordsPacket cc;
bool isNeedToStopThread = false;

void stopUdpServer() {
	isNeedToStopThread = true;
}

int udpServer()
{
	//system("title UDP Server");

	sockaddr_in server, client;

	// initialise winsock
	WSADATA wsa;

	//printf("Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code: %d", WSAGetLastError());
		exit(0);
	}
	printf("Initialised.\n");

	// create a socket
	SOCKET server_socket;
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket: %d", WSAGetLastError());
	}
	//printf("Socket created.\n");

	// prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	// bind
	if (bind(server_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code: %d", WSAGetLastError());
		exit(1);
	}
	//puts("Bind done.");

	while (!isNeedToStopThread)
	{
		//printf("Waiting for data...");
		fflush(stdout);
		char message[BUFLEN] = {};

		// try to receive some data, this is a blocking call
		int message_len;
		int slen = sizeof(sockaddr_in);
		if (message_len = recvfrom(server_socket, message, BUFLEN, 0, (sockaddr*)&client, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code: %d", WSAGetLastError());
			break;
		}

		double* arr = (double*)message;
		cc.x = arr[0];
		cc.y = arr[1];
		cc.z = arr[2];
		cc.yaw = arr[3];
		cc.pitch = arr[4];
		cc.roll = arr[5];
	}

	closesocket(server_socket);
	WSACleanup();
}