format PE GUI 4.0 ; PE64 and 6.0 for 64bit
include '%include%\win32a.inc' ; win64a.inc if 64 bit
include 'reconst_res.inc'

; Assembled with FASM (flat assembler)

APP_MAX_EVENT_COUNT = 0x40

APP_MAX_LINE_BUFFER = 0x400
APP_PATH_BUFMAX     = 0x400
; Macro snprintf for Buffer A
macro SLB_A form,[args]{
  common cinvoke snprintf_s,TXT_LineBuffer_A,APP_MAX_LINE_BUFFER,[APP_WIDTH],form,args
}
macro PLB_A form,[args]{
  common SLB_A   form,args
  common cinvoke strlen,TXT_LineBuffer_A
  common invoke  WriteConsole,[hSTDOUT],TXT_LineBuffer_A,eax,0,0
}

entry BeginCode

section '.code' executable readable writeable
BeginCode:
        ; Initial Setup
        call      AppInitialize
        call      AppConsoleInit
        call      AppConsoleLoop ; DOES NOT RETURN WHEN USER CLICKS [X] BUTTON
EndApp:
        call     AppCleanup
        invoke   ExitProcess,[AppExitCode]
        ret
CrashApp:
        invoke MessageBox,0,[CrashApp_Msg],CrashApp_Title,MB_ICONERROR
        jmp EndApp
; Procedure definitions
proc AppInitialize uses ebx edx
        locals  ; use locals macro
          ;l_Args  dd ?
          ;l_Argv  dd ?
          ;l_Argc  dd 0
        endl
        ; alt: locals variable:type,var2:type
        invoke GetModuleHandle,0
        mov [hInstance],eax
        invoke GetStartupInfo,AppStartInfo
        ;invoke GetCommandLine; result in eax, passed to next function
        ;mov [l_Args], eax
        ;lea ebx, [l_Argc]
        ;invoke CommandLineToArgv,[l_Args],ebx
        ;mov [l_Argv], eax
        ; process commandline arguments
        ;invoke LocalFree,[l_Argv]
        mov     [APP_MBHELD],0
        mov     [APP_VIEWOFF],0
        mov     [APP_CUR_LINE],1
        mov     [APP_MAX_LINE],0
        mov     [APP_ARR_AS],0
        ; TODO get Mod Files and Registry State
        call AppModListInit ; Reads mod files
        call GenerateActiveState ; Reads current registry state
        ; Check if any files found, error if 0 mod files
        cmp     [APP_MAX_LINE],0
        je      .errnofiles
        ret
.errnofiles:
        mov  [AppExitCode],2
        mov  [CrashApp_Msg],CrashReason_NoMods
        jmp     CrashApp
        ret
  endp
proc AppModListInit uses ebx
        locals
          hFind      dd ?
          resData    WIN32_FIND_DATAA
          pathBuf    rb APP_PATH_BUFMAX
        endl
        cinvoke      malloc,0x40      ; Initialize Modlist Pointer
        mov          [APP_MODLIST_PTR],eax
        mov          [APP_ARR_AS],0x10
        mov          [APP_MAX_LINE],0
        lea          eax,[pathBuf]    ; Init Search Path
        cinvoke      strcpy_s,eax,APP_PATH_BUFMAX,P_FILE_REL
        lea          eax,[pathBuf]
        cinvoke      strcat_s,eax,APP_PATH_BUFMAX,P_FILE_SEARCH
        lea          eax,[pathBuf]
        lea          ebx,[resData]
        invoke       FindFirstFile,eax,ebx
        mov          [hFind],eax
        cmp          eax,INVALID_HANDLE_VALUE
        je           .end
.addfstr:
        lea          eax,[resData.cFileName]
        stdcall      AddStrToList,eax
        lea          ebx,[resData]
        invoke       FindNextFile,[hFind],ebx
        test         eax,eax
        jnz          .addfstr
        invoke       FindClose,[hFind]
.end:
        ret
  endp
