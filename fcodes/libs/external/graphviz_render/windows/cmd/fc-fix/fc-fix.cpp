#include "fc-fix.h"
#include <fstream>
#include <iostream>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <windows.h>

using namespace std;

#pragma hdrstop

/*
        To have fontconfig work right , it requires some config files located at
   \\etc\\fonts\\ of
        where fc functions are called .We also need to insert windows font
   directory to fc config file
*/

int fix_fontconfig(TCHAR *baseDir) {
  char B[1024];
  string FontFolder = "";
  string fcConfigFile = "";
  string BB = "";

  // retrieve font directory
  ::SHGetSpecialFolderPath(NULL, B, CSIDL_FONTS, 0);

  // check if directory already exists
  // set fc config file
  fcConfigFile.append(baseDir);
  fcConfigFile.append("\\etc\\fonts\\fonts.conf");

  cout << fcConfigFile;

  ifstream file(fcConfigFile.c_str(), ios::in);
  try {
    if (file) {
      while (!file.eof()) {
        BB.push_back(file.get());
      }
      file.close();

      //		 cout << BB;

      //		 cout << "\n" <<"position of found text:"
      //<<BB.find("#WINDOWSFONTDIR#",0 );

      size_t a = BB.find("#WINDOWSFONTDIR#", 0);
      if (a != string::npos)
        BB.replace(a, strlen(B), B);

      std::ofstream out(fcConfigFile.c_str());
      out << BB << '\n';
      out.close();
    }
  } catch (...) {
    ;
  }

  return 0;
}

UINT __stdcall fc_fix(MSIHANDLE hInstall) {
  char vbuff[500] = {0};
  DWORD vlen = 500;

  UINT gp = MsiGetPropertyA(hInstall, "CustomActionData", vbuff, &vlen);
  fix_fontconfig(vbuff);

  return 0;
}
