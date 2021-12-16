// BahRecorder.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BahRecorder.h"

#define MAX_LOADSTRING 100
#define SPECWIDTH 368	// display width
#define SPECHEIGHT 127	// height (changing requires palette adjustments too)
DWORD timer = 0;
HDC specdc = 0;
HBITMAP specbmp = 0;
BYTE *specbuf;

int specmode = 0, specpos = 0;
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL InitDevice();
#define FREQ 44100
#define CHANS 2
#define BUFSTEP 200000
char *recbuf = nullptr;
DWORD reclen;
HRECORD rchan = 0;
HSTREAM chan = 0;
void StartRecording();
void StopRecording();
void WriteToDisk();
void OpenFile();
void Play();
HWND hWnd;
HWND bRecord;
HWND bStop;
HWND bPlay;
HWND bPause;
HWND hwndTrackBar;
HWND lTime;
HWND lFileName;
int Time;
int recodingTime;
char bass_using = 0;
QWORD len;
QWORD time;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, (LPSTR)szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BAHRECORDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BAHRECORDER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

	HBRUSH hb = ::CreateSolidBrush(RGB(240, 240, 240));
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(MAIN_ICO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = hb;
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_BAHRECORDER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(SMALL_ICO));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable
	INITCOMMONCONTROLSEX init = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
	InitCommonControlsEx(&init);
	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, 0, 400, 270, nullptr, nullptr, hInstance, nullptr);
	bRecord = CreateWindow("BUTTON", "Record", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 10, 70, 30, hWnd, (HMENU)IDB_RECORD, hInstance, 0);
	bPause = CreateWindow("BUTTON", "Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 106, 10, 70, 30, hWnd, (HMENU)IDB_PAUSE, hInstance, 0);
	bStop = CreateWindow("BUTTON", "Stop", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 202, 10, 70, 30, hWnd, (HMENU)IDB_STOP, hInstance, 0);
	bPlay = CreateWindow("BUTTON", "Play", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 300, 10, 70, 30, hWnd, (HMENU)IDB_PLAY, hInstance, 0);
	hwndTrackBar = CreateWindowEx(0, TRACKBAR_CLASS, "Trackbar Control", WS_CHILD | WS_VISIBLE | TBS_ENABLESELRANGE, 10, 50, 320, 30, hWnd, (HMENU)IDT_TRACKBAR, hInst, NULL);
	lTime = CreateWindow("static", "0:00:00", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 327, 53, 43, 16, hWnd, (HMENU)IDL_TIME, hInstance, NULL);
	HFONT font = CreateFont(16, 0, 0, 0, FW_REGULAR, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_SWISS, "MS Shell Dlg");

	SendMessage(bRecord, WM_SETFONT, WPARAM(font), 0);
	SendMessage(bStop, WM_SETFONT, WPARAM(font), 0);
	SendMessage(bPlay, WM_SETFONT, WPARAM(font), 0);
	SendMessage(bPause, WM_SETFONT, WPARAM(font), 0);
	SendMessage(lTime, WM_SETFONT, WPARAM(font), 0);
	SendMessage(lFileName, WM_SETFONT, WPARAM(font), 0);
	EnableWindow(bStop, false);
	EnableWindow(bPlay, false);
	EnableWindow(bPause, false);

	if ((!BASS_RecordInit(-1)))
	{
		BASS_RecordFree();
		BASS_Free();
		MessageBox(NULL, "Error", "Recoding devise don't initialized!", 0);
	}

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
void CALLBACK UpdateSpectrum(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	HDC dc;
	int x, y, y1;

	if (specmode == 3) { // waveform
		int c;
		float *buf;
		BASS_CHANNELINFO ci;
		memset(specbuf, 0, SPECWIDTH*SPECHEIGHT);
		BASS_ChannelGetInfo(chan, &ci); // get number of channels
		buf = (float *) alloca(ci.chans*SPECWIDTH * sizeof(float));
		BASS_ChannelGetData(chan, buf, (ci.chans*SPECWIDTH * sizeof(float)) | BASS_DATA_FLOAT); // get the sample data (floating-point to avoid 8 & 16 bit processing)
		for (c = 0; c < (int)ci.chans; c++) {
			for (x = 0; x<SPECWIDTH; x++) {
				int v = (1 - buf[x*ci.chans + c])*SPECHEIGHT / 2; // invert and scale to fit display
				if (v<0) v = 0;
				else if (v >= SPECHEIGHT) v = SPECHEIGHT - 1;
				if (!x) y = v;
				do { // draw line from previous sample...
					if (y<v) y++;
					else if (y>v) y--;
					specbuf[y*SPECWIDTH + x] = c & 1 ? 127 : 1; // left=green, right=red
				} while (y != v);
			}
		}
	}
	else {
		float fft[1024];
		BASS_ChannelGetData(chan, fft, BASS_DATA_FFT2048); // get the FFT data

		if (!specmode) { // "normal" FFT
			memset(specbuf, 0, SPECWIDTH*SPECHEIGHT);
			for (x = 0; x<SPECWIDTH / 2; x++) {
				y = (int)(sqrt(fft[x + 1]) * 3 * SPECHEIGHT - 4); // scale it (sqrt to make low values more visible)

				if (y>SPECHEIGHT) y = SPECHEIGHT; // cap it
				if (x && (y1 = (y + y1) / 2)) // interpolate from previous to make the display smoother
					while (--y1 >= 0) specbuf[y1*SPECWIDTH + x * 2 - 1] = y1 + 1;
				y1 = y;
				while (--y >= 0) specbuf[y/(SPECWIDTH*SPECWIDTH) + x * 2] = (y + 1) % 256; // draw level
			}
		}
		else if (specmode == 1) { // logarithmic, combine bins
			int b0 = 0;
			memset(specbuf, 0, SPECWIDTH*SPECHEIGHT);
#define BANDS 28
			for (x = 0; x<BANDS; x++) {
				float peak = 0;
				int b1 = pow(2, x*10.0 / (BANDS - 1));
				if (b1 <= b0) b1 = b0 + 1; // make sure it uses at least 1 FFT bin
				if (b1>1023) b1 = 1023;
				for (; b0<b1; b0++)
					if (peak<fft[1 + b0]) peak = fft[1 + b0];
				y = sqrt(peak) * 3 * SPECHEIGHT - 4; // scale it (sqrt to make low values more visible)
				if (y>SPECHEIGHT) y = SPECHEIGHT; // cap it
				while (--y >= 0)
					memset(specbuf + y*SPECWIDTH + x*(SPECWIDTH / BANDS), y + 1, SPECWIDTH / BANDS - 2); // draw bar
			}
		}
		else { // "3D"
			for (x = 0; x<SPECHEIGHT; x++) {
				y = sqrt(fft[x + 1]) * 3 * 127; // scale it (sqrt to make low values more visible)
				if (y>127) y = 127; // cap it
				specbuf[x*SPECWIDTH + specpos] = 128 + y; // plot it
			}
			// move marker onto next position
			specpos = (specpos + 1) % SPECWIDTH;
			for (x = 0; x<SPECHEIGHT; x++) specbuf[x*SPECWIDTH + specpos] = 255;
		}
	}

	// update the display
	dc = GetDC(hWnd);
	BitBlt(dc, 10, 80, SPECWIDTH, SPECHEIGHT, specdc, 0, 0, SRCCOPY);
	ReleaseDC(hWnd, dc);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_OPEN:
				EnableWindow(bStop, false);
				EnableWindow(bPlay, true);
				EnableWindow(bPause, false);
				EnableWindow(bRecord, true);
				OpenFile();
				break;
			case IDM_SAVE:
				WriteToDisk();
				break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDB_PAUSE:
				if (bass_using == 0)
				{
					if (GetWindowTextLength((HWND)bPause) == 5) // when Pause pressed
					{
						SetWindowText((HWND)bPause, "Resume");
						KillTimer(hWnd, ID_TIMER);
						BASS_ChannelPause(rchan);
					}
					else  //when Resume pressed
					{
						BASS_ChannelPlay(rchan, false);
						SetTimer(hWnd, ID_TIMER, 10, (TIMERPROC)NULL);
						SetWindowText((HWND)bPause, "Pause");
					}
				}
				else
				{
					if (GetWindowTextLength((HWND)bPause) == 5) // when Pause pressed
					{
						SetWindowText((HWND)bPause, "Resume");
						KillTimer(hWnd, ID_TIMER);
						BASS_ChannelPause(chan);
					}
					else  //when Resume pressed
					{
						BASS_ChannelPlay(chan, false);
						SetTimer(hWnd, ID_TIMER, 10, (TIMERPROC)NULL);
						SetWindowText((HWND)bPause, "Pause");
					}
				}
				break;
			case IDB_RECORD: 
				if (InitDevice())
				{
					bass_using = 0;
					EnableWindow(bStop, true);
					EnableWindow(bPlay, false);
					EnableWindow(bPause, true);
					EnableWindow(bRecord, false);
					StartRecording();
					recodingTime = 0;
					SetTimer(hWnd, ID_TIMER, 10, (TIMERPROC)NULL);
					SetWindowText(lFileName, "File name");
				}
				break;
			case IDB_STOP:
				EnableWindow(bStop, false);
				EnableWindow(bPlay, true);
				EnableWindow(bPause, false);
				EnableWindow(bRecord, true);
				SendMessage(hwndTrackBar, TBM_SETPOS, (WPARAM)TRUE, 0);
				if (GetWindowTextLength((HWND)bPause) == 6) // when Pause pressed
				{
					SetWindowText((HWND)bPause, "Pause");
				}
				if (bass_using == 0)
				{
					StopRecording();
					KillTimer(hWnd, ID_TIMER);
				}
				else
				{
					BASS_ChannelStop(chan);
					KillTimer(hWnd, ID_TIMER);
				}
				StopRecording();
				break;
			case IDB_PLAY:
				bass_using = 1;
				EnableWindow(bStop, true);
				EnableWindow(bPlay, false);
				EnableWindow(bPause, true);
				EnableWindow(bRecord, false);
				Play();
				Time = 0;
				SetTimer(hWnd, ID_TIMER, 10, (TIMERPROC)NULL);
				if (bass_using == 0)
				{
					if (!BASS_ChannelIsActive(rchan))
						KillTimer(hWnd, ID_TIMER);
				}
				else
				{
					if (!BASS_ChannelIsActive(chan))
						KillTimer(hWnd, ID_TIMER);
				}
				break;
            }
        }
        break;
	case WM_TIMER:
		switch (wParam)
		{
		case ID_TIMER:
			recodingTime++;
			if (bass_using == 1)
			{
				time = BASS_ChannelGetPosition(chan, BASS_POS_BYTE);
				Time = (int)BASS_ChannelBytes2Seconds(chan, time);
				if (BASS_ChannelBytes2Seconds(chan, len) - BASS_ChannelBytes2Seconds(chan, time) <= 0.1)
					BASS_ChannelStop(chan);
				SendMessage(hwndTrackBar, TBM_SETPOS, (WPARAM)TRUE, (int)((double)time / len * 100));	
				if (!BASS_ChannelIsActive(chan))
				{
					SendMessage(hwndTrackBar, TBM_SETPOS, (WPARAM)TRUE, 0);
					KillTimer(hWnd, ID_TIMER);
					EnableWindow(bRecord, true);
					EnableWindow(bPlay, true);
					EnableWindow(bStop, false);
					EnableWindow(bPause, false);
				}
			}
			else
			{
				Time = recodingTime / 100;
			}
			char formTime[8];
			formTime[0] = char(48 + Time / 3600);
			formTime[1] = ':';
			formTime[2] = char(48 + Time % 3600 / 60 / 10);
			formTime[3] = char(48 + Time % 3600 / 60 % 10);
			formTime[4] = ':';
			formTime[5] = char(48 + Time % 3600 % 60 / 10);
			formTime[6] = char(48 + Time % 3600 % 60 % 10);
			formTime[7] = '\0';
			if (recodingTime % 100 == 0)
				SetWindowText(lTime, formTime);
			break;
		}
	case WM_HSCROLL:
		{
			if (LOWORD(wParam) != TB_THUMBPOSITION)
			{
				break;
			}
			int trackBarPosition = SendDlgItemMessage(hWnd, IDT_TRACKBAR, TBM_GETPOS, 0,0);
			QWORD position = (double)trackBarPosition / 100 * len;
			BASS_ChannelSetPosition(chan, position, BASS_POS_BYTE);
		}
		break;
	case WM_PAINT:
		if (GetUpdateRect(hWnd, 0, 0)) {
			PAINTSTRUCT p;
			HDC dc;
			if (!(dc = BeginPaint(hWnd, &p))) return 0;
			BitBlt(dc, 10, 80, SPECWIDTH, SPECHEIGHT, specdc, 0, 0, SRCCOPY);
			EndPaint(hWnd, &p);
		}
		break;
	case WM_LBUTTONUP:
		specmode = (specmode + 1) % 4; // change spectrum mode
		memset(specbuf, 0, SPECWIDTH*SPECHEIGHT);	// clear display
		break;
    case WM_DESTROY:
		exit(EXIT_SUCCESS);
		
		// if (specbmp) DeleteObject(specbmp);
		// if (specdc) DeleteDC(specdc);
		// BASS_RecordFree();
		// BASS_Free();
		// PostQuitMessage(0);
		// if (timer) timeKillEvent(timer);
		
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL CALLBACK RecordingCallback(HRECORD handle, const void *buffer, DWORD length, void *user)
{
	if ((reclen%BUFSTEP) + length >= BUFSTEP) {
		recbuf = (char*)realloc(recbuf, ((reclen + length) / BUFSTEP + 1)*BUFSTEP);
		if (!recbuf) 
		{
			rchan = 0;
			MessageBox(NULL, "Error", "Out of memmory!", 0);
			return false;
		}
	}
	memcpy(recbuf + reclen, buffer, length);
	reclen += length;
	return true;
}