proc GenerateActiveState uses ebx ecx edx
        locals
          l_ln    dd ?
          l_hkey  dd ?
          l_rest  dd ?
          l_resd  dd ?
          l_ress  dd ?
        endl
        xor          eax,eax
        mov          [l_ln],eax
        mov          [l_rest],eax
        mov          [l_resd],eax
        cinvoke      malloc,[APP_ARR_AS]
        mov          [APP_ACTLIST_PTR],eax
        cinvoke      memset,eax,0,[APP_ARR_AS]
        lea          eax,[l_hkey]
        invoke       RegCreateKey,P_REG_ROOT,P_REG_MODS,0,0,0,KEY_READ,0,eax,0
        ;TODO chcek error?
.lj_start:
        mov          eax,[l_ln]
        cmp          eax,[APP_MAX_LINE]
        jge          .lj_end
        mov          ebx,[APP_MODLIST_PTR]
        mov          eax,[ebx+eax*4]
        lea          ebx,[l_rest]
        lea          ecx,[l_resd]
        lea          edx,[l_ress]
        mov          [l_ress],4
        invoke       RegQueryValue,[l_hkey],eax,0,ebx,ecx,edx
        test         eax,eax
        jnz          .lj_noval ; Error, likely does not exist
        cmp          [l_rest],REG_DWORD
        jne          .lj_noval ; Wrong type, just in case
        mov          eax,[l_resd]
        mov          ebx,[APP_ACTLIST_PTR]
        mov          ecx,[l_ln]
        mov          byte[ebx+ecx],al
.lj_noval:
        inc          [l_ln]
        jmp          .lj_start
.lj_end:
        invoke       RegCloseKey,[l_hkey]
        ret
  endp
proc UpdateRegistry stdcall uses ebx,\
  lnum:DWORD
        locals
          l_hkey  dd ?
          l_sp    dd ?
          l_resd  dd ?
        endl
        mov          ebx,[APP_ACTLIST_PTR]
        mov          eax,[lnum]
        dec          eax                ; from 1 indexed to 0 indexed
        movzx        ebx,byte[ebx+eax]
        mov          [l_resd],ebx ; Value now loaded to local
        mov          ebx,[APP_MODLIST_PTR]
        mov          ebx,[ebx+eax*4]
        mov          [l_sp],ebx   ; Value Name loaded to local
        lea          eax,[l_hkey]
        invoke       RegCreateKey,P_REG_ROOT,P_REG_MODS,0,0,0,KEY_WRITE,0,eax,0
        ; TODO check error
        lea          eax,[l_resd]
        invoke       RegSetValue,[l_hkey],[l_sp],0,REG_DWORD,eax,4
        ; TODO check error
        invoke       RegCloseKey,[l_hkey]
        ret
  endp
proc AppCleanup
        ; Series of freeing data stores
        ; So far not bothering, windows can cleanup when it closes stuff so forcefully,
        ; will have to implement close routine to make it matter
        call AppConsoleCleanup
        ret
  endp
proc AppConsoleInit uses ebx
        locals
          l_mode dd ?
        endl
        invoke  AllocConsole
        invoke  GetStdHandle,STD_OUTPUT_HANDLE
        mov     [hSTDOUT],eax
        invoke  GetStdHandle,STD_INPUT_HANDLE
        mov     [hSTDIN],eax
        ; set Output Modes
        lea     ebx,[l_mode]
        invoke  GetConsoleMode,[hSTDOUT],ebx
        mov     ebx,[l_mode]
        mov     [hOrgOutMode],ebx
        or      [l_mode],5;turn on: ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT
        invoke  SetConsoleMode,[hSTDOUT],[l_mode]
        ; set Input Modes
        lea     ebx,[l_mode]
        invoke  GetConsoleMode,[hSTDIN],ebx
        mov     ebx,[l_mode]
        mov     [hOrgInMode],ebx
        or      [l_mode],18h;turn on: ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT
        and     [l_mode],0xFFFFFFB9 ;turn off: ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE
        invoke  SetConsoleMode,[hSTDIN],[l_mode]
        ; Setting New Buffer makes ClearScreen work.
        stdcall Print,VT_NEWBUF
        stdcall Print,VT_CUR_HIDE ;Hide default cursor
        PLB_A   FORM_VT_SETTITLE,APP_TITLE_NAME
        ret
  endp
proc AppConsoleLoop uses ebx ecx edx
        locals
          lo_i          dd ?
          numEvt        dd ?
        endl
        local events[APP_MAX_EVENT_COUNT]:INPUT_RECORD
