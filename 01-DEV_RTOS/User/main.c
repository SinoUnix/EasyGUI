/**
 * Keil project example for GSM SIM800/900 for SMS and RTOS support
 *
 * @note      Check defines.h file for configuration settings!
 * @note      When using Nucleo F411 board, example has set 8MHz external HSE clock!
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 *
 * \par Description
 *
 * This examples shows how you can receive SMS and read it.
 * If you send SMS with specific content, it will do actions with LED on board:
 *
 * - LED ON: led will turn on,
 * - LED OFF: Led will turn off,
 * - LED TOGGLE: Led will toggle.
 *
 * After that, SMS with "OK" or "ERROR" should be returned to received number to confirm action
 *
 * \par Pinout for example (Nucleo STM32F411)
 *
\verbatim
GSM         STM32F4xx           DESCRIPTION
 
RX          PA9                 TX from STM to RX from GSM
TX          PA10                RX from STM
VCC         3.3V                Use external 3.3V regulator
GND         GND
RST         PA0
CTS         PA3                 RTS from ST to CTS from GSM
            BUTTON(PA0, PC13)   Discovery/Nucleo button, depends on configuration
            
            PA2                 TX for debug purpose (connect to PC) with 921600 bauds
\endverbatim
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#include "tm_stm32_touch.h"
#include "tm_stm32_exti.h"
#include "cmsis_os.h"
#include "tm_stm32_general.h"

#include "gui.h"
#include "gui_window.h"
#include "gui_button.h"
#include "gui_led.h"
#include "gui_progbar.h"
#include "gui_graph.h"
#include "gui_edittext.h"
#include "gui_checkbox.h"
#include "gui_radio.h"
#include "gui_listbox.h"

#include "math.h"

#define COUNT_OF(x)     (sizeof(x) / sizeof((x)[0]))

TM_TOUCH_t TS;

GUI_HANDLE_p win1, win2, win3, wins[3];
GUI_HANDLE_p btn1, btn2, btn3, btn4, btn5, btn6;
GUI_HANDLE_p led[8];
GUI_HANDLE_p prog1, prog2, prog3, prog4;
GUI_HANDLE_p graph1, graph2, graph3;
GUI_HANDLE_p edit1, edit2, edit3;
GUI_HANDLE_p cb1, cb2;
GUI_HANDLE_p rb[4];
GUI_HANDLE_p lb1;

GUI_GRAPH_DATA_p graphdata1, graphdata2, graphdata3, graphdata4;

char str[100];

extern GUI_Const GUI_FONT_t GUI_Font_Comic_Sans_MS_Regular_22;
extern GUI_Const GUI_FONT_t GUI_Font_Calibri_Bold_8;
extern GUI_Const GUI_FONT_t GUI_Font_Arial_Bold_18;
extern GUI_Const GUI_FONT_t GUI_Font_FontAwesome_Regular_30;
extern GUI_Const GUI_FONT_t GUI_Font_Arial_Narrow_Italic_22;

uint32_t time;

GUI_TIMER_t *tim1, *tim2;

GUI_Char* listboxtexts[] = {
    _T("Item 1"),
    _T("Item 2"),
    _T("Item 3"),
    _T("Item 4"),
    _T("Item 5"),
    _T("Item 6"),
    _T("Item 7"),
    _T("Item 8"),
};

uint8_t edittext_callback(GUI_HANDLE_p h, GUI_WC_t cmd, void* param, void* result) {
    if (cmd == GUI_WC_ValueChanged) {
        __GUI_DEBUG("Value changed!\r\n");
        return 1;
    } else {
        return GUI_WIDGET_ProcessDefaultCallback(h, cmd, param, result);
    }
}

uint8_t checkbox_callback(GUI_HANDLE_p h, GUI_WC_t cmd, void* param, void* result) {
    uint8_t ret = GUI_WIDGET_ProcessDefaultCallback(h, cmd, param, result);
    if (cmd == GUI_WC_Click) {
        if (GUI_CHECKBOX_IsChecked(h)) {
            TM_DISCO_LedOn(LED_ALL);
        } else {
            TM_DISCO_LedOff(LED_ALL);
        }
    }
    return ret;
}

int main(void) {
    GUI_STRING_UNICODE_t s;
    
    GUI_KeyboardData_t key;
    uint32_t state;
    
    static float i = 0, len = 72, radius = 90;
    float x, y;
    
    TM_RCC_InitSystem();                                    /* Init system */
    HAL_Init();                                             /* Init HAL layer */
    TM_DISCO_LedInit();                                     /* Init leds */
    TM_DISCO_ButtonInit();                                  /* Init button */
    TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DISCO_USART, DISCO_USART_PP, 115200);     /* Init USART for debug purpose */
    
    /* Print first screen message */
    printf("GUI; Compiled: %s %s, sizeof: %d\r\n", __DATE__, __TIME__, sizeof(char *) * 5);
    
    TM_GENERAL_DWTCounterEnable();
    
    GUI_Init();
    
    win1 = GUI_WINDOW_GetDesktop();                         /* Get desktop window */
    
    /* Button */
    btn1 = GUI_BUTTON_Create(1, 10, 10, 120, 40, 0, 0);
    GUI_WIDGET_SetFont(btn1, &GUI_Font_Arial_Narrow_Italic_22);
    GUI_WIDGET_AllocTextMemory(btn1, 255);
    GUI_WIDGET_SetText(btn1, _T("Button"));
    
    /* Edit text */
    edit1 = GUI_EDITTEXT_Create(1, 140, 10, 120, 40, 0, 0);
    GUI_WIDGET_SetFont(edit1, &GUI_Font_Arial_Narrow_Italic_22);
    GUI_WIDGET_AllocTextMemory(edit1, 255);
    GUI_WIDGET_SetText(edit1, _T("Edit text"));
    GUI_WIDGET_SetCallback(edit1, edittext_callback);
    
    #define PI      3.14159265359f
    
    /* Graph */
    graph1 = GUI_GRAPH_Create(0, 270, 10, 200, 150, 0, 0);
 
    GUI_GRAPH_SetMinX(graph1, -100);
    GUI_GRAPH_SetMaxX(graph1, 100);
    GUI_GRAPH_SetMinY(graph1, -100);
    GUI_GRAPH_SetMaxY(graph1, 100);
    
    graphdata1 = GUI_GRAPH_DATA_Create(GUI_GRAPH_TYPE_XY, len);
    graphdata2 = GUI_GRAPH_DATA_Create(GUI_GRAPH_TYPE_YT, len / 2);
    
    for (i = 0; i <= 360; i += 360 / len) {
        x = cos((float)i * (PI / 180.0f));
        y = sin((float)i * (PI / 180.0f));
        GUI_GRAPH_DATA_AddValue(graphdata1, x * radius, y * radius);
        GUI_GRAPH_DATA_AddValue(graphdata2, x * radius / 3, y * radius / 4);
    }
    
    graphdata1->Color = GUI_COLOR_RED;
    graphdata2->Color = GUI_COLOR_GREEN;
   
    GUI_GRAPH_AttachData(graph1, graphdata1);
    GUI_GRAPH_AttachData(graph1, graphdata2);
    
    /* Progress bar */
    prog1 = GUI_PROGBAR_Create(2, 10, 60, 120, 40, 0, 0);
    GUI_WIDGET_SetFont(prog1, &GUI_Font_Arial_Narrow_Italic_22);
    GUI_WIDGET_SetText(prog1, _T("Progbar"));
    
    /* Checkbox */
    cb1 = GUI_CHECKBOX_Create(1, 140, 60, 60, 40, 0, 0);
    GUI_WIDGET_SetCallback(cb1, checkbox_callback);
    GUI_WIDGET_SetFont(cb1, &GUI_Font_Arial_Narrow_Italic_22);
    GUI_WIDGET_SetText(cb1, _T("CB1"));
    cb2 = GUI_CHECKBOX_Create(1, 200, 60, 60, 40, 0, 0);
    GUI_WIDGET_SetFont(cb2, &GUI_Font_Arial_Narrow_Italic_22);
    GUI_WIDGET_SetText(cb2, _T("CB1"));
    
    /* Radio control */
    for (state = 0; state < 4; state++) {
        rb[state] = GUI_RADIO_Create(10, 10, 110 + (state * 30), 60, 25, 0, 0);
        GUI_WIDGET_SetFont(rb[state], &GUI_Font_Arial_Narrow_Italic_22);
        GUI_WIDGET_SetText(rb[state], _T("RB1"));
        GUI_RADIO_SetGroup(rb[state], state % 2);
        GUI_RADIO_SetValue(rb[state], state);
    }
    
    /* Leds */
    led[0] = GUI_LED_Create(0, 10, 242, 20, 20, 0, 0);
    led[1] = GUI_LED_Create(0, 40, 242, 20, 20, 0, 0);
    GUI_LED_Set(led[0], 1);
    GUI_LED_SetType(led[1], GUI_LED_TYPE_CIRCLE);
    
    /* Listbox */
    lb1 = GUI_LISTBOX_Create(1, 80, 110, 180, 150, NULL, 0);
    GUI_WIDGET_SetFont(lb1, &GUI_Font_Comic_Sans_MS_Regular_22);
    for (state = 0; state < COUNT_OF(listboxtexts); state++) {
        GUI_LISTBOX_AddString(lb1, listboxtexts[state]);
    }
    GUI_LISTBOX_SetSliderAuto(lb1, 0);
    GUI_LISTBOX_SetSliderVisibility(lb1, 1);
    
    __GUI_LINKEDLIST_PrintList(NULL);

    TM_EXTI_Attach(GPIOI, GPIO_PIN_13, TM_EXTI_Trigger_Rising);
    TS.Orientation = 1;
    TM_TOUCH_Init(NULL, &TS);
    
    GUI_STRING_UNICODE_Init(&s);
  
