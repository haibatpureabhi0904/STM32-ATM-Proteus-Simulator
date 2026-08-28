/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void lcd_init();
void lcd_cmd(unsigned char);
void lcd_data(unsigned char);
void lcd_str(const char *p);
char get_key(void);
void display_num(uint16_t num);
void show_seg(uint8_t pattern);
uint16_t read_joystick(void);
void lcd_set_cursor(uint8_t, uint8_t);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//for lcd data
int data_line[8] = { GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
		GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15 };

// Common Anode Lookup Table (0-9)
const uint8_t ca_map[10] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90  // 9
};

//----------------------------ATM MENU------------------------------------------
typedef enum {
	STATE_ENTER_PIN, STATE_MENU, STATE_BALANCE, STATE_WITHDRAW, STATE_TOKEN,
} Atm_state;

//--------------------------MENU OPTIONS------------------------------------------
const char *menu_options[] = { "1. Check Balance", "2. Withdraw Amount",
		"3. View Token" };
/* USER CODE END 0 */

int main(void) {

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_ADC1_Init();
	/* USER CODE BEGIN 2 */

	//----------initial status leds-------------------------
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

	//------------turn off 7 segment display -----------------------
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);

	//---------initialize lcd---------------------------------
	lcd_init();
	lcd_cmd(0x80);
	lcd_str("---ATM SYSTEM---");
	lcd_cmd(0xC0);
	lcd_str("Enter PIN: ");