.lj_sread:
        lea     ebx,[events]
        lea     ecx,[numEvt]
        invoke  ReadConsoleInput,[hSTDIN],ebx,APP_MAX_EVENT_COUNT,ecx
        test    eax,eax
        jz      .lj_eread
        ;check eax for error
        xor     ebx,ebx
        mov     [lo_i],ebx
.lj_nread:
        mov     ebx,[lo_i]   ; loop index
        mov     ecx,[numEvt] ; loop count
        cmp     ebx,ecx
        jge     .lj_sread
        ; Switch Message Type
        imul    ebx,sizeof.INPUT_RECORD
        lea     edx,[events+ebx];
        movzx   eax,word [edx+INPUT_RECORD.EventType]; move zero extended WORD to DWORD
        ; test Key Case
        cmp     eax,IR_KEY_EVENT
        jne     .case_NotKey
        stdcall ProcessKeyEvent,edx
        jmp     .case_D
 .case_NotKey:
        cmp     eax,IR_MOUSE_EVENT
        jne     .case_NotMouse
        stdcall ProcessMouseEvent,edx
        jmp     .case_D
 .case_NotMouse:
        cmp     eax,IR_WINDOW_BUFFER_SIZE_EVENT
        jne     .case_D
        stdcall ProcessResizeEvent,edx
 .case_D: ;Ignore Type/end switch
        inc     [lo_i]
        jmp     .lj_nread
.lj_eread:
        ret
  endp
proc Print stdcall uses ebx,\
  StrPtr:DWORD
        cinvoke strlen,[StrPtr]
        invoke  WriteConsole,[hSTDOUT],[StrPtr],eax,0,0
        ret
  endp
proc ProcessKeyEvent stdcall uses ebx,\
  eventPtr:DWORD
        mov    ebx,[eventPtr] ; INPUT_RECORD
        lea    ebx,[ebx+INPUT_RECORD.KeyEvent] ; IR_KEY_EVENT_RECORD
        mov    eax,[ebx+IR_KEY_EVENT_RECORD.bKeyDown] ; only on DownPress
        test   eax,eax
        jz     .noprnt
        movzx  eax,word[ebx+IR_KEY_EVENT_RECORD.wVirtualKeyCode]
        cmp     eax,VK_UP
        jne     .notcurup
        call    KeyScrollUp
        ;SCROLL UPDOWN
        jmp     .noprnt
.notcurup:
        cmp     eax,VK_DOWN
        jne     .notcurdown
        call    KeyScrollDown
        ;SCROLL UPDOWN
        jmp     .noprnt
.notcurdown:
        cmp     eax,VK_PRIOR
        jne     .notpageup
        call    KeyPageUp
        jmp     .noprnt
.notpageup:
        cmp     eax,VK_NEXT
        jne     .notpagedown
        call    KeyPageDown
        jmp     .noprnt
.notpagedown:
        cmp    eax,VK_RETURN ; Enter for Newline?
        jne    .noprnt
        call   ToggleActive
.noprnt:
        ret
  endp
proc ProcessMouseEvent stdcall uses ebx ecx,\
  eventPtr:DWORD
        locals
          l_sl  dd ?
          l_clc dd ?
        endl
        mov     ebx,[eventPtr]
        lea     ebx,[ebx+INPUT_RECORD.MouseEvent]
        mov     eax,[ebx+IR_MOUSE_EVENT_RECORD.dwButtonState]
        and     eax,IR_M_LMB
        xor     ecx,ecx          ; default to 0
        test    [APP_MBHELD],eax ; check if LMB is pressed now , compare to APP_MBHELD
        cmovz   ecx,eax          ; set to 1 if eax 1 and not held
        mov     [APP_MBHELD],eax ; update held
        mov     [l_clc],ecx ; l_clc now 1 if and only if eax is 1 and APP_MBHELD 0
        cmp     [l_clc],1
        ;jz      .mcurs ; Move Cursor on Single Click
        jnz     .skip
.mcurs:
        mov     eax,[ebx+IR_MOUSE_EVENT_RECORD.dwEventFlags]
        cmp     eax,IR_M_DOUBLE
        jne     .sinclc ;Toggle on double click
        call    ToggleActive
        jmp     .skip
