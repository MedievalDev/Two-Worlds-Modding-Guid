#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <shlwapi.h> //Shlwapi
#include <direct.h>
#include <commctrl.h> //comctl32
#include <shellapi.h> //Shell32
#include <winnetwk.h> //required for shlobj.h to work
#include <shlobj.h> // Shell32
#include <Objbase.h> // Ole32
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wdio.h"
#include "pathPrompt.h"

//UI element IDs that get disabled when working
#define ID_STATE_FIRST 103
#define IDC_EDIT_INPUT      103
#define IDC_EDIT_OUTPUT     104
#define ID_BUTTON_PACK      105
#define ID_BUTTON_UNPACK    106
#define ID_CHECK_BULK       107
#define ID_CHECK_OVERWRITE  108
#define ID_CHECK_PRESERVE   109
#define ID_BUTTON_BROWSE_I  110
#define ID_BUTTON_BROWSE_O  111
#define ID_STATE_LAST 111
//UI elements that stay active as normal
#define IDC_PROGRESS_FILES  120
#define IDL_EDIT_INPUT      121
#define IDL_EDIT_OUTPUT     122
#define IDL_PROGRESS_FILES  123

#define ID_STATIC_PAD       130

#define WM_ACTDONE          WM_APP+0

#define L1W  100
#define L1W2 30
#define L1H  26
#define L1HH 13
#define LBORD 8

#define L2Wi L1W*5
#define L2W L2Wi+LBORD*2
#define L2H L1H*5+L1HH*3+LBORD*2

