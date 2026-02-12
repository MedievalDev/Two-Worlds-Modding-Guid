#ifndef UNICODE
#define UNICODE
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h> //comctl32
#include <string.h>
#include "pathPrompt.h"

#define ID_BUT 103
#define ID_BUTYES 113
#define ID_BUTNO  114
#define ID_EXTR 200

int subRes = 0;
int waiting = 0;

#define FULLWIDTH 580
#define BORDERWIDTH 8
#define STATH 48
#define DYNH 64
#define BUTWIDTH 45
void constructUISub(HINSTANCE inst, HWND hwnd, PromptVar* vars){
	int xid = ID_EXTR;
	int yp = BORDERWIDTH;
	int xp = BORDERWIDTH;
	int FULH = STATH+DYNH*vars->count;
	HWND cel;
	int err;
	//CREATE BORDER
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		0, 0, FULLWIDTH+BORDERWIDTH*2, BORDERWIDTH, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		0, BORDERWIDTH, BORDERWIDTH, FULH, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		FULLWIDTH+BORDERWIDTH, BORDERWIDTH, BORDERWIDTH, FULH, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		0, FULH+BORDERWIDTH, FULLWIDTH+BORDERWIDTH*2, BORDERWIDTH, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	//MESSAGE
	cel = CreateWindowEx(0,L"STATIC",vars->promptMessage,
		WS_VISIBLE | WS_CHILD,
		xp, yp, FULLWIDTH, 18, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	yp+=18;
	//ENTRIES
	for(int i = 0; i<vars->count; i++){
		//SEPARATOR
		cel = CreateWindowEx(0,L"STATIC",NULL,
			WS_VISIBLE | WS_CHILD,
			xp, yp, FULLWIDTH, 4, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		cel = CreateWindowEx(0,L"STATIC",NULL,
			WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
			xp, yp+4, FULLWIDTH, 2, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		cel = CreateWindowEx(0,L"STATIC",NULL,
			WS_VISIBLE | WS_CHILD,
			xp, yp+6, FULLWIDTH, 4, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		//FILE INPUT AND OUTPUT
		cel = CreateWindowEx(0,L"STATIC",vars->inputs[i],
			WS_VISIBLE | WS_CHILD | SS_PATHELLIPSIS,
			xp, yp+10, FULLWIDTH, 18, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		cel = CreateWindowEx(0,L"STATIC",L"🡇",//➔
			WS_VISIBLE | WS_CHILD | SS_CENTER,
			xp, yp+28, FULLWIDTH, 18, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		cel = CreateWindowEx(0,L"STATIC",vars->outputs[i],
			WS_VISIBLE | WS_CHILD | SS_PATHELLIPSIS,
			xp, yp+46, FULLWIDTH, 18, hwnd,
			(HMENU)xid++,
			inst,NULL
		);if(cel==NULL)err++;
		yp+=DYNH;
	}
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		xp, yp, FULLWIDTH, 4, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
		xp, yp+4, FULLWIDTH, 2, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		xp, yp+6, FULLWIDTH, 4, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	yp+=10;
	//BUTTONS
	int center = FULLWIDTH/2;
	int butA = center-BORDERWIDTH-BUTWIDTH;
	int butB = center+BORDERWIDTH;
	//TODO YES NO STATICS
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		xp, yp, butA, 20, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		xp+center-BORDERWIDTH, yp, BORDERWIDTH*2, 20, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",NULL,
		WS_VISIBLE | WS_CHILD,
		xp+butB+BUTWIDTH, yp, FULLWIDTH-butB-BUTWIDTH, 20, hwnd,
		(HMENU)xid++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"BUTTON",L"Yes",
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		xp+butA, yp, BUTWIDTH, 20, hwnd,
		(HMENU)ID_BUTYES,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"BUTTON",L"No",
		WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		xp+butB, yp, BUTWIDTH, 20, hwnd,
		(HMENU)ID_BUTNO,
		inst,NULL
	);if(cel==NULL)err++;
}

LRESULT CALLBACK SubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
		case WM_CREATE:
			{
				PromptVar* var = ((CREATESTRUCT*)lParam)->lpCreateParams;
				HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
				constructUISub(inst, hwnd, var);
			}
			break;
		case WM_COMMAND:
			{
				int ID = LOWORD(wParam);
				switch(ID){
					case ID_BUTYES:
						subRes = 1;
						waiting = 0;
						break;
					case ID_BUTNO:
						subRes = 0;
						waiting = 0;
						break;
				}
			}
			break;
		case WM_DESTROY:
			waiting = 0;
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int promptFileAction(HWND parent, PromptVar* vars){
	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
	const wchar_t CLASS_NAME[]  = L"TestSub";
	int bwx = GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	int bwy = GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFIXEDFRAME)*2;
	int width = bwx+FULLWIDTH+BORDERWIDTH*2;
	int height = bwy+STATH+DYNH*vars->count+BORDERWIDTH*2;
	
	WNDCLASS wc = {0};
	wc.lpfnWndProc   = SubProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc); //TODO should not register and unregister every time, but should work at least?
	HWND sub =  CreateWindowEx(
		0,              // Optional window styles.
		CLASS_NAME,                     // Window class
		vars->promptTitle,      // Window text
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU ,            
		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		parent,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		vars       // Additional application data
		);
	subRes = 0;
	waiting = 1;
	
	ShowWindow(sub, 1);
	EnableWindow(parent, FALSE);
	MSG msg = { };
	while(waiting){
		if(GetMessage(&msg, NULL, 0, 0)){
			if(!IsDialogMessage(sub, &msg)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}else{
			waiting = 0;
			subRes = 0;
			PostQuitMessage(msg.wParam);
			break;
		}
	}
	EnableWindow(parent, TRUE);
	DestroyWindow(sub);
	UnregisterClass(CLASS_NAME, hInstance);
	return subRes;
}