.sinclc:
        movzx   ebx,[ebx+IR_MOUSE_EVENT_RECORD.dwMousePosition.y]
        inc     ebx
        add     ebx,[APP_VIEWOFF]
        mov     [l_sl],ebx
        cmp     ebx,[APP_MAX_LINE]
        jge     .skip
        stdcall MCursTo,[l_sl] ; TODO Toggle on DoubleClick
.skip:
        ret
  endp
proc ProcessResizeEvent stdcall uses ebx,\
  eventPtr:DWORD
        stdcall Print,VT_CLEARSCR
        stdcall Print,VT_RESETCUR
        mov     ebx,[eventPtr]
        movzx   eax,word[ebx+INPUT_RECORD.dwSize.x]
        mov     [APP_WIDTH],eax
        movzx   eax,word[ebx+INPUT_RECORD.dwSize.y]
        mov     [APP_HEIGHT],eax
        sub     eax,1
        mov     ebx,eax
        PLB_A   FORM_VT_SETMARG,0,ebx ; Set Margins
        call RenderScreen
        ret
  endp
proc RenderScreen uses ebx ecx
        locals
          l_cl          dd ?  ; Current Line
          l_cvpl        dd ?  ; Current Viewport Line
        endl
        stdcall Print,VT_CLEARSCR
        mov eax,[APP_VIEWOFF]
        inc eax
        mov [l_cl],eax
        mov [l_cvpl],1
.ll_pl:
        mov     ebx,[l_cvpl]
        cmp     ebx,[APP_HEIGHT]
        jge      .ll_en ; no more lines on screen
        PLB_A   FORM_VT_SETCUR,ebx,0
        mov     ebx,[l_cl]
        cmp     ebx,[APP_MAX_LINE]
        jg      .ll_en ; no more lines in list
        mov     ebx,[l_cvpl]
        PLB_A   FORM_VT_SETCUR,ebx,0
        mov     ebx,[l_cl]
        stdcall RenderLine,ebx
        inc     [l_cl]
        inc     [l_cvpl]
        jmp     .ll_pl
.ll_en: ;end of loop Write LineCount
        call     UpdateLineNum
.done:
        ret
  endp
proc RenderLine stdcall uses ebx,\
  lNum:DWORD
        locals
          l_ptr dd ?
        endl
        ; CHECK Selected Line
        stdcall Print,VT_CLEARLINE
        mov     eax,[lNum]
        cmp     eax,[APP_CUR_LINE]
        jz      .l_sel
        stdcall  Print,FORM_OTH_LINE
        jmp     .l_esel
.l_sel:
        stdcall  Print,FORM_CUR_LINE
.l_esel:
        mov     ebx,[APP_MODLIST_PTR] ; Get file name
        mov     eax,[lNum]
        dec     eax
        mov     ebx,[ebx+eax*4]
        mov     [l_ptr],ebx
        mov     ebx,[APP_ACTLIST_PTR] ; Check if active
        movzx   eax,byte[ebx+eax]
        test    eax,1
        jz      .inact
        PLB_A   FORM_ACTIVE_LINE,[l_ptr]
        jmp     .done
.inact:
        PLB_A   FORM_INACTIVE_LINE,[l_ptr]
.done:
        ret
  endp
proc MCursTo stdcall uses ebx ecx,\
  nPos:DWORD
        locals
          prevLine dd ?
          vp_cl    dd ?
        endl
        mov     eax,[nPos]
        cmp     eax,[APP_MAX_LINE]
        jge     .end ; out of range
        sub     eax,[APP_VIEWOFF]
        cmp     eax,[APP_HEIGHT]
        jge     .end ; out of scope
        mov     eax,[APP_CUR_LINE]
        mov     [prevLine],eax
        sub     eax,[APP_VIEWOFF]
        mov     [vp_cl],eax
        ; move selector
        mov      ebx,[vp_cl]
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_OTH_LINE
        mov      ebx,[nPos]
        mov      [APP_CUR_LINE],ebx
        sub      ebx,[APP_VIEWOFF]
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_CUR_LINE
.done:
        mov eax,[prevLine]
        cmp eax,[APP_CUR_LINE]
        je  .end
        call     UpdateLineNum
.end:
        ret
  endp
