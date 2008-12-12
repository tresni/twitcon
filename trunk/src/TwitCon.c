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
# define argc __argc
# define argv __argv
# define stricmp _stricmp
#endif

#define TARGET "twitter.com"
#define URL ""
#define VERSION "1.0"

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

char* compact_arguments(int start, int argc, char **argv)
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

/* URL encode the message.  Also need to escape &, <, and > */
char* url_encode(const char* string, int* length)
{
	const char encode_chars[] = { '%', '$', '&', '+', ',', '/', ':', ';', '=', '?', '@', ' ', '"', '\'', '#', 0 };
	const char escape_chars[] = { '&', '<', '>', 0 };
	const char *escape_seqnc[] = { "&amp;", "&lt;", "&gt;", 0 };
	int i;
	char *buffer, *location = NULL;

	*length = strlen(string);

	buffer = (char*)malloc(*length + 1);
	strcpy(buffer, string);

	for (i = 0; escape_chars[i] != 0; i++)
	{
		int offset = 0;
		while(location = strchr(buffer + offset, escape_chars[i]))
		{
			char *buffer2 = buffer;
			int new_length = strlen(buffer) + strlen(escape_seqnc[0]) + 1;

			offset = location - buffer + 1;
			buffer = (char *)malloc(new_length);

			memset(buffer, 0, new_length);
			strncpy(buffer, buffer2, offset - 1);
			sprintf(buffer, "%s%s%s", buffer, escape_seqnc[i], location + 1);

			free(buffer2);
		}
	}

	// twitter counts entities towards the 160 character limit, but not urlencoded strings
	*length = strlen(buffer);

	for (i = 0; encode_chars[i] != 0; i++)
	{
		int offset = 0;
		while (location = strchr(buffer + offset, encode_chars[i]))
		{
			char *buffer2 = buffer;
			int new_length = strlen(buffer2) + 4;
			
			offset = location - buffer + 1;
			buffer = (char *)malloc(new_length);

			memset(buffer, 0, new_length);
			strncpy(buffer, buffer2, offset - 1);
			sprintf(buffer, "%s%%%x%s", buffer, location[0], location + 1);

			free(buffer2);
		}
	}

	return buffer;
}

void CopyString(char **dest, char *src)
{
	if (*dest != NULL) free(*dest);
	*dest = malloc(strlen(src) + 1);
	strcpy(*dest, src);
}

int ReadOptions(char **username, char **password, char **target, char **url, int argc, char** argv)
{
	char optionsFile[MAX_PATH + 1];
	FILE* fp;
	int i;

	if (argv[1][0] == '-' && argv[1][1] == 'c')
		strncpy(optionsFile, argv[2], MAX_PATH);
	else
		strcpy(optionsFile, pszConfFile);

	fp = fopen(optionsFile, "rt");

	if (fp != NULL)
	{
		while (!feof(fp))
		{
			char line[4096];
			char *option, *value;

			fgets(line, 4096, fp);
			if(line[0] == 0) continue;

			option = strtok(line, " \t");
			if (option[0] == '#' || option[0] == '!')
				continue;
			
			value = strtok(NULL, " \t\r\n");
			if (stricmp(option, "username") == 0)
				CopyString(username, value);
			else if (stricmp(option, "password") == 0)
				CopyString(password, value);
			else if (stricmp(option, "target") == 0)
				CopyString(target, value);
			else if (stricmp(option, "url") == 0)
				CopyString(url, value);
		}
	}

	for (i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-') break;
		switch (argv[i][1])
		{
			case 'u':
			case 'U':
				CopyString(username, argv[++i]);
				break;
			case 'p':
			case 'P':
				CopyString(password, argv[++i]);
				break;
			case 't':
			case 'T':
				CopyString(target, argv[++i]);
				break;
			case 'r':
			case 'R':
				CopyString(url, argv[++i]);
				break;
		}
	}

	if (*target == NULL)
		CopyString(target, TARGET);

	if (*url == NULL)
		CopyString(url, URL);

	return i;

}