BOOL InitDevice()
{
	BASS_RecordFree();
	if (!BASS_RecordInit(-1)) {
		MessageBox(NULL, "Error", "Recording devise don't initialised!", 0);
		return false;
	}
	else
		return true;
}

void StartRecording()
{
	WAVEFORMATEX *wf;
	if (recbuf)
	{
		BASS_StreamFree(chan);
		chan = 0;
		free(recbuf);
		recbuf = nullptr;
		BASS_Free();
	}
	recbuf = (char*)malloc(BUFSTEP);
	reclen = 44;
	memcpy(recbuf, "RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20); 
	memcpy(recbuf + 36, "data\0\0\0\0", 8);
	wf = (WAVEFORMATEX*)(recbuf + 20);
	wf->wFormatTag = 1;  //Specifies the waveform audio format type
	wf->nChannels = CHANS;  //Specifies the number of channels of audio data.For monophonic audio, set this member to 1. For stereo, set this member to 2
	wf->wBitsPerSample = 16;  //Specifies the number of bits per sample for the format type specified by wFormatTag. If wFormatTag - wBitsPerSample = 8 or 16
	wf->nSamplesPerSec = FREQ;  //Specifies the sample frequency at which each channel should be played or recorded(kHz)
	wf->nBlockAlign = wf->nChannels*wf->wBitsPerSample / 8;  //Specifies the block alignment in bytes. The block alignment is the size of the minimum atomic unit of data for the wFormatTag format type
	wf->nAvgBytesPerSec = wf->nSamplesPerSec*wf->nBlockAlign;  //Specifies the required average data transfer rate in bytes per second
	rchan = BASS_RecordStart(FREQ, CHANS, 0, RecordingCallback, nullptr);
	if (!rchan) 
	{
		free(recbuf);
		MessageBox(NULL, "Recoding hash't done!", "Error", 0);
		recbuf = 0;
	}
}

void StopRecording()
{
	BASS_ChannelStop(rchan);
	rchan = 0;
	*(DWORD*)(recbuf + 4) = reclen - 8; 
	*(DWORD*)(recbuf + 40) = reclen - 44;
}

void Play()
{
	FILE *fp;
	char szTempName[MAX_PATH];
	tmpnam_s(szTempName, MAX_PATH);
	fopen_s(&fp,szTempName, "wb");
	
	fwrite(recbuf, reclen, 1, fp);
	fclose(fp);
	BASS_Init(-1, 44100, BASS_DEVICE_3D, 0, NULL);
	chan = BASS_StreamCreateFile(FALSE, szTempName, 0, 0, 0);
	len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);
	BASS_ChannelPlay(chan, TRUE);
	{ // create bitmap to draw spectrum in (8 bit for easy updating)
		BYTE data[2000] = { 0 };
		BITMAPINFOHEADER *bh = (BITMAPINFOHEADER*)data;
		RGBQUAD *pal = (RGBQUAD*)(data + sizeof(*bh));
		int a;
		bh->biSize = sizeof(*bh);
		bh->biWidth = SPECWIDTH;
		bh->biHeight = SPECHEIGHT; // upside down (line 0=bottom)
		bh->biPlanes = 1;
		bh->biBitCount = 8;
		bh->biClrUsed = bh->biClrImportant = 256;
		// setup palette
		for (a = 1; a<128; a++) {
			pal[a].rgbGreen = 256 - 2 * a;
			pal[a].rgbRed = 2 * a;
		}
		for (a = 0; a<32; a++) {
			pal[128 + a].rgbBlue = 8 * a;
			pal[128 + 32 + a].rgbBlue = 255;
			pal[128 + 32 + a].rgbRed = 8 * a;
			pal[128 + 64 + a].rgbRed = 255;
			pal[128 + 64 + a].rgbBlue = 8 * (31 - a);
			pal[128 + 64 + a].rgbGreen = 8 * a;
			pal[128 + 96 + a].rgbRed = 255;
			pal[128 + 96 + a].rgbGreen = 255;
			pal[128 + 96 + a].rgbBlue = 8 * a;
		}
		// create the bitmap
		specbmp = CreateDIBSection(0, (BITMAPINFO*)bh, DIB_RGB_COLORS, (void**)&specbuf, NULL, 0);
		specdc = CreateCompatibleDC(0);
		SelectObject(specdc, specbmp);
	}
	// start update timer (40hz)
	timer = timeSetEvent(25, 25, (LPTIMECALLBACK)&UpdateSpectrum, 0, TIME_PERIODIC);
	// remove(szTempName);
}