proc KeyPageUp stdcall uses ebx ecx edx
        mov    ebx,[APP_VIEWOFF]
        cmp    ebx,0
        je     .end
        mov    edx,[APP_HEIGHT]
        dec    edx
        sub    ebx,edx
        xor    ecx,ecx ; Restrict Minimum ViewOff to 0
        cmp    ebx,ecx
        cmovl  ebx,ecx
        mov    [APP_VIEWOFF],ebx
        mov    ebx,[APP_CUR_LINE]
        sub    ebx,edx
        mov    ecx,1   ; Restrict Minimum Line to 1
        cmp    ebx,ecx
        cmovl  ebx,ecx
        mov    [APP_CUR_LINE],ebx
        call   RenderScreen
.end:
        ret
  endp
proc KeyPageDown stdcall uses ebx ecx
        mov   ebx,[APP_VIEWOFF]
        mov   ecx,[APP_HEIGHT]
        dec   ecx
        add   ebx,ecx
        cmp   ebx,[APP_MAX_LINE]
        jge   .end ; Already at Last Page
        mov   [APP_VIEWOFF],ebx
        mov   ebx,[APP_CUR_LINE]
        add   ebx,ecx ; Restrict Cursor to Last Line
        cmp   ebx,[APP_MAX_LINE]
        cmovg ebx,[APP_MAX_LINE]
        mov   [APP_CUR_LINE],ebx
        call  RenderScreen
.end:
        ret
  endp
proc KeyScrollDown stdcall uses ebx ecx
        locals
          prevLine dd ?
          vp_cl    dd ?
        endl
        mov      eax,[APP_CUR_LINE]
        mov      [prevLine],eax
        cmp      eax,[APP_MAX_LINE]
        jge      .done ; Can't scroll past end
        sub      eax,[APP_VIEWOFF]
        mov      [vp_cl],eax
        ; remove selector
        PLB_A    FORM_VT_SETCUR,eax,0
        stdcall  Print,FORM_OTH_LINE
        mov      eax,[vp_cl]
        inc      eax
        cmp      eax,[APP_HEIGHT]
        jge      .scrollscr
        inc      [APP_CUR_LINE]
        mov      ebx,[vp_cl]
        inc      ebx
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_CUR_LINE
        jmp .done
.scrollscr:
        ; scrollscreen
        stdcall  Print,VT_VP_SCRDOWN
        ; increase curline and viewOff
        inc      [APP_CUR_LINE]
        inc      [APP_VIEWOFF]
        ; render single line
        mov      ebx,[vp_cl]
        PLB_A    FORM_VT_SETCUR,ebx,0
        ;stdcall  Print,FORM_CUR_LINE
        stdcall  RenderLine,[APP_CUR_LINE]
.done:
        mov eax,[prevLine]
        cmp eax,[APP_CUR_LINE]
        je  .end
        call     UpdateLineNum
.end:
        ret
  endp
proc KeyScrollUp stdcall uses ebx ecx
        locals
          prevLine dd ?
          vp_cl    dd ?
        endl
        mov     eax,[APP_CUR_LINE]
        mov     [prevLine],eax
        cmp     eax,1
        jle     .done ; Can't scroll past end
        sub     eax,[APP_VIEWOFF]
        mov     [vp_cl],eax
        ;dec     eax
        cmp     eax,1
        jle     .scrollscr
        ; remove selector
        mov      ebx,[vp_cl]
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_OTH_LINE
        dec      [APP_CUR_LINE]
        mov      ebx,[vp_cl]
        dec      ebx
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_CUR_LINE
        jmp .done
.scrollscr:
        ; remove selector
        mov      ebx,[vp_cl]
        PLB_A    FORM_VT_SETCUR,ebx,0
        stdcall  Print,FORM_OTH_LINE
        ; scrollscreen
        stdcall  Print,VT_VP_SCRUP
        ; increase curline and viewOff
        dec      [APP_CUR_LINE]
        dec      [APP_VIEWOFF]
        ; render single line
        mov      ebx,[vp_cl]
        PLB_A    FORM_VT_SETCUR,ebx,0
        ;stdcall  Print,FORM_CUR_LINE
        stdcall  RenderLine,[APP_CUR_LINE]
.done:
        mov eax,[prevLine]
        cmp eax,[APP_CUR_LINE]
        je  .end
        call     UpdateLineNum
.end:
        ret
  endp
