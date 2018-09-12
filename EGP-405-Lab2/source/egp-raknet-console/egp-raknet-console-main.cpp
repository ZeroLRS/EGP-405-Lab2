// Certificate of Authenticity
//
// EGP-405-01 Networking for Online Games
// Lab 2
// 7-30-2018
//
// Vedant Chaudhari, 1532077
// Lucas Spiker
//
// We certify that this work is entirely our own.The assessor of this project may reproduce this project 
// and provide copies to other academic staff, and/or communicate a copy of this project to a plagiarism 
// - checking service, which may retain a copy of the project on its database.

/*
	RakNet Console Application
	Prepared by Dan Buckstein
	September 5, 2018

	Simple RakNet application following official tutorials.

	****TO-DO: add your own implementations and comments

	Resources: 
	http://www.jenkinssoftware.com/raknet/manual/tutorialsample1.html
	http://www.jenkinssoftware.com/raknet/manual/tutorialsample2.html
	http://www.jenkinssoftware.com/raknet/manual/tutorialsample3.html

	http://www.raknet.net/raknet/manual/creatingpackets.html
	http://www.raknet.net/raknet/manual/receivingpackets.html
	http://www.raknet.net/raknet/manual/sendingpackets.html
	http://www.raknet.net/raknet/manual/timestamping.html

	Read them for the secrets to success!
	E.g. pointers on pointers ;)
*/


// standard includes
#include <stdio.h>
#include <string.h>
#include <list>

// RakNet includes
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/RakNetTypes.h"
#include "RakNet/RakPeerInterface.h"


enum GameMessages
{
	ID_CUSTOM_MESSAGE_START = ID_USER_PACKET_ENUM,

	ID_GAME_MESSAGE, 
	ID_CHAT_REQUEST,
	ID_CHAT_MESSAGE,
	ID_USERNAME_REQUEST,
	ID_USERNAME_MESSAGE,
	ID_USER_JOINED,
};

#pragma pack(push, 1)
struct BaseData
{
	unsigned char typeID;

	char message[256];
};
#pragma pack(pop)

// Generated by the client, a request for the host (server) to send out a message
#pragma pack(push, 1)
struct ChatRequest
{
	unsigned char typeID;

	char recipient[32];
	char message[512];
};
#pragma pack(pop)

// Generated by the server, a message being delivered to connected clients
#pragma pack(push, 1)
struct ChatMessage
{
	unsigned char typeID;

	bool isPrivate;

	char sender[32];
	char message[512];
};
#pragma pack(pop)

// This Structure holds the userID of all current peer connections to the server
struct userID
{
	char username[32];
	RakNet::RakNetGUID guid;
};

