/**
  ******************************************************************************
  * @file    keyboard.c
  * @author  Brett Brotherton
  * @version V1.0.0
  * @date    2013-11-03
  * @brief   Library to emulate a USB HID keyboard using 
  * STM32F4-Discovery sample code and drivers
  ******************************************************************************
  * @attention
  * This code was mostly adapted from aurdiuno USB library 
  * https://github.com/arduino/Arduino
  * 
  * Copyright (c) 2010, Peter Barrett   
  * Permission to use, copy, modify, and/or distribute this software for  
  * any purpose with or without fee is hereby granted, provided that the  
  * above copyright notice and this permission notice appear in all copies.  
  * 
  * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
  * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
  * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
  * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
  * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
  * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
  * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
  * SOFTWARE.
  * ******************************************************************************
*/

#include <stdint.h>
#include "main.h"
#include "usbd_hid_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "keyboard.h"

//constants
#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
        0x00,             // NUL
        0x00,             // SOH
        0x00,             // STX
        0x00,             // ETX
        0x00,             // EOT
        0x00,             // ENQ
        0x00,             // ACK  
        0x00,             // BEL
        0x2a,                        // BS        Backspace
        0x2b,                        // TAB        Tab
        0x28,                        // LF        Enter
        0x00,             // VT 
        0x00,             // FF 
        0x00,             // CR 
        0x00,             // SO 
        0x00,             // SI 
        0x00,             // DEL
        0x00,             // DC1
        0x00,             // DC2
        0x00,             // DC3
        0x00,             // DC4
        0x00,             // NAK
        0x00,             // SYN
        0x00,             // ETB
        0x00,             // CAN
        0x00,             // EM 
        0x00,             // SUB
        0x00,             // ESC
        0x00,             // FS 
        0x00,             // GS 
        0x00,             // RS 
        0x00,             // US 

        0x2c,                   //  ' '
        0x1e|SHIFT,           // !
        0x34|SHIFT,           // "
        0x20|SHIFT,    // #
        0x21|SHIFT,    // $
        0x22|SHIFT,    // %
        0x24|SHIFT,    // &
        0x34,          // '
        0x26|SHIFT,    // (
        0x27|SHIFT,    // )
        0x25|SHIFT,    // *
        0x2e|SHIFT,    // +
        0x36,          // ,
        0x2d,          // -
        0x37,          // .
        0x38,          // /
        0x27,          // 0
        0x1e,          // 1
        0x1f,          // 2
        0x20,          // 3
        0x21,          // 4
        0x22,          // 5
        0x23,          // 6
        0x24,          // 7
        0x25,          // 8
        0x26,          // 9
        0x33|SHIFT,      // :
        0x33,          // ;
        0x36|SHIFT,      // <
        0x2e,          // =
        0x37|SHIFT,      // >
        0x38|SHIFT,      // ?
        0x1f|SHIFT,      // @
        0x04|SHIFT,      // A
        0x05|SHIFT,      // B
        0x06|SHIFT,      // C
        0x07|SHIFT,      // D
        0x08|SHIFT,      // E
        0x09|SHIFT,      // F
        0x0a|SHIFT,      // G
        0x0b|SHIFT,      // H
        0x0c|SHIFT,      // I
        0x0d|SHIFT,      // J
        0x0e|SHIFT,      // K
        0x0f|SHIFT,      // L
        0x10|SHIFT,      // M
        0x11|SHIFT,      // N
        0x12|SHIFT,      // O
        0x13|SHIFT,      // P
        0x14|SHIFT,      // Q
        0x15|SHIFT,      // R
        0x16|SHIFT,      // S
        0x17|SHIFT,      // T
        0x18|SHIFT,      // U
        0x19|SHIFT,      // V
        0x1a|SHIFT,      // W
        0x1b|SHIFT,      // X
        0x1c|SHIFT,      // Y
        0x1d|SHIFT,      // Z
        0x2f,          // [
        0x31,          // bslash
        0x30,          // ]
        0x23|SHIFT,    // ^
        0x2d|SHIFT,    // _
        0x35,          // `
        0x04,          // a
        0x05,          // b
        0x06,          // c
        0x07,          // d
        0x08,          // e
        0x09,          // f
        0x0a,          // g
        0x0b,          // h
        0x0c,          // i
        0x0d,          // j
        0x0e,          // k
        0x0f,          // l
        0x10,          // m
        0x11,          // n
        0x12,          // o
        0x13,          // p
        0x14,          // q
        0x15,          // r
        0x16,          // s
        0x17,          // t
        0x18,          // u
        0x19,          // v
        0x1a,          // w
        0x1b,          // x
        0x1c,          // y
        0x1d,          // z
        0x2f|SHIFT,    // 
        0x31|SHIFT,    // |
        0x30|SHIFT,    // }
        0x35|SHIFT,    // ~
        0                                // DEL
};