proc ToggleActive uses ebx ecx
        mov     ebx,[APP_ACTLIST_PTR]
        mov     ecx,[APP_CUR_LINE]
        dec     ecx
        movzx   eax,byte[ebx+ecx]
        test    eax,eax
        setz    byte[ebx+ecx] ; Toggle the byte
        ; update view
        sub     ecx,[APP_VIEWOFF]
        PLB_A   FORM_VT_SETCUR,ecx,0
        stdcall RenderLine,[APP_CUR_LINE]
        ; update registry
        stdcall UpdateRegistry,[APP_CUR_LINE]
        ret
  endp
proc UpdateLineNum uses ebx ecx
        mov     ebx,[APP_HEIGHT]
        PLB_A   FORM_VT_SETCUR,ebx,0
        stdcall Print,VT_CLEARLINE
        mov     ebx,[APP_CUR_LINE]
        mov     ecx,[APP_MAX_LINE]
        PLB_A   FORM_LINENUBER,ebx,ecx
        ret
  endp
proc AppConsoleCleanup
        invoke  SetConsoleMode,[hSTDOUT],[hOrgOutMode] ; Be kind and reset console window mode
        invoke  SetConsoleMode,[hSTDIN],[hOrgInMode]
        ret
  endp
proc AppWindowCleanup

        ret
  endp
proc AddStrToList uses ebx ecx edx,\
  strPtr:DWORD
        locals
          l_sl  dd ?
          l_sp  dd ?
        endl
        cinvoke       strlen,[strPtr]
        add           eax,4
        mov           [l_sl],eax
        cinvoke       malloc,eax
        mov           [l_sp],eax
        cinvoke       strcpy_s,eax,[l_sl],[strPtr]
        mov           eax,[APP_MAX_LINE]
        cmp           eax,[APP_ARR_AS]
        jl            .spaceavail
        ; Increase array size
        mov           eax,[APP_ARR_AS]
        add           eax,0x10
        mov           [APP_ARR_AS],eax
        imul          eax,4
        cinvoke       realloc,[APP_MODLIST_PTR],eax
        mov           [APP_MODLIST_PTR],eax
.spaceavail:
        mov           ecx,[APP_MODLIST_PTR]
        mov           ebx,[APP_MAX_LINE]
        mov           eax,[l_sp]
        mov           [ecx+ebx*4],eax
        inc           [APP_MAX_LINE]
        ret
  endp

section '.rdata' data readable
align 32

APP_TITLE_NAME          db      'TwoWorlds 1 Mod Selector',0
CrashApp_Title          db      'Error',0

CrashReason_Default     db      'Unspecified Crash',0
CrashReason_NoMods      db      'No mod files found',0Ah,\
                                'Ensure the executable is in the game folder',0Ah,\
                                'and that there are mods in the Mods Folder',0
TXT_ENDL                db      0Ah,0
TXT_SPACE               db      ' ',0
FORM_INT                db      '%d',0
FORM_HEX                db      '%x',0
FORM_HEXLN              db      '%x',0Ah,0
FORM_HEX8               db      '%02x',0
FORM_HEX32              db      '%08x',0
FORM_STR                db      '%s',0
FORM_PRNT_2D            db      '%d,%d',0Ah,0
FORM_VT_SETCUR          db      VT_ESC,'[%d;%dH',0
FORM_VT_MTLINE          db      VT_ESC,'[%dd',0
FORM_VT_SETMARG         db      VT_ESC,'[%d;%dr',0
FORM_VT_SETTITLE        db      VT_ESC,']0;%s',VT_ESC,'\',0
FORM_LINENUBER          db      '==<%d/%d>==',0
; Paths
P_REG_ROOT = HKEY_CURRENT_USER
P_REG_MODS              db      'SOFTWARE\Reality Pump\TwoWorlds\Mods',0
;P_FILE_ABS              db      'D:\Games\TwoWorlds\Mods\',0
P_FILE_REL              db      'Mods\',0
P_FILE_SEARCH           db      '*.wd',0