//--------------application variables------------------------

	Atm_state current_state = STATE_ENTER_PIN;

	char entered_pin[5] = { 0 };   //store pin here
	uint8_t pin_index = 0;         //stored pin index

	uint16_t withdraw_amt = 0;    //withdraw amount store here

	uint16_t balance = 9000;
	uint16_t seg_display_val = 0;    //multiplex the 7 segment display initially
	uint16_t token_no = 1;          //initial token number
	uint32_t last_joy_time = 0;          //joystick time
	int menu_sel = 0;           //atm menu index: 0. Balance 1.Withdraw 2. Token

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		// 1. Continuous multiplexed display on 7-segment (~40ms per iteration)
		for (int i = 0; i < 5; i++) {
			display_num(seg_display_val);
		}

		// 2. Read Pressed Key
		char key = get_key();

		// 3. ATM states machine

		switch (current_state) {
		/*-----------------------------------------------------------------------------------------
		 ===================================STATE 1. PIN ENTRY===================================*/
		case STATE_ENTER_PIN:
			seg_display_val = 0;
			if (key >= '0' && key <= '9') {
				if (pin_index < 4) {
					entered_pin[pin_index] = key;
					pin_index++;
					HAL_Delay(100);
					lcd_data('*');
				}
			} else if (key == 'O') {    // Fixed: Moved outside digit check
				pin_index = 0;
				memset(entered_pin, 0, sizeof(entered_pin));
				lcd_set_cursor(1, 11);
				lcd_str("    ");
				lcd_set_cursor(1, 11);
			}

			if (pin_index == 4) {
				HAL_Delay(300);
				if (strcmp(entered_pin, "1234") == 0) {
					current_state = STATE_MENU;
					menu_sel = 0;
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // Green LED ON
					HAL_Delay(200);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
					lcd_cmd(0x01);
					lcd_set_cursor(0, 0);
					lcd_str(menu_options[menu_sel]);
					lcd_set_cursor(1, 0);
					lcd_str("Joystick + '='");
				} else {
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); // Red Error LED ON
					lcd_cmd(0x01);
					lcd_set_cursor(0, 0);
					lcd_str("Wrong PIN! Retry");
					HAL_Delay(1200);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
					pin_index = 0;
					memset(entered_pin, 0, sizeof(entered_pin));
					lcd_cmd(0x01);
					lcd_set_cursor(0, 0);
					lcd_str("---ATM SYSTEM---");
					lcd_set_cursor(1, 0);
					lcd_str("Enter PIN: ");
				}
			}
			break;

			/*----------------------------------------------------------------------------------------------
			 ===================================STATE 2. MENU NAVIGATION===================================*/
		case STATE_MENU:
			if (HAL_GetTick() - last_joy_time > 250) {
				uint16_t joy_val = read_joystick();
				last_joy_time = HAL_GetTick();

				int prev_sel = menu_sel;

				if (joy_val > 3000) { // Joystick UP
					if (menu_sel > 0)
						menu_sel--;
				} else if (joy_val < 1000) { // Joystick DOWN
					if (menu_sel < 2)
						menu_sel++;
				}

				// Redraw LCD only when menu item actually changes
				if (prev_sel != menu_sel) {
					lcd_cmd(0x01);
					lcd_set_cursor(0, 0);
					lcd_str(menu_options[menu_sel]);
					lcd_set_cursor(1, 0);
				}
			}
			//===============select option menu by selecting "=" on keypad or switch on PA7=====================================

			if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_RESET || key == '=') {
				HAL_Delay(200);
				lcd_cmd(0x01);

				if (menu_sel == 0) {                    //check balance
					current_state = STATE_BALANCE;
					seg_display_val = balance;
					lcd_set_cursor(0, 0);
					lcd_str("A/c Balance:");
					lcd_set_cursor(1, 0);
					char buf[16];
					sprintf(buf, "$%u (O:Back)", balance); // Shows Balance on LCD
					lcd_str(buf);
				} else if (menu_sel == 1) {               //withdraw amount
					current_state = STATE_WITHDRAW;
					withdraw_amt = 0;
					lcd_set_cursor(0, 0);
					lcd_str("Enter Amnt:");
					lcd_set_cursor(1, 0);
					lcd_str("=:OK   O:Cancel");
					lcd_set_cursor(0, 12);
					seg_display_val = 0;
				} else if (menu_sel == 2) {               //view token
					current_state = STATE_TOKEN;
					token_no++;
					if (token_no > 9999) {
						token_no = 1;
					}
					seg_display_val = token_no;
					lcd_str("Token Number: ");
					lcd_set_cursor(1, 0);
					char buf[16];
					sprintf(buf, "Token: %04u", token_no);
					lcd_str(buf);
				}
			}
			break;

			/*----------------------------------------------------------------------------------------------
			 ====================================STATE 3. VIEW BALANCE===================================*/
		case STATE_BALANCE:
			seg_display_val = balance;

			if (key == 'O' || HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_RESET) {
				current_state = STATE_MENU;
				lcd_cmd(0x01);
				char buf[6];
				sprintf(buf, "%04u", balance);
				lcd_str(buf);
				lcd_set_cursor(0, 0);
				lcd_str(menu_options[menu_sel]);
				lcd_set_cursor(1, 0);
				lcd_str("Joystick + '='");
				seg_display_val = 0;
			}
			break;

			/*----------------------------------------------------------------------------------------------
			 ===================================STATE 4. WITHDRAW AMOUNT===================================*/
		case STATE_WITHDRAW:
			display_num(token_no);
			if (key >= '0' && key <= '9') {
				if (withdraw_amt < 1000) {
					withdraw_amt = (withdraw_amt * 10) + (key - '0');
					seg_display_val = withdraw_amt;               //<----------------------------------------

					lcd_set_cursor(0, 12);
					char buf[6];
					sprintf(buf, "%04u", withdraw_amt);
					lcd_str(buf);
				}
			} else if (key == '=') {
				lcd_cmd(0x01);
				if (withdraw_amt > 0 && withdraw_amt <= balance) {
					balance -= withdraw_amt;
					seg_display_val = balance;                  //<------------------------------------
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);     //green light flash
					HAL_Delay(100);
					lcd_set_cursor(0, 0);
					lcd_str("Withdraw Success");
					lcd_set_cursor(1, 0);
					lcd_str("Please Take Cash");
					HAL_Delay(100);
					lcd_cmd(0x01);
					char buf[16];
					sprintf(buf, "Token: %04u", token_no);
					lcd_str(buf);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);
				}
				else {
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
					HAL_Delay(100);
					lcd_set_cursor(0, 0);
					lcd_str("Invalid Amount");
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);   //rred light off
				}
				HAL_Delay(1500);
				current_state = STATE_MENU;
				lcd_cmd(0x01);
				lcd_set_cursor(0, 0);
				lcd_str(menu_options[menu_sel]);
				lcd_set_cursor(1, 0);
				seg_display_val = 0;

			} else if (key == 'O') {
				current_state = STATE_MENU;
				seg_display_val = 0;
				lcd_cmd(0x01);
				lcd_set_cursor(0, 0);
				lcd_str(menu_options[menu_sel]);
				lcd_set_cursor(1, 0);
				lcd_str("Joystick + '='");
			}
			break;

			/*----------------------------------------------------------------------------------------------
			 ===================================STATE 5. SHOW TOKEN========================================*/
		case STATE_TOKEN:
			seg_display_val = token_no;
			if (key == 'O' || HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_RESET) {
				current_state = STATE_MENU;
				lcd_cmd(0x01);
				lcd_set_cursor(0, 0);
				lcd_str(menu_options[menu_sel]);
				lcd_set_cursor(1, 0);
				lcd_str("Joystick + '='");
				seg_display_val = 0;
			}
			break;
		}

	}
}
/*===================================================================================================
 ------------------------------------------READ JOYSTICK---------------------------------------------*/

uint16_t read_joystick(void) {
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	uint16_t value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	return value;
}
/*==================================================================================================
 ---------------------------------------SEGMENT DISPLAY--------------------------------------------*/

void show_seg(uint8_t pattern) {
	//check for segment a
	if ((pattern & 0x01) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	}

	//check for segment b
	if ((pattern & 0x02) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	}

	//check for segment c
	if ((pattern & 0x04) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
	}

	//check for segment d
	if ((pattern & 0x08) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	}

	//check for segment e
	if ((pattern & 0x10) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	}

	//check for segment f
	if ((pattern & 0x20) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	}

	//check for segment g
	if ((pattern & 0x40) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	}

	//check for segment dp
	if ((pattern & 0x80) != 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
	}
}