typedef struct _WStringArray{
	wchar_t** array;
	size_t cur;
	size_t max;
} WStringArray;
void WStringArray_free(WStringArray* arr){
	for(int i = 0; i<arr->cur; i++){
		free(arr->array[i]);
	}
	free(arr->array);
	free(arr);
}
WStringArray* WStringArray_Alloc(){
	WStringArray* arr = malloc(sizeof(WStringArray));
	arr->cur = 0;
	arr->max = 5;
	arr->array = malloc(sizeof(wchar_t*) * arr->max);
	return arr;
}
void WStringArray_Add(WStringArray* arr, wchar_t* string){
	if(arr->cur>=arr->max){
		arr->max+=5;
		arr->array = realloc(arr->array, sizeof(wchar_t*) * arr->max);
	}
	arr->array[arr->cur]=_wcsdup(string);
	arr->cur+=1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int promptIOAcceptA(wchar_t **sources, wchar_t**destinations, size_t count, int action);

wchar_t* WQM_PACK = L"Appliation is still packing.\nCancel and quit anyway?";
wchar_t* WQM_UNPACK = L"Appliation is still unpacking.\nCancel and quit anyway?";
wchar_t* WCM_PACK = L"Packing Complete";
wchar_t* WCM_UNPACK = L"Unpacking Complete";
//volatile globals might be accessed from multiple threads
volatile HWND mainWindow = NULL;
volatile HANDLE workThread = NULL;
volatile wchar_t* workQuitMessage = NULL;
volatile wchar_t* workCompleteMessage = NULL;
volatile int cancelWork = 0;
volatile int consoleMode = 0;
volatile int actionComplete = 0;
//used to transfer info to worker thread as it initiates.
volatile wchar_t* wtr_input = NULL;
volatile wchar_t* wtr_output = NULL;
volatile int wtr_confirmed = 0;
volatile int wtr_action = 0;
volatile int wtr_bulk = 0; //TODO do not implemented yet
volatile int wtr_overwrite = 0;
int wtr_up_overwrite = 0;
volatile int wtr_preserve = 0;
volatile int wtr_forceSingle = 0;
volatile int wtr_errorCount = 0;

#define ACT_PACK 5
#define ACT_UNPACK 4
#define ACT_AUTO 3
//used internally for single items
int wtr_progmax = 0;
int wtr_progcur = 0;
void _setProgRange(int maxRange){
	if(consoleMode!=1){
		HWND hwndPB = GetDlgItem(mainWindow, IDC_PROGRESS_FILES);
		PostMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, maxRange));
		PostMessage(hwndPB, PBM_SETSTEP, (WPARAM)1, 0);
	}
}
void _updateProg(wchar_t* outText){
	if(consoleMode!=1){
		HWND hwndPB = GetDlgItem(mainWindow, IDC_PROGRESS_FILES);
		HWND hwndPL = GetDlgItem(mainWindow, IDL_PROGRESS_FILES);
		PostMessage(hwndPB, PBM_STEPIT, 0, 0); 
		SetWindowText(hwndPL, outText);
	}
}
int promptOverwrite(){
	if(wtr_confirmed)return 0;//NO PROMPT
	if(consoleMode!=1){
		int res = MessageBox(mainWindow, L"Overwrite existing files?", L"File Exists", MB_YESNOCANCEL | MB_ICONWARNING);
		if(res==IDCANCEL)return 2;
		if(res==IDYES)return 1;
		if(res==IDNO)return 0;
	}
	return 0;//if console mode, no prompt
}
int makeDirs(char *path){
	char subPath[MAX_PATH];
	strcpy(subPath, path);
	char search = '\\';
	int index = 0;
	char* sh = subPath;
	sh = strchr(sh + 1, search);
	do{
		index = sh-subPath;
		char tmp = subPath[index];
		subPath[index]= 0;
		if(!PathIsDirectoryA(subPath)){
			_mkdir(subPath);//TODO error check
		}
		subPath[index]=tmp;
		sh = strchr(sh + 1, search);
	}while(sh!=NULL);
	return 0;
}
int makeDirsW(wchar_t* path){
	size_t p_mlen = wcstombs(NULL, path, 0)+4;
	char *chrpath = malloc(p_mlen);
	size_t ret = wcstombs(chrpath, path, p_mlen);
	makeDirs(chrpath);//TODO Error check
	free(chrpath);
}
void _setState(wchar_t* outText){
	if(consoleMode!=1){
		HWND hwndPL = GetDlgItem(mainWindow, IDL_PROGRESS_FILES);
		SetWindowText(hwndPL, outText);
	}
}
BOOL _setErr(wchar_t* outText){
	_setState(outText);
	return FALSE;
}
void _simProg(){
	int maxrange = 10;
	int curindex = 0;
	
	_setProgRange(maxrange);
	
	while(curindex<maxrange && !cancelWork){curindex++;
		Sleep(500);
		_updateProg(L"Test...");
	}
}
int _listFiles(wchar_t *path, wchar_t *localpath, WStringArray* pathList){
	int filecount = 0;
	wchar_t searchPath[MAX_PATH*2]={0};
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	swprintf_s(searchPath, MAX_PATH*2, L"%s%s", path, L"\\*");
	//TODO find files and fill pathlist
	//wprintf(L"Searching Path: %s\n", searchPath);
	hFind = FindFirstFile(searchPath, &FindFileData);
  if(hFind != INVALID_HANDLE_VALUE){
		do{
			if(!_wcsicmp(FindFileData.cFileName, L".")
				|| !_wcsicmp(FindFileData.cFileName, L"..")
				|| !_wcsicmp(FindFileData.cFileName, L"FILETIME")
				|| !_wcsicmp(FindFileData.cFileName, L"GUID"))continue;
			wchar_t fullPath[MAX_PATH*2]={0};
			swprintf_s(fullPath, MAX_PATH*2, L"%s\\%s", path, FindFileData.cFileName);
			wchar_t loPath[MAX_PATH*2]={0};
			swprintf_s(loPath, MAX_PATH*2, L"%s\\%s", localpath, FindFileData.cFileName);
			//TODO use path combine functions instead? ^
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				filecount += _listFiles(fullPath, loPath, pathList);
			}else{
				//wprintf(L"- %s\n", FindFileData.cFileName);
				//wprintf(L"- %s\n", fullPath);
				//wprintf(L"- %s\n", loPath);
				WStringArray_Add(pathList, loPath);
				filecount++;
			}
		}while(FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return filecount;
}
BOOL _validate(wchar_t *inputPath, wchar_t **outputPath, int action){
	if(!action)return FALSE;
	int inplen = wcslen(inputPath);
	if(!inplen)return _setErr(L"Input Empty");
	if(!wcslen(*outputPath)){
		inplen+=10;
		*outputPath = realloc(*outputPath, inplen*2);
		wcsncpy(*outputPath, inputPath, inplen);
	}
	//TODO more descriptive errors, use _setErr(L"");
	if(action==ACT_PACK){
		if(!PathIsDirectory(inputPath))return _setErr(L"Input is not an existing folder");
		if(PathFileExists(*outputPath)){
			//output likely same as input, target is same directory, append .wd and retry tests
			if(PathIsDirectory(*outputPath)){
				int olen = wcslen(*outputPath);
				olen+=10;
				*outputPath = realloc(*outputPath, olen*2);
				wcscat(*outputPath, L".wd");
			}
			if(PathFileExists(*outputPath)){
				if(PathIsDirectory(*outputPath))return _setErr(L"Cannot open folder as file");
			}
		}
	}else if(action==ACT_UNPACK){
		if(PathIsDirectory(inputPath))return _setErr(L"Cannot unpack a folder");
		if(!PathFileExists(inputPath))return _setErr(L"File must exist");
		if(!PathIsDirectory(*outputPath) && PathFileExists(*outputPath)){
			//output likely same as input, target is same directory, remove .wd and retry tests
			int outlen = wcslen(*outputPath);
			if(outlen>3){
				wchar_t *ext = PathFindExtensionW(*outputPath);
				if(!_wcsicmp(ext, L".wd")){
					ext[0]=0;//should set null terminator to remove the .wd of the path
				}else{
					return _setErr(L"File must be a .wd archive");
				}
			}
		}
	}
	return TRUE;//Success
}

INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData){
	//TODO Does not reliably scroll into view, seems excessively hard to fix at the moment
	if (uMsg==BFFM_INITIALIZED){
		wchar_t* pstr = (wchar_t*)pData;
		int plen = wcslen(pstr);
		int exptarg = FALSE;
		if(!plen){
			int reqlen = GetCurrentDirectoryW(MAX_PATH*2, pstr);
			if(reqlen>=MAX_PATH*2){//TODO error excessive path length
				pstr[0]=0;//ensure nulled
			}else{
				exptarg = TRUE;
			}
		}
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pstr);
		if(exptarg){
			SendMessage(hwnd, BFFM_SETEXPANDED, TRUE, (LPARAM)pstr);
		}
		//scroll into view
		int success = FALSE;
		HWND hwndFolderCtrl = NULL;
		HWND hwndTV = NULL;
		LRESULT item = 0;
		if(hwnd){
			hwndFolderCtrl = GetDlgItem(hwnd, 0);//_dlgItemBrowseControl
			if(hwndFolderCtrl){
				//TODO replace hard coded fetch with enum/scan for correct item
				hwndTV = GetDlgItem(hwndFolderCtrl, 100);//_dlgItemTreeView
				if(hwndTV){
					//would reload state, but cannot find out if possible
					item = SendMessage(hwndTV, TVM_GETNEXTITEM, TVGN_CARET, 0);
					if(item){
						if(exptarg){
							PostMessage(hwndTV, TVM_SELECTITEM, TVGN_FIRSTVISIBLE, (LPARAM)item);
						}else{
							PostMessage(hwndTV, TVM_ENSUREVISIBLE, 0, (LPARAM)item);
						}
						success = TRUE;
		}}}}
		if(!success){
			//MessageBox(NULL, L"Could not scroll into view", L"ERROR", 0);
			//if(!hwndFolderCtrl)MessageBox(NULL, L"no hwndFolderCtrl", L"Cause", 0);
			//if(!hwndTV)MessageBox(NULL, L"no hwndTV", L"Cause", 0);
			//if(!item)MessageBox(NULL, L"no item", L"Cause", 0);
			//TODO error unable to scroll into view
			//considerHACK: SendKeys.Send("{TAB}{TAB}{DOWN}{DOWN}{UP}{UP}"); how to in C?
		}
	}
	
	return 0;
}
int promptBrowsePath(wchar_t* ploc, wchar_t* hint){
	BROWSEINFO binf = {0};
	binf.lpszTitle = hint;
	binf.ulFlags = BIF_BROWSEINCLUDEFILES | BIF_USENEWUI;
	binf.lpfn = BrowseCallbackProc;
	binf.lParam = (LPARAM)ploc;
	LPITEMIDLIST res = SHBrowseForFolder(&binf);
	if(res==NULL)return 0;
	BOOL res2 = SHGetPathFromIDList(res, ploc);
	//TODO error check res2
	ILFree(res);
	return 1;
}

