/******************************************************************************
* 
* File Name: eink_task.c
*
* Description: This file contains task and functions related to the of E-Ink
* that demonstrates controlling a EInk display using the EmWin Graphics Library.
* The project displays a start up screen with
* text "CYPRESS EMWIN GRAPHICS DEMO EINK DISPLAY".
*
* The project then displays the following screens in a loop
*
*   1. A screen showing LED is in on mode
*   2. A screen showing LED is in blink mode
*   3. A screen showing LED is in off mode
*
*******************************************************************************
* The code is modified based on the example code provided by Infineon
* Example Code Name: emWin E-Ink FreeRTOS
* Modifier: Yen-Chen Chang @ makestraight
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "GUI.h"
#include "cy8ckit_028_epd_pins.h"
#include "mtb_e2271cs021.h"
#include "LCDConf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "images.h"

/*******************************************************************************
* Global variables
*******************************************************************************/
/* HAL SPI object to interface with display driver */
cyhal_spi_t spi; 

/* Configuration structure defining the necessary pins to communicate with
 * the E-ink display */
const mtb_e2271cs021_pins_t pins =
{
    .spi_mosi = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MOSI,
    .spi_miso = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MISO,
    .spi_sclk = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_SCLK,
    .spi_cs = CY8CKIT_028_EPD_PIN_DISPLAY_CS,
    .reset = CY8CKIT_028_EPD_PIN_DISPLAY_RST,
    .busy = CY8CKIT_028_EPD_PIN_DISPLAY_BUSY,
    .discharge = CY8CKIT_028_EPD_PIN_DISPLAY_DISCHARGE,
    .enable = CY8CKIT_028_EPD_PIN_DISPLAY_EN,
    .border = CY8CKIT_028_EPD_PIN_DISPLAY_BORDER,
    .io_enable = CY8CKIT_028_EPD_PIN_DISPLAY_IOEN,
};


/*******************************************************************
*
* The points of the triangle
*/
static const GUI_POINT aPointTriangle[] = {
 { 10,  0},
 {  0,  5},
 {  0, -5}
};

/* Buffer to the previous frame written on the display */
uint8_t previous_frame[PV_EINK_IMAGE_SIZE] = {0};

/* Pointer to the new frame that need to be written */
uint8_t *current_frame;

/*******************************************************************************
* Macros
*******************************************************************************/
#define DELAY_AFTER_STARTUP_SCREEN_MS       (2000)
#define AMBIENT_TEMPERATURE_C               (20)
#define SPI_BAUD_RATE_HZ                    (20000000)

/*******************************************************************************
* Forward declaration
*******************************************************************************/
void show_startup_screen(void);
void show_instructions_screen(void);
void wait_for_switch_press_and_release(void);
void clear_screen(void);

void show_led_on(void);
void show_led_off(void);
void show_led_blink(void);

/*******************************************************************************
* Function Name: void show_startup_screen(void)
********************************************************************************
*
* Summary: This function displays the startup screen with
*           the demo description text
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void show_startup_screen(void)
{
    /* Set foreground and background color and font size */
    GUI_SetFont(GUI_FONT_16B_1);
    GUI_SetColor(GUI_BLACK);
    GUI_SetBkColor(GUI_WHITE);
    GUI_Clear();

    //GUI_DrawBitmap(&bmCypressLogoFullColor_PNG_1bpp, 2, 2);
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("CYPRESS", 132, 85);
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("EMWIN GRAPHICS", 132, 105);
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("EINK DISPLAY DEMO", 132, 125);
}


/*******************************************************************************
* Function Name: void show_instructions_screen(void)
********************************************************************************
*
* Summary: This function shows screen with instructions to press SW2 to
*           scroll through various display pages
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void show_instructions_screen(void)
{
    /* Set font size, background color and text mode */
    GUI_SetFont(GUI_FONT_16B_1);
    GUI_SetBkColor(GUI_WHITE);
    GUI_SetColor(GUI_BLACK);
    GUI_SetTextMode(GUI_TM_NORMAL);

    /* Clear the display */
    GUI_Clear();

    /* Display instructions text */
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("PRESS SW2 ON THE KIT", 132, 58);
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("TO SWITCH ", 132, 78);
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("LED mode!", 132, 98);
}

/*******************************************************************************
* Function Name: void show_led_on(void)
********************************************************************************
*
* Summary: This function shows screen with information that LED is now on
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void show_led_on(void)
{
    /* Set font size, background color and text mode */
    GUI_SetFont(GUI_FONT_32B_1);
    GUI_SetBkColor(GUI_WHITE);
    GUI_SetColor(GUI_BLACK);
    GUI_SetTextMode(GUI_TM_NORMAL);

    /* Clear the display */
    GUI_Clear();

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("LED", 132, 5);

    GUI_SetFont(GUI_FONT_16B_1);
    GUI_DispStringAt("ON", 110, 58);
    GUI_DispStringAt("OFF", 110, 78);
    GUI_DispStringAt("BLINK", 110, 98);

    // draw arrows
    GUI_FillPolygon(&aPointTriangle[0], 3, 90, 65);

}
/*******************************************************************************
* Function Name: void show_led_off(void)
********************************************************************************
*
* Summary: This function shows screen with information that LED is now off
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void show_led_off(void)
{
    /* Set font size, background color and text mode */
    GUI_SetFont(GUI_FONT_32B_1);
    GUI_SetBkColor(GUI_WHITE);
    GUI_SetColor(GUI_BLACK);
    GUI_SetTextMode(GUI_TM_NORMAL);

    /* Clear the display */
    GUI_Clear();

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("LED", 132, 5);

    GUI_SetFont(GUI_FONT_16B_1);
    GUI_DispStringAt("ON", 110, 58);
    GUI_DispStringAt("OFF", 110, 78);
    GUI_DispStringAt("BLINK", 110, 98);

    // draw arrows
    GUI_FillPolygon(&aPointTriangle[0], 3, 90,85);
}
/*******************************************************************************
* Function Name: void show_led_blink(void)
********************************************************************************
*
* Summary: This function shows screen with information that LED is now blinking
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void show_led_blink(void)
{
    /* Set font size, background color and text mode */
    GUI_SetFont(GUI_FONT_32B_1);
    GUI_SetBkColor(GUI_WHITE);
    GUI_SetColor(GUI_BLACK);
    GUI_SetTextMode(GUI_TM_NORMAL);

    /* Clear the display */
    GUI_Clear();

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("LED", 132, 5);

    GUI_SetFont(GUI_FONT_16B_1);
    GUI_DispStringAt("ON", 110, 58);
    GUI_DispStringAt("OFF", 110, 78);
    GUI_DispStringAt("BLINK", 110, 98);

    // draw arrows
    GUI_FillPolygon(&aPointTriangle[0], 3, 90, 105);
}