/*==========================================================================================================
 ----------------------------------------------------DISPLAY NUMBER-----------------------------------------*/

void display_num(uint16_t num) {
    uint8_t dg1 = (num / 1000) % 10;
    uint8_t dg2 = (num / 100) % 10;
    uint8_t dg3 = (num / 10) % 10;
    uint8_t dg4 = num % 10;

    // ==================== DIGIT 1 (Thousands) ====================
    // 1. Turn OFF all digit anodes
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    // 2. Clear all segment cathodes (all HIGH = all segments OFF in Common Anode)
    show_seg(0xFF);
    // 3. Load segment pattern
    show_seg(ca_map[dg1]);
    // 4. Turn ON Digit 1 anode
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(2);

    // ==================== DIGIT 2 (Hundreds) ====================
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    show_seg(0xFF);
    show_seg(ca_map[dg2]);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(2);

    // ==================== DIGIT 3 (Tens) ====================
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    show_seg(0xFF);
    show_seg(ca_map[dg3]);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(2);

    // ==================== DIGIT 4 (Units) ====================
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    show_seg(0xFF);
    show_seg(ca_map[dg4]);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(2);

    // Turn off Digit 4 and clear segments before leaving
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    show_seg(0xFF);
}

/*===========================================================================================================
 -------------------------------------------KEYPAD INITIALIZATION--------------------------------------------*/

char get_key(void) {
	//-----------SCAN FOR ROW 1---------------------
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); //c0=0, c1=c2=c3=1
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_PIN_SET);

	//----if button 1 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '7';
	}

	//----if button 5 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '8';
	}

	//----if button 9 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '9';
	}

	//----if button 13 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '/';
	}

	//-----------SCAN FOR ROW 2---------------------
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); //c1=0, c0=c2=c3=1
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_PIN_SET);

	//----if button 2 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '4';
	}

	//----if button 6 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '5';
	}

	//----if button 10 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '6';
	}

	//----if button 14 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return 'X';
	}

	//-----------SCAN FOR ROW 3---------------------
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); //c2=0, c1=c0=c3=1
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_3,
			GPIO_PIN_SET);

	//----if button 3 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '1';
	}

	//----if button 7 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '2';
	}

	//----if button 11 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '3';
	}

	//----if button 15 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '-';
	}

	//-----------SCAN FOR ROW 4---------------------
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); //c3=0, c1=c2=c0=1
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_0,
			GPIO_PIN_SET);

	//----if button 4 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return 'O';
	}

	//----if button 8 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '0';
	}

	//----if button 12 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '=';
	}

	//----if button 16 is pressed-----
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) {
		HAL_Delay(100);
		return '+';
	}
	return '\0';
}

/*===============================================================================================================
 -----------------------------------------------LCD INITIALIZATION------------------------------------------------*/
void lcd_init(void) {
	HAL_Delay(50);
	lcd_cmd(0x38);
	HAL_Delay(1);
	lcd_cmd(0x0C);
	HAL_Delay(1);
	lcd_cmd(0x06);
	HAL_Delay(1);
	lcd_cmd(0x01);
	HAL_Delay(1);
}

/*===============================================================================================================
 -------------------------------------------------LCD COMMAND----------------------------------------------------*/

void lcd_cmd(unsigned char c) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);  //rs=0
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  //rw=0
	for (int i = 0; i < 8; i++) {
		HAL_GPIO_WritePin(GPIOA, data_line[i], (c >> i) & 1);
	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  //en=1
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  //en=0
}
/*=================================================================================================================
 ---------------------------------------------------LCD DATA---------------------------------------------------*/

void lcd_data(unsigned char d) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);  //rs=1
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  //rw=0
	for (int i = 0; i < 8; i++) {
		HAL_GPIO_WritePin(GPIOA, data_line[i], (d >> i) & 1);
	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  //en=1
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  //en=0
}

/*=================================================================================================================
 -------------------------------------------------------LCD STRING------------------------------------------------*/

void lcd_str(const char *p) {
	while (*p != '\0') {
		lcd_data(*p);
		p++;
	}
}

/*=================================================================================================================
 ------------------------------------------------------LCD SET CURSOR-------------------------------------------*/

void lcd_set_cursor(uint8_t row, uint8_t col) {
	if (row == 0) {
		lcd_cmd(0x80 + col);
	} else {
		lcd_cmd(0xC0 + col);
	}
}

/*================================================================================================================*/
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8
					| GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA,
			GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | LD2_Pin | GPIO_PIN_6
					| GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
					| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4
					| GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PC0 PC1 PC2 PC3
	 PC8 PC9 PC10 PC11 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
			| GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PA2 PA3 PA4 LD2_Pin
	 PA6 PA8 PA9 PA10
	 PA11 PA12 PA13 PA14
	 PA15 */
	GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | LD2_Pin
			| GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
			| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PA7 */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PC4 PC5 PC6 PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PB0 PB1 PB2 PB3
	 PB4 PB5 PB6 PB7 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
			| GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf('Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