void LogMessage(char *message)
{
	static FILE *fp = (FILE *)-1;

	if (fp == (FILE *)-1)
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
/* Use WinMain on windows so it doesn't open a console dialog. */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#else
int main(int argc, char** argv)
{
#endif
	char *psz, *status=0;
	char *uname = NULL, *paswd = NULL, *target = NULL, *url = NULL;
	char sbuffer[4096], authstring[256];
#ifdef WIN32
	WSADATA wsad;
#endif
	SOCKET s;
	SOCKADDR_IN sai;
	HOSTENT *phe;
	int iBytes = 1;

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

	if (argc == 1)
	{
		LogMessage("You have to call twitcon with arguements, I mean come on, really?!");
		return 1;
	}

	iBytes = ReadOptions(&uname, &paswd, &target, &url, argc, argv);
	status = compact_arguments(iBytes, argc, argv);

	iBytes = 0;
	psz = url_encode(status, &iBytes);
	free(status);
	status = psz;

	if (iBytes > 160)
	{
		LogMessage("Status message is limited to 160 characters.  Twitter recommends 140.");
		return 1;
	}

	sprintf(sbuffer,"%s:%s",uname,paswd);
	base64_encode(sbuffer,authstring);

#ifdef WIN32
	if (WSAStartup(MAKEWORD(1,1),&wsad))
	{
		LogMessage("Error: Unable to start Winsock.");
		return 1;
	}
#endif

	// Resolve the target host name
	phe=gethostbyname(target);
	if (!phe)
	{
#ifdef WIN32
		WSACleanup();
#endif
		sprintf(sbuffer,"Error: Unable to resolve address for %s.\n",target);
		LogMessage(sbuffer);
		return 1;
	}

	// Initialize and fill the socket address structure.
	ZeroMemory(&sai,sizeof(SOCKADDR_IN));
	sai.sin_family=AF_INET;
	sai.sin_addr=*(IN_ADDR*)phe->h_addr_list[0];
	sai.sin_port=htons(80);

	// Create a socket for the connection.
	s=socket(AF_INET,SOCK_STREAM,PF_UNSPEC);
	if (s==INVALID_SOCKET)
	{
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
		sprintf(sbuffer,"Error: Unable to connect to %s (%s).\n",target,inet_ntoa(sai.sin_addr));
		LogMessage(sbuffer);
		return 1;
	}

		// Construct the HTTP GET request
	sprintf(sbuffer,"POST %s/statuses/update.xml", url);
	sprintf(strchr(sbuffer,0)," HTTP/1.1\nUser-Agent: TwitCon %s\nHost: %s\nContent-Type: application/x-www-form-urlencoded\nAuthorization: Basic %s\n",VERSION,target,authstring);

	sprintf(strchr(sbuffer, 0), "Content-Length: %i\n\nsource=twitcon&status=%s",strlen(status) + 22, status);
	free(status);

	// Send the HTTP GET request to the server
	if (send(s,sbuffer,strlen(sbuffer),0)==SOCKET_ERROR)
	{
		closesocket(s);
#ifdef WIN32
		WSACleanup();
#endif
		LogMessage("Error: Unable to send request to server.");
		return 1;
	}

	// Read the HTTP response from the server
	iBytes=recv(s,sbuffer,4096,0);
	if (iBytes==SOCKET_ERROR)
	{
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
	sbuffer[iBytes]=0;
	if (psz=strstr(sbuffer,"\r\n\r\n")) psz+=4;
	if (strstr(sbuffer, "<error>"))
	{
		LogMessage(sbuffer);
		return 1;
	}
	// Close the socket and cleanup Winsock
	closesocket(s);
#ifdef WIN32
	WSACleanup();
#endif

	// Return 0 to the OS, indicating no error
	return 0;
}