static KeyReport _keyReport;
static USB_OTG_CORE_HANDLE *mycorehandle = NULL;
void keyboardInit(USB_OTG_CORE_HANDLE *corehandle) {
  mycorehandle = corehandle;
  _keyReport.keys[0] = 0;
  _keyReport.keys[1] = 0;        
  _keyReport.keys[2] = 0;
  _keyReport.keys[3] = 0;        
  _keyReport.keys[4] = 0;
  _keyReport.keys[5] = 0;        
  _keyReport.modifiers = 0;
  _keyReport.reserved = 0;
}

int keyboardPress(uint8_t k) 
{
        uint8_t i;
        if (k >= 136) {                        // it's a non-printing key (not a modifier)
                k = k - 136;
        } else if (k >= 128) {        // it's a modifier key
                _keyReport.modifiers |= (1<<(k-128));
                k = 0;
        } else {                                // it's a printing key
                k = _asciimap[k];
                if (!k) {
                        //setWriteError();
                        return 0;
                }
                if (k & 0x80) {                                                // it's a capital letter or other character reached with shift
                        _keyReport.modifiers |= 0x02;        // the left shift modifier
                        k &= 0x7F;
                }
        }
        
        // Add k to the key report only if it's not already present
        // and if there is an empty slot.
        if (_keyReport.keys[0] != k && _keyReport.keys[1] != k && 
                _keyReport.keys[2] != k && _keyReport.keys[3] != k &&
                _keyReport.keys[4] != k && _keyReport.keys[5] != k) {
                
                for (i=0; i<6; i++) {
                        if (_keyReport.keys[i] == 0x00) {
                                _keyReport.keys[i] = k;
                                break;
                        }
                }
                if (i == 6) {
                        //setWriteError();
                        return 0;
                }        
        }
        //sendReport(&_keyReport);
        USBD_HID_SendReport (mycorehandle, &(_keyReport.modifiers), 8);
        return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
int keyboardRelease(uint8_t k) 
{
        uint8_t i;
        if (k >= 136) {                        // it's a non-printing key (not a modifier)
                k = k - 136;
        } else if (k >= 128) {        // it's a modifier key
                _keyReport.modifiers &= ~(1<<(k-128));
                k = 0;
        } else {                                // it's a printing key
                k = _asciimap[k];
                if (!k) {
                        return 0;
                }
                if (k & 0x80) {                                                        // it's a capital letter or other character reached with shift
                        _keyReport.modifiers &= ~(0x02);        // the left shift modifier
                        k &= 0x7F;
                }
        }
        
        // Test the key report to see if k is present.  Clear it if it exists.
        // Check all positions in case the key is present more than once (which it shouldn't be)
        for (i=0; i<6; i++) {
                if (0 != k && _keyReport.keys[i] == k) {
                        _keyReport.keys[i] = 0x00;
                }
        }

        //sendReport(&_keyReport);
        USBD_HID_SendReport (mycorehandle, &(_keyReport.modifiers), 8);
        return 1;
}

void keyboardReleaseAll(void)
{
        _keyReport.keys[0] = 0;
        _keyReport.keys[1] = 0;        
        _keyReport.keys[2] = 0;
        _keyReport.keys[3] = 0;        
        _keyReport.keys[4] = 0;
        _keyReport.keys[5] = 0;        
        _keyReport.modifiers = 0;
        //sendReport(&_keyReport);
        USBD_HID_SendReport (mycorehandle, &(_keyReport.modifiers), 8);
}

int keyboardWrite(uint8_t c)
{        
        uint8_t p = keyboardPress(c);                // Keydown
        /*uint8_t r =*/ keyboardRelease(c);                // Keyup
        return (p);                                        // just return the result of press() since release() almost always returns 1
}


int keyboardPutString(char *str) {
  //first make sure we clear keyboard
  keyboardReleaseAll();
  //do a keyboard write for each character in the null terminated string
  int chars = 0;
  while (*str != 0) {
    chars += keyboardWrite(*str);
    str++;
  }
  return chars;
}
