/*********************************************************************
* TwitCon.c / True Console Twitter update with no dependancies.      *
**********************************************************************
* Copyright (c) 2008, Brian Hartvigsen                               *
*   <brian.andrew@brianandjenny.com>                                 *
* All rights reserved.                                               *
* Redistribution and use in source and binary forms, with or without *
* modification, are permitted provided that the following conditions *
* are met:                                                           *
*                                                                    *
*   * Redistributions of source code must retain the above copyright *
*     notice, this list of conditions and the following disclaimer.  *
*   * Redistributions in binary form must reproduce the above        *
*     copyright notice, this list of conditions and the following    *
*     disclaimer in the documentation and/or other materials         *
*     provided with the distribution.                                *
*   * Neither the name of the TwitCon nor the names of its           *
*     contributors may be used to endorse or promote products        *
*     derived from this software without specific prior written      *
*     permission.                                                    *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS  *
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,           *
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED    *
* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,      *
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON  *
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR *
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF *
* THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF    *
* SUCH DAMAGE.                                                       *
*                                                                    *
*********************************************************************/

/*                                                                    
* C Implementation of EveryDNS.net Dynamic Domain Update
*
* Copyright (c) 2001, Matt Whitlock <mwhitlock@whitsoftdev.com>
* All rights reserved.
*
* This software is OSI Certified Open Source Software. Use of this source
* code is covered by the BSD Open Source license and is subject to the terms
* and conditions specified therein.
*/
#include <stdio.h>

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <stdlib.h>
# include <windows.h>
# include <winsock.h>
#endif

#define TARGET "twitter.com"
#define VERSION "0.1"

// The base64 character set
const char *pszBase64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char pszLogFile[MAX_PATH + 1];
char pszConfFile[MAX_PATH + 1];


void base64_encode(char *pszIn, char *pszOut)
{
// Encodes the string at pszIn into base64 and stores the result at pszOut.

	DWORD dwBits=0, dwInChars=0;

	for (;;pszIn+=3,pszOut+=4) {
		if (pszIn[0]) {
			dwBits=pszIn[0]<<16;
			if (pszIn[1]) {
				dwBits|=pszIn[1]<<8;
				if (pszIn[2]) {
					dwBits|=pszIn[2];
				}
			}
		} else {
			*pszOut=0;
			return;
		}
		pszOut[0]=pszBase64[dwBits>>18];
		pszOut[1]=pszBase64[dwBits>>12&0x3F];
		if (pszIn[1]) {
			pszOut[2]=pszBase64[dwBits>>6&0x3F];
			if (pszIn[2]) {
				pszOut[3]=pszBase64[dwBits&0x3F];
			} else {
				pszOut[3]='=';
				pszOut[4]=0;
				return;
			}
		} else {
			pszOut[2]='=';
			pszOut[3]='=';
			pszOut[4]=0;
			return;
		}
	}
}

char* compact_options(int start, int argc, char **argv)
{
	int argi;
	char *sep = "";
	char *temp = "", *temp2;

	for (argi = start; argi < argc; argi++) {
		char *temp3;
		temp2 = (char *)malloc(strlen(argv[argi]) + strlen(temp) + strlen(sep) + 1);
		sprintf(temp2, "%s%s%s", temp, sep, argv[argi]);
		sep = " ";
		temp3 = temp;
		temp = temp2;
		if (temp3 != "")
			free(temp3);
	}
	return temp;
}

void LogMessage(char *message)
{
	static FILE *fp = -1;

	if (fp == -1)
	{
		fp = fopen(pszLogFile, "r");
		if (fp != NULL)
		{
			fclose(fp);
			fp = fopen(pszLogFile, "a");
		}
	}
	if (fp == NULL)
#ifdef WIN32
		MessageBox(NULL, message, "TwitCon", MB_ICONEXCLAMATION);
#else
		printf(message);
#endif
	else
		fputs(message, fp);
}


#ifdef WIN32
#define argc __argc
#define argv __argv