/*******************************************************************************
* Function Name: void clear_screen(void)
********************************************************************************
*
* Summary: This function clears the screen
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void clear_screen(void)
{
    GUI_SetColor(GUI_BLACK);
    GUI_SetBkColor(GUI_WHITE);
    GUI_Clear();
}


/*******************************************************************************
* Function Name: void wait_for_switch_press_and_release(void)
********************************************************************************
*
* Summary: This implements a simple "Wait for button press and release"
*           function.  It first waits for the button to be pressed and then
*           waits for the button to be released.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Side Effects:
*  This is a blocking function and exits only on a button press and release
*
*******************************************************************************/
void wait_for_switch_press_and_release(void)
{
    /* Wait for SW2 to be pressed */
    while( CYBSP_BTN_PRESSED != cyhal_gpio_read(CYBSP_USER_BTN));

    /* Wait for SW2 to be released */
    while( CYBSP_BTN_PRESSED == cyhal_gpio_read(CYBSP_USER_BTN));
}

/*******************************************************************************
* Function Name: void eInk_task(void *arg)
********************************************************************************
*
* Summary: Following functions are performed
*           1. Initialize the EmWin library
*           2. Display the startup screen for 2 seconds
*           3. Display the instruction screen and wait for key press and release
*           4. Inside a while loop scroll through the 3 pages on every
*               key press and release
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void eInk_task(void *arg)
{
    cy_rslt_t result;
    uint8_t page_number = 0;

    /* Array of demo pages functions */
    void (*ledPageArray[])(void) = {
    		show_led_on,
			show_led_blink,
			show_led_off
    };

    uint8_t num_of_demo_pages = (sizeof(ledPageArray)/sizeof(ledPageArray[0]));

    /* Configure Switch and LEDs*/
    cyhal_gpio_init( CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 
                     CYBSP_BTN_OFF);
    cyhal_pwm_t pwm_obj;
    int duty_cycle = 100;

    // Initialize PWM on the supplied pin and assign a new clock
    result = cyhal_pwm_init(&pwm_obj, CYBSP_USER_LED, NULL);

    // Set a duty cycle of 100% and frequency of 1Hz to initialize LED as off mode
    result = cyhal_pwm_set_duty_cycle(&pwm_obj, duty_cycle, 1);

    // Start the PWM output
    result = cyhal_pwm_start(&pwm_obj);
    
    /* Initialize SPI and EINK display */
    result = cyhal_spi_init(&spi, CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MOSI,
            CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MISO,
            CY8CKIT_028_EPD_PIN_DISPLAY_SPI_SCLK, NC, NULL, 8,
            CYHAL_SPI_MODE_00_MSB, false);
    if (CY_RSLT_SUCCESS == result)
    {
        result = cyhal_spi_set_frequency(&spi, SPI_BAUD_RATE_HZ);
        if (CY_RSLT_SUCCESS == result)
        {
            result = mtb_e2271cs021_init(&pins, &spi);

            /* Set ambient temperature, in degree C, in order to perform temperature
             * compensation of E-INK parameters */
            mtb_e2271cs021_set_temp_factor(AMBIENT_TEMPERATURE_C);

            current_frame = (uint8_t*)LCD_GetDisplayBuffer();

            /* Initialize EmWin driver*/
            GUI_Init();

            /* Show the startup screen */
            show_startup_screen();

            /* Update the display */
            mtb_e2271cs021_show_frame(previous_frame, current_frame,
                                      MTB_E2271CS021_FULL_4STAGE, true);

            vTaskDelay(DELAY_AFTER_STARTUP_SCREEN_MS);

            /* Show the instructions screen */
            show_instructions_screen();

            /* Update the display */
            mtb_e2271cs021_show_frame(previous_frame, current_frame,
                                      MTB_E2271CS021_FULL_4STAGE, true);

            wait_for_switch_press_and_release();

            for(;;)
            {
            	duty_cycle = (duty_cycle+50) % 150;
                cyhal_pwm_set_duty_cycle(&pwm_obj, duty_cycle, 1);
                cyhal_pwm_start(&pwm_obj);

                /* Using page_number as index, update the display with a demo screen
                */
                (*ledPageArray[page_number])();

                /* Update the display */
                // Set the update mode as 2 stage to make it more smooth
                mtb_e2271cs021_show_frame(previous_frame, current_frame,
                                          MTB_E2271CS021_FULL_2STAGE, true);


                /* Wait for a switch press event */
                wait_for_switch_press_and_release();

                /* Cycle through demo pages */
                page_number = (page_number+1) % num_of_demo_pages;
            }
        }
    }
}
