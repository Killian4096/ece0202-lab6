#include "stm32l476xx.h"
#include "I2C.h"
#include "ssd1306.h"


#include <string.h>
#include <stdio.h>

void System_Clock_Init(void);
void RTC_Clock_Init(void);
void RTC_Init(void);
void I2C_GPIO_init(void);
void DisplayString(char* messge);
char date[] = "123456";



void DisplayString(char* message){

	ssd1306_Fill(White);
	ssd1306_SetCursor(2,0);
	ssd1306_WriteString(message, Font_11x18, Black);
	ssd1306_UpdateScreen();	

	
}



void SysTick_Handler(void){
	uint32_t ht = (0x30 & RTC_TIME_GetHour()) >> 4;
	uint32_t hu = RTC_TIME_GetHour() & 0xF;
	uint32_t mnt = (0x70 & RTC_TIME_GetMinute()) >> 4;
	uint32_t mnu = 0xF & RTC_TIME_GetMinute();
	uint32_t st = (0x70 & RTC_TIME_GetSecond()) >> 4;
	uint32_t su = 0xF & RTC_TIME_GetSecond();
	char time[32];
	sprintf(time, "%d%d|%d%d|%d%d", ht,hu,mnt,mnu,st,su);
	DisplayString(time);
	
	// Trigger alarm when new minute
	if (su == 00) {
		// Turn on LED
		GPIOA->ODR |= 1UL << 5; // Turn on LED via output 1
	} else {
		// Turn off LED
		GPIOA->ODR &= ~(1UL << 5); // Turn off LED via output 0
	}
}

/*
void RTC_Alarm_IRQHandler(void) {
	// Check if LED should be on
	uint32_t su = 0xF & RTC_TIME_GetSecond();
	
	if (su == 00) {
		// Turn on LED
		GPIOA->ODR |= 1UL << 5; // Turn on LED via output 1
	} else {
		// Turn off LED
		GPIOA->ODR &= ~(1UL << 5); // Turn off LED via output 0
	}
	
	// Clear flags
	RTC->ISR &= ~(RTC_ISR_ALRAF);
	RTC->ISR &= ~(RTC_ISR_ALRBF);
}
*/

void RTC_Setup(void){
	//RTC SETUP
	//Provided Functions
	RTC_Clock_Init();
	RTC_Disable_Write_Protection();
	
	//Enable Initialization
	RTC->ISR |= RTC_ISR_INIT;
	
	//Wait for intitialization flag to set
	while((RTC->ISR & RTC_ISR_INITF) == 0){
	;}
	
	
	//Set RTC
	RTC_Set_Time(0x1, 0x13, 0x59, 0x50); 
	
	//Close up shop
	RTC->ISR &= ~RTC_ISR_INIT;
	RTC_Enable_Write_Protection();
	//END RTC SETUP
}

void SysTick_Setup(void){
	//FIND FREQUENCY
	//disable IRQ
	SysTick->CTRL &= ~(1<<SysTick_CTRL_TICKINT_Pos);
	
	/*const unsigned long int INTERRUPT_TICKS = 10;
	const unsigned long int TIMER_FREQUENCY = 10;*/
	//Period, hardcoding for now, need to redo
	uint32_t Interrupt_Period = (8000000/1000) - 1;
	SysTick->LOAD = Interrupt_Period;
	
	SysTick->VAL = 0;
	
	//Clocksource processor
	SysTick->CTRL |= (1<<SysTick_CTRL_CLKSOURCE_Pos);
	
	//Enable interrupts
	SysTick->CTRL |= (1<<SysTick_CTRL_TICKINT_Pos);
	SysTick->CTRL |= (1<<SysTick_CTRL_ENABLE_Pos);

}
	
/*
// Function to handle setting an alarm
void Alarm_Setup(void) {
	// Step 1: Disable write protection
	RTC_Disable_Write_Protection();
	
	// Step 2: Disable alarm A & B
	RTC->CR |= ~(RTC_CR_ALRAE);
	RTC->CR |= ~(RTC_CR_ALRBE);
	
	// Step 3: Wait for ALRMAR and ALRMBR to be editable
	while ((RTC->ISR & RTC_ISR_ALRAWF) > 0) {
		// Do nothing
	}
	while ((RTC->ISR & RTC_ISR_ALRBWF) > 0) {
		// Do nothing
	}
	
	// Step 4: Configure alarm
	// Set alarm A to trigger on every minute (when seconds are 00)
	RTC->ALRMAR = 0xC0808000;
	// Set alarm B to trigger 1 second after alarm A (when seconds are 01)
	RTC->ALRMBR = 0xC0808001;
	
	// Step 5: Re-enable alarm A and B
	RTC->CR |= RTC_CR_ALRAE;
	RTC->CR |= RTC_CR_ALRBE;
	
	// Step 6: Enable write protection
	RTC_Enable_Write_Protection();
	
	// Enable interrupts
	
}

*/
void GPIO_Setup(void) {
	// Configure PA5
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Enable clock of Port A
	GPIOA->MODER &= ~(3UL<<10);          // Clear mode bits
	GPIOA->MODER |= 1UL<<10;             // Set mode to output
	GPIOA->OTYPER &= ~(1UL<<5);          // Select push-pull output
}
	

int main(void){
	
	// Enable High Speed Internal Clock (HSI = 16 MHz)
  RCC->CR |= ((uint32_t)RCC_CR_HSION);
	
  // wait until HSI is ready
  while ( (RCC->CR & (uint32_t) RCC_CR_HSIRDY) == 0 ) {
	;}
	
  // Select HSI as system clock source 
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSI;  //01: HSI16 oscillator used as system clock

  // Wait till HSI is used as system clock source 
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) == 0 ) {
	;}
	

	NVIC_SetPriority(SysTick_IRQn, 1);		// Set Priority to 1
	NVIC_EnableIRQ(SysTick_IRQn);					// Enable EXTI0_1 interrupt in NVIC
	
	//SETUP
	System_Clock_Init();
	I2C_GPIO_init();
	I2C_Initialization(I2C1);
	ssd1306_Init();
	RTC_Setup();
	SysTick_Setup();
	GPIO_Setup();
	//Alarm_Setup();
	
	
	
	
	
	



  // Dead loop & program hangs here
	while(1){	}
}




	
