#include <windows.h>

typedef struct _PromptVars{
	wchar_t* promptTitle;
	wchar_t* promptMessage;
	wchar_t** inputs;
	wchar_t** outputs;
	int count;
} PromptVar;

int promptFileAction(HWND parent, PromptVar* vars);