/* Use WinMain on windows so it doesn't open a console dialog. */
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#else
int main(int argc, char** argv)
{
#endif
	char *psz, *pszStatus=0;
	char pszUsername[4096], pszPassword[4096];
	char szTemp[4096], szAuthString64[256];
#ifdef WIN32
	WSADATA wsad;
#endif
	SOCKET s;
	SOCKADDR_IN sai;
	HOSTENT *phe;
	int iBytes, startParam = 1;
	FILE *fp;

#ifdef WIN32
	GetModuleFileName(NULL, pszConfFile, MAX_PATH);
	*strrchr(pszConfFile, '\\') = 0;
	strcpy(pszLogFile, pszConfFile);
	strcat(pszConfFile, "\\twitter.conf");
	strcat(pszLogFile, "\\twitter.log");
#else
	pszConfFile = "~/.twitcon";
	pszLogFile = "~/twitcon.log";
#endif

	fp = fopen(pszConfFile, "rt");
	if (fp != NULL)
	{
		fgets(pszUsername, 4095, fp);
		fgets(pszPassword, 4095, fp);
		for (iBytes = strlen(pszUsername) - 1; pszUsername[iBytes] == '\r' || pszUsername[iBytes] == '\n' || pszUsername[iBytes] == ' '; pszUsername[iBytes] = '\0', iBytes--);
		for (iBytes = strlen(pszPassword) - 1; pszPassword[iBytes] == '\r' || pszPassword[iBytes] == '\n' || pszPassword[iBytes] == ' '; pszPassword[iBytes] = '\0', iBytes--);
		fclose(fp);
	}
	else
	{
		if (argc < 3)
		{
			sprintf(szTemp, "%s username password status message here", argv[0]);
			return 1;
		}
		strcpy(pszUsername, argv[1]);
		strcpy(pszPassword, argv[2]);
		startParam = 3;
	}

	sprintf(szTemp,"%s:%s",pszUsername,pszPassword);
	base64_encode(szTemp,szAuthString64);

#ifdef WIN32
	if (WSAStartup(MAKEWORD(1,1),&wsad)) {
		LogMessage("Error: Unable to start Winsock.");
		return 1;
	}
#endif

	// Resolve the target host name
	phe=gethostbyname(TARGET);
	if (!phe) {
#ifdef WIN32
		WSACleanup();
#endif
		sprintf(szTemp,"Error: Unable to resolve address for %s.\n",TARGET);
		LogMessage(szTemp);
		return 1;
	}

	// Initialize and fill the socket address structure.
	ZeroMemory(&sai,sizeof(SOCKADDR_IN));
	sai.sin_family=AF_INET;
	sai.sin_addr=*(IN_ADDR*)phe->h_addr_list[0];
	sai.sin_port=htons(80);

	// Create a socket for the connection.
	s=socket(AF_INET,SOCK_STREAM,PF_UNSPEC);
	if (s==INVALID_SOCKET) {
#ifdef WIN32
		WSACleanup();
#endif
		LogMessage("Error: Unable to create socket.");
		return 1;
	}

	// Connect the socket to the address initialized above.
	if (connect(s,(SOCKADDR*)&sai,sizeof(SOCKADDR_IN))) {
		closesocket(s);
#ifdef WIN32
		WSACleanup();
#endif
		sprintf(szTemp,"Error: Unable to connect to %s (%s).\n",TARGET,inet_ntoa(sai.sin_addr));
		LogMessage(szTemp);
		return 1;
	}

		// Construct the HTTP GET request
	strcpy(szTemp,"POST /statuses/update.xml");
	sprintf(strchr(szTemp,0)," HTTP/1.1\nUser-Agent: TwitCon %s\nHost: %s\nContent-Type: application/x-www-form-urlencoded\nAuthorization: Basic %s\n",VERSION,TARGET,szAuthString64);

	pszStatus = compact_options(startParam, argc, argv);
	sprintf(strchr(szTemp, 0), "Content-Length: %i\n\nstatus=%s",strlen(pszStatus) + 7, pszStatus);
	free(pszStatus);

	// Send the HTTP GET request to the server
	if (send(s,szTemp,strlen(szTemp),0)==SOCKET_ERROR) {
		closesocket(s);
#ifdef WIN32
		WSACleanup();
#endif
		LogMessage("Error: Unable to send request to server.");
		return 1;
	}

	// Read the HTTP response from the server
	iBytes=recv(s,szTemp,4096,0);
	if (iBytes==SOCKET_ERROR) {
		closesocket(s);
#ifdef WIN32
		WSACleanup();
#endif
		LogMessage("Error: Unable to receive response from server.");
		return 1;
	}

	// Attempt to locate the exit code returned by the server script.
	// Report to the user the result of the update based on the exit code.
	// Note that this section overlaps psz and szTemp, but this shouldn't be a
	// problem since the description of the result should always be shorter than
	// the HTTP response header, which is not used.
	szTemp[iBytes]=0;
	if (psz=strstr(szTemp,"\r\n\r\n")) psz+=4;
	if (strstr(szTemp, "<error>"))
		LogMessage(psz);
	// Close the socket and cleanup Winsock
	closesocket(s);
#ifdef WIN32
	WSACleanup();
#endif

	// Return 0 to the OS, indicating no error
	return 0;
}