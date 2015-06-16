[Setup]
AppName=NewtonCam
AppVersion=0.4
DefaultDirName={pf}\NewtonCam

; Since no icons will be created in "{group}", we don't need the wizard
; to ask for a Start Menu folder name:

DisableProgramGroupPage=yes

OutputDir=.
OutputBaseFilename=NewtonCam_setup

[Components]
Name: "LIBS"; Description: "NewtonCam common libraries"; Types: full compact custom; Flags: fixed
Name: "Server"; Description: "NewtonCam server"; Types: full compact custom; Flags: fixed
Name: "GUI"; Description: "NewtonCam server GUI"; Types: full compact custom; Flags: fixed
Name: "Client"; Description: "NewtonCam command client"; Types: full compact custom; Flags: fixed
Name: "Docs"; Description: "NewtonCam user guide"; Types: full compact custom; Flags: fixed
Name: "QT"; Description: "QT libraries"; Types: full
Name: "CFITSIO"; Description: "CFITSIO library"; Types: full
Name: "MSVC"; Description: "Visual Studio 2013 redistributable libraries"; Types: full




[Files]
Source: "..\NewtonCam.build-x86\net_protocol\release\net_protocol.dll"; DestDir: "{app}"; Components: LIBS
Source: "..\NewtonCam.build-x86\serverGUI\release\serverGUI.dll"; DestDir: "{app}"; Components: LIBS
Source: "AndorSDK\atmcd32d.dll"; DestDir: "{app}"; Components: LIBS

Source: "..\NewtonCam.build-x86\camera\release\camera.dll"; DestDir: "{app}"; Components: Server
Source: "..\NewtonCam.build-x86\server\release\server.dll"; DestDir: "{app}"; Components: Server
Source: "..\NewtonCam.build-x86\NewtonCam\release\NewtonCam_server.exe"; DestDir: "{app}"; Components: Server
Source: "NewtonCam.ini"; DestDir: "{app}"; Components: Server

Source: "..\NewtonCam.build-x86\NewtonGUI\release\NewtonGUI.exe"; DestDir: "{app}"; Components: GUI

Source: "..\NewtonCam.build-x86\client\release\NewtonCam_client.exe"; DestDir: "{app}"; Components: Client

Source: "doc\NewtonCam.pdf"; DestDir: "{app}\doc"; Components: Docs

Source: "ext_libs\qwindows.dll"; DestDir: "{app}\platforms"; Components: QT
Source: "ext_libs\Qt5Core.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\Qt5Gui.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\Qt5Network.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\Qt5Widgets.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\icuin53.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\icuuc53.dll"; DestDir: "{app}"; Components: QT
Source: "ext_libs\icudt53.dll"; DestDir: "{app}"; Components: QT

Source: "cfitsio\cfitsio.dll"; DestDir: "{app}"; Components: CFITSIO

Source: "ext_libs\vcredist_x86.exe"; DestDir: "{app}"; Components: MSVC


[Run]
Filename: "{app}\vcredist_x86.exe"; Description: "Run instalation of Visual Studio 2013 redistributables"; Flags: postinstall runascurrentuser