int ProgReport(char *path){
	int l = mbstowcs(NULL, path, strlen(path))+2;
	wchar_t* wp = malloc(l*2);
	mbstowcs(wp, path, l);
	_updateProg(wp);
	free(wp);
	if(cancelWork)_setState(L"Canceled.");
	return cancelWork;
}
int exec_pack(wchar_t *inputPath, wchar_t *outputPath, int *overwrite){
	workCompleteMessage = WCM_PACK;
	if(!wtr_up_overwrite && !*overwrite && PathFileExistsW(outputPath)){
		int res = promptOverwrite();
		if(res==2)return 0;
		*overwrite=res;
		wtr_up_overwrite=1;//user prompted for overwrite
	}
	WStringArray* inputList = WStringArray_Alloc();
	_listFiles(inputPath, L".", inputList);//consider changing filecount to filesizes?
	WD_File_Descriptor wdfd = {0};
	wd_createDefaultWDHeader(&wdfd.head);
	wdfd.dir = calloc(inputList->cur, sizeof(WD_Directory_Entry));
	wdfd.dirlen = inputList->cur;
	size_t pstart_mlen = wcstombs(NULL, inputPath, 0)+4;
	char *pstart = malloc(pstart_mlen);
	size_t ret = wcstombs(pstart, inputPath, pstart_mlen);
	//TODO error check character conversion
	char sfoutpath[MAX_PATH*2];
	FILE *outHAND = NULL;
	//special case check for "FILETIME" and "GUID"
	PathCombineA(sfoutpath, pstart, "GUID");
	outHAND = fopen(sfoutpath, "rb");
	if(outHAND!=NULL){
		char gt[16]={0};
		if(fread(gt, 1, 16, outHAND)==16){
			memcpy(wdfd.head.guid, gt, 16);
		}
		fclose(outHAND); outHAND=NULL;
	}
	//default to current time
	GetSystemTimeAsFileTime(&wdfd.timestamp);
	PathCombineA(sfoutpath, pstart, "FILETIME");
	outHAND = fopen(sfoutpath, "rb");
	if(outHAND!=NULL){
		char ft[sizeof(FILETIME)]={0};
		if(fread(ft, 1, sizeof(FILETIME), outHAND)==sizeof(FILETIME)){
			memcpy(&wdfd.timestamp, ft, sizeof(FILETIME));
		}
		fclose(outHAND); outHAND=NULL;
	}
	int result = 0;
	makeDirsW(outputPath);
	outHAND = _wfopen(outputPath, L"wb");
	if(outHAND!=NULL){
		wdfd.handle = outHAND; outHAND=NULL;
		for(int i = 0; i<wdfd.dirlen; i++){
			WD_Directory_Entry *ent = &wdfd.dir[i];
			size_t plf_mlen = wcstombs(NULL, inputList->array[i], 0)+4;
			char *plf = malloc(plf_mlen);
			size_t ret = wcstombs(plf, &inputList->array[i][2], plf_mlen);
			//TODO error check character conversion
			PathCombineA(sfoutpath, pstart, plf);
			ent->path = _strdup(plf);
			ent->source_path = _strdup(sfoutpath);
			char *ext = PathFindExtensionA(sfoutpath);
			if(!_stricmp(ext, ".phx")){
				ent->header.flags = 0;//do not compress physics files
			}else{
				//default flag to compressed
				ent->header.flags = WDFLAG_COMPRESSED;
			}
			//TODO check for plaintext files
		}
		_setProgRange(wdfd.dirlen);
		result = wd_createNew(&wdfd, ProgReport);
		if(result<0)wtr_errorCount = -result;
	//}else{//TODO ERROR CHECKING _setErr(L"");
	}
	free(pstart);
	wd_Free(&wdfd);
	WStringArray_free(inputList);
	return result;
}
int exec_unpack(wchar_t *inputPath, wchar_t *outputPath, int *overwrite, int preserve){
	workCompleteMessage = WCM_UNPACK;
	WD_File_Descriptor wdfd = {0};
	wdfd.handle = _wfopen(inputPath, L"rb");
	if(wdfd.handle == NULL)return 1;
	int wdres = wd_openExisting(&wdfd);
	_setProgRange(wdfd.dirlen);
	size_t pstart_mlen = wcstombs(NULL, outputPath, 0)+4;
	char *pstart = malloc(pstart_mlen);
	size_t ret = wcstombs(pstart, outputPath, pstart_mlen);
	char sfoutpath[MAX_PATH*2];
	//TODO error check character conversion
	if(preserve){//preserves GUID and FILETIME
		//TODO TEST THIS CODE
		PathCombineA(sfoutpath, pstart, "GUID");
		FILE *outHAND = fopen(sfoutpath, "wb");
		if(outHAND!=NULL){
			fwrite(wdfd.head.guid, 1, 16, outHAND);
			fclose(outHAND);
		}
		PathCombineA(sfoutpath, pstart, "FILETIME");
		outHAND = fopen(sfoutpath, "wb");
		if(outHAND!=NULL){
			fwrite(&wdfd.timestamp, 1, sizeof(FILETIME), outHAND);
			fclose(outHAND);
		}
	}
	for(int i = 0; i<wdfd.dirlen; i++){
		WD_Directory_Entry *ent = &wdfd.dir[i];
		//printf("FileToUnpack: %d: %s\n", i, ent->path);
		if(ProgReport(ent->path))break;
		PathCombineA(sfoutpath, pstart, ent->path);
		//printf("makedir and open %s\n", sfoutpath);
		makeDirs(sfoutpath);
		if(!wtr_up_overwrite && !*overwrite && PathFileExistsA(sfoutpath)){
			int res = promptOverwrite();
			if(res==2)break;
			*overwrite = res;
			wtr_up_overwrite=1;//user prompted for overwrite
		}
		if(*overwrite || !PathFileExistsA(sfoutpath)){
			FILE *outHAND = fopen(sfoutpath, "wb");
			if(outHAND!=NULL){
				wd_pipeFileOut(outHAND, &wdfd, ent);
				fclose(outHAND);
			}
		}
	}
	//TODO manage error count
	free(pstart);
	wd_Free(&wdfd);
	return 0;
}
//int executeCommandFS(wchar_t *inputPath, wchar_t *outputPath, int action, int bulk, int overwrite, int preserve, int forceSingle){
int executeCommandFS(wchar_t *inputPath, wchar_t **outputPath, int action, int overwrite, int preserve){
	cancelWork = 0;
	if(action==ACT_AUTO){
		//Initially make it single only
		if(wcslen(inputPath)>3){
			int endoff = wcslen(inputPath)-3;
			wchar_t *ext = &inputPath[endoff];
			//wprintf(L"%s\n", inputPath);
			//wprintf(L"%s\n", ext);
			if(!_wcsicmp(ext, L".wd")){//input path is wd file, must mean single unpack
				action=ACT_UNPACK;
			}
		}
		if(PathIsDirectory(inputPath)){
			action=ACT_PACK;
		}
	}
	if(action==ACT_AUTO)return 1; //Error auto unable to determine mode
	if(action == ACT_PACK)workQuitMessage=WQM_PACK;
	if(action == ACT_UNPACK)workQuitMessage=WQM_UNPACK;
	if(!_validate(inputPath, outputPath, action))return 1; //Error invalid input
	_setState(L"");
	if(!promptIOAcceptA(&inputPath, outputPath, 1, action)){
		return 1;//User canceled action
	}
	//_simProg();
	if(action == ACT_PACK){
		int exres = exec_pack(inputPath, *outputPath, &overwrite);
	}else if(action == ACT_UNPACK){
		int exres = exec_unpack(inputPath, *outputPath, &overwrite, preserve);
	}
	_setState(L"Finished.");
	wtr_up_overwrite = 0;//reset user prompt for overwrite
	return 0;
}
void changeUIState(BOOL mode){
	for(int cid = ID_STATE_FIRST; cid <= ID_STATE_LAST; cid++){
		HWND hwndTC = GetDlgItem(mainWindow, cid);
		EnableWindow(hwndTC, mode);
	}
}
int _getCheckedID(int Id){
	HWND hwnd = GetDlgItem(mainWindow, Id);
	LRESULT chk = SendMessage(hwnd, BM_GETCHECK, (WPARAM)NULL,(LPARAM)NULL);
	if(chk==BST_CHECKED){
		return 1;
	}else{
		return 0;
	}
}
wchar_t* _getTextID(int Id){
	HWND hwnd = GetDlgItem(mainWindow, Id);
	int tlen = GetWindowTextLength(hwnd)+5;
	wchar_t* tbuf = malloc(tlen*2);
	GetWindowText(hwnd, tbuf, tlen+4);
	return tbuf;
}
//TODO let prompt handle printing of info and checking auto-accept flag
#define PIOT L"Confirm"
#define PIOM_PACK L"Are you sure you wish to pack?"
#define PIOM_UNPACK L"Are you sure you wish to unpack?"
#define _PIOM_PACK L"Are you sure you wish to pack the following files?"
#define _PIOM_UNPACK L"Are you sure you wish to unpack the following files?"
#define PIOF_NL L"%s\n%s -> %s"