//    time = TM_DELAY_Time();
    state = 0;
	while (1) {
        GUI_Process();
        
        if ((TM_DELAY_Time() - time) >= 50) {
            time = TM_DELAY_Time();
            
            //__GUI_TIMER_Start(edit1->Timer);
            
//            if (state % 2) {
//                GUI_WINDOW_SetActive(win2);
//            } else {
//                GUI_WINDOW_SetActive(win3);
//            }
//            __GUI_LINKEDLIST_PrintList(NULL);
            state++;
            GUI_PROGBAR_SetValue(prog1, state % 100);
            
            x = cos((float)i * (PI / 180.0f));
            y = sin((float)i * (PI / 180.0f));
            GUI_GRAPH_DATA_AddValue(graphdata2, x * radius / 3, y * radius / 4);
            i += 360.0f / len;
        }
        
        while (!TM_USART_BufferEmpty(DISCO_USART)) {
            GUI_Char ch = TM_USART_Getc(DISCO_USART);
            __GUI_DEBUG("Key: %c (%2X)\r\n", ch, ch);
            switch (GUI_STRING_UNICODE_Decode(&s, ch)) {
                case UNICODE_OK:
                    if (ch == '.') {
                        ch = GUI_KEY_DOWN;
                    } else if (ch == ',') {
                        ch = GUI_KEY_UP;
                    }
                    key.Keys[s.t - 1] = ch;
                    GUI_INPUT_KeyAdd(&key);
                    key.Keys[0] = 0;
                    GUI_INPUT_KeyAdd(&key);
                
                    if (ch == 'D') {
                        GUI_LISTBOX_DeleteLastString(lb1);
                    } else if (ch == 'A') {
                        GUI_LISTBOX_AddString(lb1, _T("Test"));
                    } else if (ch == 'C') {
                        GUI_LISTBOX_SetString(lb1, 2, _T("Changed"));
                    } else if (ch == 'B') {
                        GUI_LISTBOX_Scroll(lb1, 1);
                    } else if (ch == 'V') {
                        GUI_LISTBOX_Scroll(lb1, -1);
                    }
                    break;
                case UNICODE_PROGRESS:
                    key.Keys[s.t - s.r - 1] = ch;
                default:
                    break;
            }
        }
	}
}

