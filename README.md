<img width="1680" height="846" alt="Screenshot 2026-08-29 010525" src="https://github.com/user-attachments/assets/508a2206-e6f4-4d27-8a7d-b6de71bd4d5d" />
<img width="1688" height="847" alt="Screenshot 2026-08-29 010349" src="https://github.com/user-attachments/assets/0d5c417c-4e8f-4a16-8dfc-8454151256e4" />
<img width="1685" height="846" alt="Screenshot 2026-08-29 010252" src="https://github.com/user-attachments/assets/425ddea4-c98a-4cbb-9022-9ba22b0ce162" />
<img width="1691" height="845" alt="Screenshot 2026-08-29 010138" src="https://github.com/user-attachments/assets/fbc3b9a8-deba-4012-adca-bb91da84a4ca" />
# STM32 Automated Teller Machine (ATM) System Simulator

A bare-metal STM32 microcontroller firmware implementation of an interactive ATM interface featuring a Finite State Machine (FSM), 4x4 matrix keypad input, an 8-bit parallel 16x2 alphanumeric LCD, a multiplexed 4-digit common anode 7-segment display, an analog joystick navigation control, and LED status indicators.

Designed and tested for STM32F4 series microcontrollers with complete schematic support for Proteus VSM simulation.

---

## Features

- **Finite State Machine Architecture:** Clean state transitions across `STATE_ENTER_PIN`, `STATE_MENU`, `STATE_BALANCE`, `STATE_WITHDRAW`, and `STATE_TOKEN`.
- **PIN Verification & Security:** Masked 4-digit PIN input with error state latching, retry logic, and LED feedback.
- **Dynamic Menu Navigation:** Analog joystick vertical control with debounce timing thresholds and button-triggered selection.
- **Transaction Processing:** Balance inquiry, validated cash withdrawal logic with boundary checking, and dynamic token generation.
- **Display Interfaces:**
  - **16x2 Character LCD (8-bit Mode):** Text prompts, transaction status, and navigation menus.
  - **4-Digit 7-Segment (Common Anode):** High-refresh multiplexed numeric output for balances, withdrawal entries, and token tracking.

---

## Hardware & Peripheral Mapping

### STM32CubeMX Pin Configuration

| Pin | Peripheral / Mode | Pull / Level | Description |
| :--- | :--- | :--- | :--- |
| **PA0** | `ADC1_IN0` (Analog) | None | Joystick Y-Axis Analog In |
| **PA2** | `GPIO_Output` | Push-Pull | LCD Register Select (`RS`) |
| **PA3** | `GPIO_Output` | Push-Pull | LCD Read/Write Select (`RW`) |
| **PA4** | `GPIO_Output` | Push-Pull | LCD Enable (`EN`) |
| **PA5** | `GPIO_Output` | Push-Pull | Success Status LED (Green) |
| **PA6** | `GPIO_Output` | Push-Pull | Error Status LED (Red) |
| **PA7** | `GPIO_Input` | Pull-Up | Push Button / Menu Enter (`ACTIVE LOW`) |
| **PA8 - PA15** | `GPIO_Output` | Push-Pull | LCD Data Bus (`D0 - D7`) |
| **PB0 - PB7** | `GPIO_Output` | Push-Pull | 7-Segment Cathode Lines (`a, b, c, d, e, f, g, dp`) |
| **PC0 - PC3** | `GPIO_Output` | Push-Pull | Keypad Column Drives (`Col 1 - 4`) |
| **PC4 - PC7** | `GPIO_Input` | Pull-Up | Keypad Row Sense Lines (`Row 1 - 4`) |
| **PC8 - PC11** | `GPIO_Output` | Push-Pull | 7-Segment Anode Multiplex Control (`Digit 1 - 4`) |

------------------------------------------------------------------------------------------------------------

## Circuit Schematic (Proteus VSM)

### 1. 16x2 LCD (LM016L) — 8-Bit Interface
```text
STM32 Pin          LCD Pin           Function
-------------------------------------------------------------
GND        ----->  Pin 1 (VSS)       Ground
+5V        ----->  Pin 2 (VDD)       Supply Voltage (+5V)
POT WIPER  ----->  Pin 3 (VEE/V0)    Contrast (10k Potentiometer)
PA2        ----->  Pin 4 (RS)        Register Select
-----------------------------------------------------------------------------------------------------------
Segment Cathodes (via 330Ω Resistors):
  PB0 -> Seg A  |  PB1 -> Seg B  |  PB2 -> Seg C  |  PB3 -> Seg D
  PB4 -> Seg E  |  PB5 -> Seg F  |  PB6 -> Seg G  |  PB7 -> Seg DP
------------------------------------------------------------------------------------------------------------
Digit Anodes (Active HIGH):
  PC8 -> Digit 1 (Thousands)     PC9 -> Digit 2 (Hundreds)
  PC10 -> Digit 3 (Tens)         PC11 -> Digit 4 (Units)
-------------------------------------------------------------------------------------------------------------
PA3        ----->  Pin 5 (RW)        Read / Write (GND / Write Mode)
PA4        ----->  Pin 6 (E)         Clock Enable
PA8 - PA15 ----->  Pin 7 - 14        Data Lines D0 to D7
--------------------------------------------------------------------------------------------------------------
STM32 Pin          Matrix Line       Connected Keys
-------------------------------------------------------------
PC4 (In / PU) ---> Row 1             '7'  '8'  '9'  '/'
PC5 (In / PU) ---> Row 2             '4'  '5'  '6'  'X'
PC6 (In / PU) ---> Row 3             '1'  '2'  '3'  '-'
PC7 (In / PU) ---> Row 4             'O'  '0'  '='  '+'
PC0 (Out)     ---> Col 1             Drive Low for Col 1 scan
PC1 (Out)     ---> Col 2             Drive Low for Col 2 scan
PC2 (Out)     ---> Col 3             Drive Low for Col 3 scan
PC3 (Out)     ---> Col 4             Drive Low for Col 4 scan
--------------------------------------------------------------------------------------------------------------
                      +-------------------+
                      |  STATE_ENTER_PIN  | <-------+ (Cancel / Timeout)
                      +-------------------+         |
                                | (Valid PIN: 1234) |
                                v                   |
                      +-------------------+         |
         +----------> |    STATE_MENU     | --------+
         |            +-------------------+
         |              /       |       \
(Back / 'O')          /         |         \
         |          v           v           v
   +---------------+   +----------------+   +---------------+
   | STATE_BALANCE |   | STATE_WITHDRAW |   |  STATE_TOKEN  |
   +---------------+   +----------------+   +---------------+