int promptIOAcceptM(wchar_t **sources, wchar_t**destinations, size_t count, wchar_t* message){
	if(count<=0)return 0; //Error must be positive count
	if(wtr_confirmed || consoleMode)return 1;
	
	PromptVar var = {0};
	var.promptTitle = PIOT;
	var.promptMessage = message;
	var.inputs = sources;
	var.outputs = destinations;
	var.count = count;
	return promptFileAction(mainWindow, &var);
}
int promptIOAcceptA(wchar_t **sources, wchar_t**destinations, size_t count, int action){
	if(action == ACT_PACK){
		return promptIOAcceptM(sources, destinations, count, PIOM_PACK);
	}else if(action == ACT_UNPACK){
		return promptIOAcceptM(sources, destinations, count, PIOM_UNPACK);
	}
}

DWORD WINAPI createWorkerThread(LPVOID lpParameter){
	int err = executeCommandFS((wchar_t*)wtr_input, (wchar_t**)&wtr_output,
		(int)wtr_action, (int)wtr_overwrite, (int)wtr_preserve);
	
	if(wtr_input){
		free((wchar_t*)wtr_input);
		wtr_input=NULL;
	}
	if(wtr_output){
		free((wchar_t*)wtr_output);
		wtr_output=NULL;
	}
	//TODO send thread finished notification to main window
	PostMessage(mainWindow, WM_ACTDONE, (WPARAM)NULL, (LPARAM)NULL);
	return ERROR_SUCCESS;
}
int attemptInputlist(WStringArray* inputlist){
	if(inputlist->cur <= 0)return 0;
	if(inputlist->cur >= 1){
		wtr_input = _wcsdup(inputlist->array[0]);
	}else{
		wtr_input = _wcsdup(L"");
	}
	if(inputlist->cur > 1){
		int allwd = 1;
		int allfolder = 1;
		for(int i = 0; i<inputlist->cur; i++){
			if(!allwd && !allfolder)return 0;
			if(PathIsDirectory(inputlist->array[i])){
				allwd=0;continue;
			}
			if(PathFileExists(inputlist->array[i])){
				allfolder=0;continue;
			}
		}
		if(allwd || allfolder){
			wchar_t** sourcelist = calloc(inputlist->cur+2, sizeof(wchar_t*)*2);
			wchar_t** destlist = &sourcelist[inputlist->cur+1];
			int action = 0;
			if(allwd){
				action = ACT_UNPACK;
			}else if(allfolder){
				action = ACT_PACK;
			}
			for(int i = 0; i<inputlist->cur; i++){
				sourcelist[i] = inputlist->array[i];
				if(wtr_output!= NULL){
					destlist[i] = calloc(MAX_PATH*2, sizeof(wchar_t));
					wchar_t* fname = PathFindFileNameW(sourcelist[i]);
					if(!PathCombineW(destlist[i], (LPCWSTR)wtr_output, (LPCWSTR)fname)){
						//TODO error
					}
					if(action == ACT_UNPACK){
						wchar_t *ext = PathFindExtensionW(destlist[i]);
						ext[0]=0;
					}else if(action == ACT_PACK){
						wcscat(destlist[i], L".wd");
					}
				}else{
					destlist[i] = _wcsdup(L"");
				}
				BOOL err = !_validate(sourcelist[i], &destlist[i], action);
				if(err){
					action = 0;
					break;
				}
			}
			if(action!=0){//user prompt
				if(!promptIOAcceptA(sourcelist, destlist, inputlist->cur, action)){
					action = 0;
				}
			}
			if(action!=0){//execute action
				cancelWork = 0;
				int overwrite = wtr_overwrite;
				for(int i = 0; i<inputlist->cur; i++){
					//TODO check for errors
					if(action == ACT_PACK){
						int exres = exec_pack(sourcelist[i], destlist[i], &overwrite);
					}else if(action == ACT_UNPACK){
						int exres = exec_unpack(sourcelist[i], destlist[i], &overwrite, wtr_preserve);
					}
				}
				wtr_up_overwrite = 0;
			}
			//free sources and destinations
			for(int i = 0; i<inputlist->cur; i++){
				if(destlist[i])free(destlist[i]);
			}
			free(sourcelist);
			if(action==0)return 0;
			return 1;
		}
		return 0;
	}else{
		if(wtr_output == NULL){
			if(inputlist->cur == 2){
				wtr_output = _wcsdup(inputlist->array[1]);
			}else{
				wtr_output = _wcsdup(L"");
			}
		}
		int err = executeCommandFS((wchar_t*)wtr_input, (wchar_t**)&wtr_output,
		(int)wtr_action, (int)wtr_overwrite, (int)wtr_preserve);
		
		if(err)return 0;
	}
	return 1;
}
int parseCommand(int action){
	if(workThread != NULL)return 1;
	wtr_action = action;
	wtr_input = _getTextID(IDC_EDIT_INPUT);
	wtr_output = _getTextID(IDC_EDIT_OUTPUT);//allocates memory, must free.
	//wtr_bulk = _getCheckedID(ID_CHECK_BULK); //Disabled for simplicty
	//wtr_forceSingle = !wtr_bulk;//may be unneccesary but adding it anyway
	wtr_overwrite = _getCheckedID(ID_CHECK_OVERWRITE);
	//wtr_preserve = _getCheckedID(ID_CHECK_PRESERVE);
	wtr_preserve = 0; //NO Preserve Button currently.

	changeUIState(FALSE);
	workThread = CreateThread(NULL, 0,createWorkerThread,0,0,0);
	
	return 0;
}
void debugConsole(){
	if(!AttachConsole(ATTACH_PARENT_PROCESS))AllocConsole();
	freopen("CONIN$", "r", stdin); 
	freopen("CONOUT$", "w", stdout); 
	freopen("CONOUT$", "w", stderr); 
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
	
	//debugConsole();
	
	WStringArray* inputlist = WStringArray_Alloc();
	if(wcslen(pCmdLine)){
		int argc = 0;
		LPWSTR *argv = CommandLineToArgvW(pCmdLine, &argc);
		wtr_action = ACT_AUTO;
		for(int i = 0; i<argc; i++){
			if(i==0 && !_wcsicmp(argv[i],L"pack")){
				wtr_action = ACT_PACK;
			}else if(i==0 && !_wcsicmp(argv[i],L"unpack")){
				wtr_action = ACT_UNPACK;
			}else if(!_wcsicmp(argv[i],L"--yes") 
				    || !_wcsicmp(argv[i],L"-y")
				    || !_wcsicmp(argv[i],L"--noprompt")){
				wtr_confirmed = 1;
			}else if(!_wcsicmp(argv[i],L"--nogui")){
				consoleMode = 1;
			}else if(!_wcsicmp(argv[i],L"--overwrite")
				    || !_wcsicmp(argv[i],L"-ow")){
				wtr_overwrite = 1;
			}else if(!_wcsicmp(argv[i],L"--output")
				    || !_wcsicmp(argv[i],L"-o")){
				if((i+1)>=argc || wtr_output!=NULL){
					return 1;
				}
				i++;
				wtr_output=_wcsdup(argv[i]);
			}else if(!_wcsicmp(argv[i],L"--preserve")
				    || !_wcsicmp(argv[i],L"-p")){
				wtr_preserve = 1;
			}else{
				WStringArray_Add(inputlist, argv[i]);
			}
		}
		LocalFree(argv);
	}
	if(consoleMode!=1){
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_STANDARD_CLASSES;
		InitCommonControlsEx(&icex);
		//claimed to be required, but doesn't seem like it when I test it.
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	}
	//parse input and attempt to complete an action
	wtr_errorCount = 0;
	actionComplete = attemptInputlist(inputlist);
	WStringArray_free(inputlist);
	
	//error state depends on action completion
	if(actionComplete && consoleMode!=1){
		//TODO print error count
		//swprintf error count into work message
		wchar_t* workMessage = (wchar_t*)workCompleteMessage;//temporary
		MessageBox(NULL, workMessage, L"Done", MB_OK);
		//free work message
	}
	if(consoleMode==1 || actionComplete)return !actionComplete;
	
	const wchar_t CLASS_NAME[]  = L"TW1WDRepacker";
	WNDCLASS wc = { };
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);
	
	//get window frame border metrics to properly size window
	int bwx = GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	int bwy = GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFIXEDFRAME)*2;
	
	mainWindow = CreateWindowEx(
		WS_EX_ACCEPTFILES,              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"TwoWorlds1 WD Repacker",      // Window text
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU ,            
		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, L2W+bwx, L2H+bwy,
		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
		);
	if(mainWindow == NULL)return 1;
	
	ShowWindow(mainWindow, nCmdShow);
	
	MSG msg = { };
	DWORD workRes = 0;
	while (GetMessage(&msg, NULL, 0, 0) > 0){
		if(!IsDialogMessage(mainWindow, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(workThread != NULL){
			DWORD waitRes = WaitForSingleObject(workThread, 0);
			if(waitRes == WAIT_OBJECT_0){
				GetExitCodeThread(workThread, &workRes);
				//TODO handle thread exit conditions
				workThread = NULL;
				changeUIState(TRUE);
			}
		}
	}
	CoUninitialize();
	return 0;
}
void constructUI(HINSTANCE inst, HWND hwnd){
	HWND cel;
	int err = 0;
	int pnum = ID_STATIC_PAD;
	int x = LBORD;
	int y = LBORD;
	//Border
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		0, 0, L2W, LBORD, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		0, LBORD, LBORD, L2H-LBORD, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		L2W-LBORD, LBORD, LBORD, L2H-LBORD, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		LBORD, L2H-LBORD, L2W-LBORD*2, LBORD, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	//INPUT
	cel = CreateWindowEx(0,L"STATIC",L"Source",
		WS_VISIBLE | WS_CHILD,
		x, y, L1W, L1H, hwnd,
		(HMENU)IDL_EDIT_INPUT,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"EDIT",L"",
		WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
		x+L1W, y, L2Wi-L1W-L1W2, L1H, hwnd,
		(HMENU)IDC_EDIT_INPUT,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"BUTTON",L"...",
		WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x+L2Wi-L1W2, y, L1W2, L1H, hwnd,
		(HMENU)ID_BUTTON_BROWSE_I,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1H;
	//PAD
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		x, y, L2Wi, L1HH, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1HH;
	//OUTPUT
	cel = CreateWindowEx(0,L"STATIC",L"Destination",
		WS_VISIBLE | WS_CHILD,
		x, y, L1W, L1H, hwnd,
		(HMENU)IDL_EDIT_OUTPUT,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"EDIT",L"",
		WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
		x+L1W, y, L2Wi-L1W-L1W2, L1H, hwnd,
		(HMENU)IDC_EDIT_OUTPUT,
		inst,NULL
	);if(cel==NULL)err++;
	cel = CreateWindowEx(0,L"BUTTON",L"...",
		WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x+L2Wi-L1W2, y, L1W2, L1H, hwnd,
		(HMENU)ID_BUTTON_BROWSE_O,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1H;
	//PAD
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		x, y, L2Wi, L1HH, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1HH;
	//MODIFIERS
	cel = CreateWindowEx(0,L"Button",L"Overwrite",
		BS_CHECKBOX | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x, y, L1W, L1H, hwnd,
		(HMENU)ID_CHECK_OVERWRITE,
		inst,NULL
	);if(cel==NULL)err++;
	x+=L1W;
	/*cel = CreateWindowEx(0,L"Button",L"Bulk",
		BS_CHECKBOX | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x, y, L1W, L1H, hwnd,
		(HMENU)ID_CHECK_BULK,
		inst,NULL
	);if(cel==NULL)err++;
	x+=L1W;*/
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		x, y, L1W*2, L1H, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	/*cel = CreateWindowEx(0,L"Button",L"Preserve",
		BS_CHECKBOX | WS_VISIBLE | WS_CHILD,
		x, y, L1W, L1H, hwnd,
		(HMENU)ID_CHECK_PRESERVE,
		inst,NULL
	);if(cel==NULL)err++;*/
	x+=L1W*2;
	//ACTION
	cel = CreateWindowEx(0,L"Button",L"Pack",
		BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x, y, L1W, L1H, hwnd,
		(HMENU)ID_BUTTON_PACK,
		inst,NULL
	);if(cel==NULL)err++;
	x+=L1W;
	cel = CreateWindowEx(0,L"Button",L"Unpack",
		BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
		x, y, L1W, L1H, hwnd,
		(HMENU)ID_BUTTON_UNPACK,
		inst,NULL
	);if(cel==NULL)err++;
	x=LBORD;
	y+=L1H;
	//TODO move to separate window
	//PAD
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD,
		x, y, L2Wi, L1HH, hwnd,
		(HMENU)pnum++,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1HH;
	//PROGRESS
	cel = CreateWindowEx(0,L"STATIC",L"",
		WS_VISIBLE | WS_CHILD | SS_PATHELLIPSIS,
		x, y, L2Wi, L1H, hwnd,
		(HMENU)IDL_PROGRESS_FILES,
		inst,NULL
	);if(cel==NULL)err++;
	y+=L1H;
	cel = CreateWindowEx(0,PROGRESS_CLASS,(LPTSTR)NULL,
		WS_VISIBLE | WS_CHILD,
		x, y, L2Wi, L1H, hwnd,
		(HMENU)IDC_PROGRESS_FILES,
		inst,NULL
	);if(cel==NULL)err++;
	
	if(err>0){
		//Error has occured, crash program?
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
		case WM_CREATE:
			{
				HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
				constructUI(inst, hwnd);
				//if commandline input had files, add them to IO
				wchar_t* tr_input = (wchar_t*)wtr_input;
				if(tr_input!=NULL){
					HWND sourcewind = GetDlgItem(hwnd, IDC_EDIT_INPUT);
					SetWindowText(sourcewind, tr_input);
					free(tr_input);
					wtr_input=NULL;
					tr_input=NULL;
				}
				wchar_t* tr_output = (wchar_t*)wtr_output;
				if(tr_output!=NULL){
					HWND outptwind = GetDlgItem(hwnd, IDC_EDIT_OUTPUT);
					SetWindowText(outptwind, tr_output);
					free(tr_output);
					wtr_output=NULL;
					tr_output=NULL;
				}
				/*if(wtr_bulk){ //Disabled for Simplicity
					HWND bhand = GetDlgItem(hwnd, ID_CHECK_BULK);
					SendMessage(bhand, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
				}*/
				if(wtr_overwrite){
					HWND ohand = GetDlgItem(hwnd, ID_CHECK_OVERWRITE);
					SendMessage(ohand, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
				}
				/*if(wtr_preserve){//No preserve button currently
					HWND phand = GetDlgItem(hwnd, ID_CHECK_PRESERVE);
					SendMessage(phand, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
				}*/
			}
			break;
		case WM_CLOSE:
			if(workThread!=NULL){
				int res = MessageBox(hwnd, (wchar_t*)workQuitMessage, L"Quit Anyway?", MB_YESNO | MB_ICONWARNING);
				if(res==IDNO)break;
				cancelWork = 1;
				//waits 2.5 sec for the cancel to take effect, increase later if needed?
				DWORD waitRes = WaitForSingleObject(workThread, 2500);
			}
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_ACTDONE:
			if(workThread != NULL)WaitForSingleObject(workThread, 1000);
		  break;
		case WM_COMMAND:
			{
				int ID = LOWORD(wParam);
				switch(ID){
					case ID_CHECK_BULK:
					case ID_CHECK_OVERWRITE:
					case ID_CHECK_PRESERVE:
						{//Code required to toggle checkboxes
							HWND bhand = GetDlgItem(hwnd, ID);
							LRESULT curstate = SendMessage(bhand, BM_GETCHECK, (WPARAM)NULL,(LPARAM)NULL);
							if(curstate==BST_CHECKED){
								PostMessage(bhand, BM_SETCHECK, BST_UNCHECKED, (LPARAM)NULL);
							}else{
								PostMessage(bhand, BM_SETCHECK, BST_CHECKED, (LPARAM)NULL);
							}
						}
						break;
					case ID_BUTTON_PACK:
						parseCommand(ACT_PACK);
						break;
					case ID_BUTTON_UNPACK:
						parseCommand(ACT_UNPACK);
						break;
					case ID_BUTTON_BROWSE_I:
						{
							wchar_t pathres[MAX_PATH*2]={0};
							HWND sourcewind = GetDlgItem(hwnd, IDC_EDIT_INPUT);
							GetWindowText(sourcewind, pathres, MAX_PATH*2);
							if(promptBrowsePath(pathres, L"Select input path")){
								SetWindowText(sourcewind, pathres);
							}
						}
						break;
					case ID_BUTTON_BROWSE_O:
						{
							wchar_t pathres[MAX_PATH*2]={0};
							HWND destwind = GetDlgItem(hwnd, IDC_EDIT_OUTPUT);
							GetWindowText(destwind, pathres, MAX_PATH*2);
							if(promptBrowsePath(pathres, L"Select output path")){
								SetWindowText(destwind, pathres);
							}
						}
						break;
				}
			}
			break;
		case WM_DROPFILES:
			{
				int dropDest=0; //1 source, 2 dest
				HDROP drophandle = (HDROP)wParam;
				POINT pt = {};
				DragQueryPoint(drophandle, &pt);
				RECT destrect = {};
				HWND destwind = GetDlgItem(hwnd, IDC_EDIT_OUTPUT);
				GetWindowRect(destwind, &destrect);
				MapWindowPoints(NULL, hwnd, &destrect, 2);
				if(pt.y < destrect.top){ //TODO MAKE TERNARY STATEMENT
					dropDest=1;
				}else{
					dropDest=2;
				}
				UINT fcount = DragQueryFile(drophandle, 0xFFFFFFFF, NULL, 0);
				if(dropDest && fcount){
					UINT reqS = DragQueryFile(drophandle, 0, NULL, 0);
					wchar_t *fname = (wchar_t *)malloc( (reqS+2)*2);
					DragQueryFile(drophandle, 0, fname, reqS+1);
					if(dropDest==1){//source
						//TODO optional, support multiple inputs
						HWND sourcewind = GetDlgItem(hwnd, IDC_EDIT_INPUT);
						SetWindowText(sourcewind, fname);
					}else if(dropDest==2){//dest
						SetWindowText(destwind, fname);
					}
					free(fname);
				}
				DragFinish(drophandle);
			}
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}