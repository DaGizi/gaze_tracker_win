/*
 * This is an example that demonstrates how to connect to the EyeX Engine and subscribe to the lightly filtered gaze data stream.
 *
 * Copyright 2013-2014 Tobii Technology AB. All rights reserved.
 */
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "eyex\EyeX.h"

//TCP server includes
#include <winsock2.h>


#pragma comment (lib, "Tobii.EyeX.Client.lib")

#pragma comment(lib, "Ws2_32.lib")


#define DEFAULT_PORT        5150
#define DEFAULT_BUFFER      4096

// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Twilight Sparkle";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

//TCP parameters
int    iPort = DEFAULT_PORT; // Port to listen for clients on
BOOL   bInterface = FALSE,	 // Listen on the specified interface
bRecvOnly = FALSE;   // Receive data only; don't echo back
char   szAddress[128];       // Interface to listen for clients on

//TCP params from main()
WSADATA       wsd;
SOCKET        sListen, sClient;
int           iAddrSize;
HANDLE        hThread;
DWORD         dwThreadId;
struct sockaddr_in local,client;
BOOL gotClient = FALSE;
//Gaze data for server send
double gazeX, gazeY;

//
// Function: usage
//
// Description:
//    Print usage information and exit
//
void usage()
{
	printf("usage: server [-p:x] [-i:IP] [-o]\n\n");
	printf("       -p:x      Port number to listen on\n");
	printf("       -i:str    Interface to listen on\n");
	printf("       -o        Don't echo the data back\n\n");
	ExitProcess(1);
}

//
// Function: ValidateArgs
//
// Description:
//    Parse the command line arguments, and set some global flags
//    to indicate what actions to perform
//
void ValidateArgs(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
		{
			switch (tolower(argv[i][1]))
			{
			case 'p':
				iPort = atoi(&argv[i][3]);
				break;
			case 'i':
				bInterface = TRUE;
				if (strlen(argv[i]) > 3)
					strcpy(szAddress, &argv[i][3]);
				break;
			case 'o':
				bRecvOnly = TRUE;
				break;
			default:
				usage();
				break;
			}
		}
	}
}

/*
 * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
 */
BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txSetGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
			BOOL success;
			printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
			// commit the snapshot with the global interactor as soon as the connection to the engine is established.
			// (it cannot be done earlier because committing means "send to the engine".)
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
			if (!success) {
				printf("Failed to initialize the data stream.\n");
			}
			else
			{
				printf("Waiting for gaze data to start streaming...\n");
			}
		}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

/*
 * Handles an event from the Gaze Point data stream.
 */
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	char szBuff[DEFAULT_BUFFER];
	char gazeXbuff[10];
	char gazeYbuff[10];
	int Decimal = 0;
	int Sign = 0;
	_snprintf(gazeXbuff, sizeof(gazeXbuff), "%i", (int)gazeX);
	_snprintf(gazeYbuff, sizeof(gazeYbuff), "%i", (int)gazeY);
	strcpy(szBuff, gazeXbuff);
	strcat(szBuff, ",");
	strcat(szBuff, gazeYbuff);
	int ret, nLeft, idx;
	while (gotClient)
	{
		if (!bRecvOnly)
		{
			nLeft = 1;
			idx = 0;
			//
			// Make sure we write all the data
			//
			while (nLeft > 0)
			{
				ret = send(sClient, &szBuff[idx], 10, 0);
				if (ret == 0)
					break;
				else if (ret == SOCKET_ERROR)
				{
					printf("send() failed: %d\n",
						WSAGetLastError());
					break;
				}
				nLeft = 0;
				//Sleep(1000);
			}
			gotClient = FALSE;
		}
		
	}
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		printf("Gaze Data: (%.1f, %.1f) timestamp %.0f ms\n", eventParams.X, eventParams.Y, eventParams.Timestamp);
		gazeX = eventParams.X;
		gazeY = eventParams.Y;
		gotClient = TRUE;
	} else {
		printf("Failed to interpret gaze data event packet.\n");
	}
}

/*
 * Callback function invoked when an event has been received from the EyeX Engine.
 */
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;
	while (!gotClient)
	{
		iAddrSize = sizeof(client);
		sClient = accept(sListen, (struct sockaddr *)&client,
			&iAddrSize);
		if (sClient == INVALID_SOCKET)
		{
			printf("accept() failed: %d\n", WSAGetLastError());
			break;
		}
		printf("Accepted client: %s:%d\n",
			inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		gotClient = TRUE;
	}
    txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}

/*
 * Application entry point.
 */
int main(int argc, char* argv[])
{
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;

	ValidateArgs(argc, argv);
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("Failed to load Winsock!\n");
		return 1;
	}
	// Create our listening socket
	//
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sListen == SOCKET_ERROR)
	{
		printf("socket() failed: %d\n", WSAGetLastError());
		return 1;
	}
	// Select the local interface and bind to it
	//
	if (bInterface)
	{
		local.sin_addr.s_addr = inet_addr(szAddress);
		if (local.sin_addr.s_addr == INADDR_NONE)
			usage();
	}
	else
		local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(iPort);

	if (bind(sListen, (struct sockaddr *)&local,
		sizeof(local)) == SOCKET_ERROR)
	{
		printf("bind() failed: %d\n", WSAGetLastError());
		return 1;
	}
	listen(sListen, 8);

	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeSystem(TX_SYSTEMCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(hContext);
	success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(hContext) == TX_RESULT_OK;

	// let the events flow until a key is pressed.
	if (success) {
		printf("Initialization was successful.\n");
	} else {
		printf("Initialization failed.\n");
	}
	printf("Press any key to exit...\n");
	_getch();
	printf("Exiting.\n");

	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
	txReleaseContext(&hContext);

	return 0;
}
