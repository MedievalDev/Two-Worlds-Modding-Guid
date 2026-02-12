IR_KEY_EVENT                 = 0x01
IR_MOUSE_EVENT               = 0x02
IR_WINDOW_BUFFER_SIZE_EVENT  = 0x04
IR_MENU_EVENT                = 0x08
IR_FOCUS_EVENT               = 0x10

struct IR_KEY_EVENT_RECORD
  bKeyDown           dd ?
  wRepeatCount       dw ?
  wVirtualKeyCode    dw ?
  wVirtualScanCode   dw ?
  union
   UnicodeChar        dw ?
   AsciiChar          db ?
  ends
  dwControlKeyState  dd ?
ends
struct COORD
     x       dw ?
     y       dw ?
ends
struct SMALL_RECT
     Left    dw ?
     Top     dw ?
     Right   dw ?
     Bottom  dw ?
ends
IR_M_LMB        = 1
IR_M_RMB        = 2
IR_M_DOUBLE     = 2

struct IR_MOUSE_EVENT_RECORD
  dwMousePosition          COORD
  dwButtonState            dd ?
  dwControlKeyState        dd ?
  dwEventFlags             dd ?
ends
struct INPUT_RECORD
     EventType dw ?
     _pad      dw ?
     union
      KeyEvent      IR_KEY_EVENT_RECORD   ; KeyEvent
      MouseEvent    IR_MOUSE_EVENT_RECORD ; MouseEvent
      dwSize        COORD                 ; WindowBufferSizeEvent
      dwCommandId   dd ?                  ; MenuEvent
      bSetFocus     dd ?                  ; FocusEvent
     ends
ends
struct CONSOLE_SCREEN_BUFFER_INFO
     dwSize                     COORD
     dwCursorPosition           COORD
     wAttributes                dw      ?
     srWindow                   SMALL_RECT
     dwMaximumWindowSize        COORD
ends