// entry function
int main(void)
{
	char str[512];
	bool isServer;

	unsigned int MAX_CLIENTS = 10;
	unsigned short SERVER_PORT = 60000;

	// Host Data
	char hostName[32] = "Server";
	std::list<userID> users;

	// Client Data
	char username[32] = "";

	// Create RakNet peer and packet instance
	RakNet::RakPeerInterface* pPeer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet* pPacket;

	// Ask user for peer type
	printf("(C)lient or (S)erver? \n");
	fgets(str, sizeof(str), stdin);

	// Ask user for the port
	printf("What is the server port? \n");
	scanf("%hu", &SERVER_PORT);

	// Setup Client Peer
	if ((str[0] == 'c') || (str[0] == 'C')) {
		RakNet::SocketDescriptor sd;
		pPeer->Startup(1, &sd, 1);

		isServer = false;
	}
	// Setup Host(Server) Peer
	else {
		printf("Maximum number of clients? \n");
		scanf("%d", &MAX_CLIENTS);

		RakNet::SocketDescriptor sd(SERVER_PORT, 0);
		pPeer->Startup(MAX_CLIENTS, &sd, 1);

		isServer = true;
	}

	// Set host parameters
	if (isServer) {
		printf("Setting up the server... \n");
		pPeer->SetMaximumIncomingConnections(MAX_CLIENTS);
	}
	// Set client parameters
	else {
		printf("Enter server IP or enter for localhost \n");
		fgets(str, sizeof(str), stdin);

		if (str[0] == '\n') {
			strcpy(str, "127.0.0.1");
		}

		printf("Enter a nickname: \n");
		fgets(username, sizeof(username), stdin);

		printf("Connecting to server... \n");
		pPeer->Connect(str, SERVER_PORT, 0, 0);
	}

	while (strcmp(str, "/quit\n") != 0)
	{	
		for (
			pPacket = pPeer->Receive(); 
			pPacket; 
			pPeer->DeallocatePacket(pPacket), pPacket = pPeer->Receive()
			)
		{
			switch (pPacket->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			{
				printf("Another client has disconnected.\n");
				break;
			}
			case ID_REMOTE_CONNECTION_LOST:
			{
				printf("Another client has lost the connection.\n");
				break;
			}
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
			{
				printf("Another client has connected.\n");
				break;
			}
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				// Host has accepted client connection
				printf("Our connection request has been accepted.\n");

				// Create packet to request current username from the server
				BaseData msg[1];
				msg->typeID = ID_USERNAME_REQUEST;
				strcpy(msg->message, username);

				// Send username message request
				pPeer->Send((char*)msg, sizeof(BaseData), HIGH_PRIORITY, RELIABLE, 0, pPacket->systemAddress, false);
			}
			break;
			case ID_USERNAME_REQUEST:
			{
				char usernameReq[31];
				bool isDuplicate = false;

				BaseData usernameMsg[1];
				usernameMsg->typeID = ID_GAME_MESSAGE;

				// Create packet to process client username request
				BaseData* data = (BaseData*)pPacket->data;
				strcpy(usernameReq, data->message);

				// Host has received a username request from a client
				printf("Username Request Initiated for ");
				printf(usernameReq);

				// Check if any connect peers have the same username
				for (userID currID : users) {
					if (strcmp(usernameReq, currID.username) == 0) {
						isDuplicate = true;
						break;
					}
				}
				// Check if the server username conflicts with the client username
				if (strcmp(usernameReq, hostName) == 0) {
					isDuplicate = true;
				}

				// Send username already exists message to peer
				if (isDuplicate == true) {
					strcpy(usernameMsg->message, "Username taken, reconnect and select a new one.");
					// End peer connection
					pPeer->Send((char*)usernameMsg, sizeof(BaseData), HIGH_PRIORITY, RELIABLE, 0, pPacket->systemAddress, false);
					pPeer->CloseConnection(pPacket->systemAddress, true);
				}
				// Accept connection if username is available
				else {
					strcpy(usernameMsg->message, "Username available, say hello!");

					// Add username to list tracked by server
					userID newUser;
					newUser.guid = pPacket->guid;
					strcpy(newUser.username, usernameReq);
					users.push_back(newUser);
					//printf("%s", users.size());

					// Send username available success packet
					pPeer->Send((char*)usernameMsg, sizeof(BaseData), HIGH_PRIORITY, RELIABLE, 0, pPacket->systemAddress, false);

					//***TODO: Server should broadcast a message welcoming new participant
				}
			}
			break;
			case ID_NEW_INCOMING_CONNECTION:
			{
				printf("A connection is incoming.\n");
				break;
			}
			case ID_NO_FREE_INCOMING_CONNECTIONS:
			{
				printf("The server is full.\n");
				break;
			}
			case ID_DISCONNECTION_NOTIFICATION:
			{
				if (isServer) {
					printf("A client has disconnected.\n");
				}
				else {
					printf("We have been disconnected.\n");
				}
				break;
			}
			case ID_CONNECTION_LOST:
			{
				if (isServer) {
					printf("A client lost the connection.\n");
				}
				else {
					printf("Connection lost.\n");
				}
				break;
			}
			case ID_GAME_MESSAGE:
			{
				BaseData* data = (BaseData*)pPacket->data;
				printf("%s \n", data->message);

				if (isServer == false) {
					fgets(str, sizeof(str), stdin);

					// Parse input for any specific conditions
					if (strcmp(str, "/help\n") == 0) {
						printf("==== COMMANDS ==== \n");
						printf("/quit -- Exit the Program \n");

						fgets(str, sizeof(str), stdin);
					}

					// Default: Send message packet
					ChatRequest msg[1];
					msg->typeID = ID_CHAT_REQUEST;
					strcpy(msg->message, str);

					pPeer->Send((char*)msg, sizeof(ChatRequest), HIGH_PRIORITY, RELIABLE, 0, pPacket->systemAddress, false);
				}

				break;
			}
			case ID_CHAT_REQUEST:
			{
				ChatRequest* data = (ChatRequest*)pPacket->data;
				ChatMessage outMsg[1];
				outMsg->sender[0] = 0;

				// Go through the users and find this message's sender
				for (userID currID : users)
				{
					if (currID.guid == pPacket->guid)
					{
						strcpy(outMsg->sender, currID.username);
					}
				}

				// Make sure we got a sender
				if (outMsg->sender[0] == 0)
				{
					printf("Sender user not registred.");
					break;
				}

				// If the message is not a private one
				if (strcmp(data->recipient, "") == 0)
				{
					outMsg->isPrivate = true;
					// Handle PM GUID switch here
				}
				else
				{
					outMsg->isPrivate = false;
				}

				outMsg->typeID = ID_CHAT_MESSAGE;
				strcpy(outMsg->message, data->message);

				printf("%s: %s", outMsg->sender, outMsg->message);

				pPeer->Send((char*)outMsg, sizeof(ChatMessage), HIGH_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
			}
				break;
			case ID_CHAT_MESSAGE:
			{
				ChatMessage* data = (ChatMessage*)pPacket->data;

				if (data->isPrivate)
				{
					printf("PM From %s: %s", data->sender, data->message);
				}
				else
				{
					printf("%s: %s", data->sender, data->message);
				}

			}
				break;
			default:
				printf("Message with identifier %i has arrived.\n", pPacket->data[0]);
				// Input loop
				break;
			}
		}
	}

	// Shut down & destroy peer
	RakNet::RakPeerInterface::DestroyInstance(pPeer);

	return 0;
}