char* GetFileName(bool save) {
	static OPENFILENAME ofn;
	static char fullpath[255], filename[256], dir[256];
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = "WAV (*.wav)\0*.wav\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fullpath;
	ofn.nMaxFile = sizeof(fullpath);
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = sizeof(filename);
	ofn.lpstrInitialDir = dir;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER;
	if (save) {
		ofn.lpstrTitle = "Save audio as...";
		GetSaveFileName(&ofn);
	}
	else {
		ofn.lpstrTitle = "Open audio";
		GetOpenFileName(&ofn);
	}
	return fullpath;
}

void WriteToDisk()
{
	FILE *fp;
	char* file = GetFileName(true);
	if (!strchr(file,'.'))
	{
		strcat(file, ".wav");
	}
	fp = fopen(file, "wb");
	fwrite(recbuf, reclen, 1, fp);
	fclose(fp);
	PathStripPath(file);
	SetWindowText(lFileName, file);
}

void OpenFile()
{
	FILE * fp;
	free(recbuf);
	char* file = GetFileName(false);
	fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	reclen = ftell(fp);
	rewind(fp);
	recbuf = (char*)malloc(sizeof(char)*reclen);
	fread(recbuf, 1, reclen, fp);
	fclose(fp);
	PathStripPath(file);
	SetWindowText(lFileName, file);
	SetWindowText(lTime, "0:00:00");
}