; VTerm Sequences
VT_ESC = 1Bh
VT_CLEARSCR             db      VT_ESC,'[2J',0
VT_CLEARLINE            db      VT_ESC,'[2K',0
VT_NEWBUF               db      VT_ESC,'[?1049h',0
VT_RESETCUR             db      VT_ESC,'[1;1H',0  ; Cursor Positions start at 0
VT_CUR_UP               db      VT_ESC,'[A',0
VT_CUR_DOWN             db      VT_ESC,'[B',0
VT_CUR_BLINK            db      VT_ESC,'[?12h',0
VT_CUR_NOBLINK          db      VT_ESC,'[?12l',0
VT_CUR_SHOW             db      VT_ESC,'[?25h',0
VT_CUR_HIDE             db      VT_ESC,'[?25l',0
VT_COL_RESET            db      VT_ESC,'[0m',0
VT_VP_SCRDOWN           db      VT_ESC,'[S',0
VT_VP_SCRUP             db      VT_ESC,'[T',0
; OutputFormatting
FORM_ACTIVE_LINE        db      '+',VT_ESC,'[32m%s',VT_ESC,'[m',0
FORM_INACTIVE_LINE      db      ' ',VT_ESC,'[31m%s',VT_ESC,'[m',0
FORM_CUR_LINE           db      '>',0
FORM_OTH_LINE           db      ' ',0

section '.data' data readable writeable
align 32

AppExitCode             dd      0
CrashApp_Msg            dd      CrashReason_Default

;UNINITIALIZED SECTION
hInstance               dd      ?
hSTDOUT                 dd      ?
hSTDIN                  dd      ?
hOrgOutMode             dd      ?
hOrgInMode              dd      ?
AppStartInfo            STARTUPINFO
; TODO Window class
TXT_LineBuffer_A        rb  APP_MAX_LINE_BUFFER
APP_WIDTH               dd      ?
APP_HEIGHT              dd      ?
APP_VIEWOFF             dd      ?
APP_CUR_LINE            dd      ?
APP_MAX_LINE            dd      ?
APP_MODLIST_PTR         dd      ?
APP_ACTLIST_PTR         dd      ?
APP_ARR_AS              dd      ?
APP_MBHELD              dd      ?


section '.idata' import data readable writeable

  library kernel,       'KERNEL32.DLL',\
          user,         'USER32.DLL',\
          advapi,       'ADVAPI32.DLL',\
          \;shell,        'SHELL32.DLL',\
          msvc,         'MSVCRT.DLL'

  import kernel,\
         ExitProcess,                   'ExitProcess',\
         GetStartupInfo,                'GetStartupInfoA',\
         GetModuleHandle,               'GetModuleHandleA',\
         \;GetCommandLine,                'GetCommandLineW',\
         \;LocalFree,                     'LocalFree',\
         AllocConsole,                  'AllocConsole',\
         GetStdHandle,                  'GetStdHandle',\
         GetConsoleMode,                'GetConsoleMode',\
         SetConsoleMode,                'SetConsoleMode',\
         ReadConsoleInput,              'ReadConsoleInputA',\
         WriteConsole,                  'WriteConsoleA',\
         GetConsoleScreenBufferInfo,    'GetConsoleScreenBufferInfo',\
         SetConsoleWindowInfo,          'SetConsoleWindowInfo',\
         FindFirstFile,                 'FindFirstFileA',\
         FindNextFile,                  'FindNextFileA',\
         FindClose,                     'FindClose'

  import user,\
         MessageBox,            'MessageBoxA'

  import advapi,\
         RegCreateKey,          'RegCreateKeyExA',\ ; hKey, Subkey, reserved, lpclass, dwopt, samdes, lpsec, phkres, lpdwdisp
         RegCloseKey,           'RegCloseKey',\     ; hkey
         RegQueryValue,         'RegQueryValueExA',\; hkey, valname, reserved, lptype, lpdate, lpcbdata ; 0 = Success, check error
         RegSetValue,           'RegSetValueExA'    ; hkey, valname, reserved, dwtype, lpdata, cbdata

  ;import shell,\
  ;       CommandLineToArgv,     'CommandLineToArgvW'

  import msvc,\                 ;cinvoke all    ; args
         strlen,                'strlen',\      ; *buffer
         snprintf_s,            '_snprintf_s',\ ; *buffer, bufsize, cnt<buf, format, ...
         strcat_s,              'strcat_s',\    ; *dest, size, *src
         strcpy_s,              'strcpy_s',\    ; *dest, size, *src
         realloc,               'realloc',\     ; *handle, nsize
         free,                  'free',\        ; *handle
         malloc,                'malloc',\      ; size
         memset,                'memset'        ; *handle,val,size

; TODO section for fileversion info
; TODO section for manifest