/* 1ms handler */
void TM_DELAY_1msHandler() {
    //osSystickHandler();                             /* Kernel systick handler processing */
    
    GUI_UpdateTime(1);
}

/* printf handler */
int fputc(int ch, FILE* fil) {
    TM_USART_Putc(DISCO_USART, ch);
    return ch;
}

/**
 * Handle EXTI interrupt routine
 */
void TM_EXTI_Handler(uint16_t GPIO_Pin) {
    static GUI_TouchData_t p = {0}, t = {0};
    if (GPIO_Pin == GPIO_PIN_13) {
        uint8_t i, update = 0, diffx, diffy;
        TM_TOUCH_Read(&TS);                         /* Read touch data */
        
        memset((void *)&t, 0x00, sizeof(t));
        t.Status = TS.NumPresses ? GUI_TouchState_PRESSED : GUI_TouchState_RELEASED;
        t.Count = TS.NumPresses > GUI_TOUCH_MAX_PRESSES ? GUI_TOUCH_MAX_PRESSES : TS.NumPresses;
        for (i = 0; i < t.Count; i++) {
            t.X[i] = TS.X[i];
            t.Y[i] = TS.Y[i];
        }
        if (t.Count != p.Count) {
            update = 1;
        } else {
            for (i = 0; i < t.Count; i++) {
                diffx = __GUI_ABS(p.X[i] - t.X[i]);
                diffy = __GUI_ABS(p.Y[i] - t.Y[i]);
                if (diffx > 3 || diffy > 3) {
                    update = 1;
                    break;
                }
            }
        }
        
        /* Check differences */
        if (update || t.Status == GUI_TouchState_RELEASED) {
            GUI_INPUT_TouchAdd(&t);
            memcpy(&p, &t, sizeof(p));
        }
    }
}
