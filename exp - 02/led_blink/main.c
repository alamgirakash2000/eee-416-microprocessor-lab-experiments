#include "stm32f446xx.h"

/* Board name: NUCLEO-F446RE

 PA.5  <--> Green LED (LD2)
 PC.13 <--> Blue user button (B1)
 
 Base Header Code by Dr. Sajid Muhaimin Choudhury, Department of EEE, BUET 22/06/2022
 
 Based on Instructor Companion of Yifeng Zhu
*/
#define A1 6 
#define B1 7
#define A2 8 
#define B2 9

#define BUTTON_PIN 13

#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field. 
                                   This value must be a multiple of 0x200. */
/*
 User HSI (high-speed internal) as the processor clock
 See Page 94 on Reference Manual to see the clock tree
 HSI Clock: 16 Mhz, 1% accuracy at 25 oC
 Max Freq of AHB: 84 MHz
 Max Freq of APB2: 84 MHZ
 Max Freq of APB1: 42 MHZ
 SysTick Clock = AHB Clock / 8
*/

static void enable_HSI(){
	
	/* Enable Power Control clock */
	/* RCC->APB1ENR |= RCC_APB1LPENR_PWRLPEN; */
	
	// Regulator voltage scaling output selection: Scale 2 
	// PWR->CR |= PWR_CR_VOS_1;
	
	// Enable High Speed Internal Clock (HSI = 16 MHz)
	RCC->CR |= ((uint32_t)RCC_CR_HSION);
	while ((RCC->CR & RCC_CR_HSIRDY) == 0); // Wait until HSI ready
	
	// Store calibration value
	PWR->CR |= (uint32_t)(16 << 3);
	
	// Reset CFGR register 
	RCC->CFGR = 0x00000000;

 	// Reset HSEON, CSSON and PLLON bits 
 	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
	while ((RCC->CR & RCC_CR_PLLRDY) != 0); // Wait until PLL disabled
	
	// Programming PLLCFGR register 
	// RCC->PLLCFGR = 0x24003010; // This is the default value

	// Tip: 
	// Recommended to set VOC Input f(PLL clock input) / PLLM to 1-2MHz
	// Set VCO output between 192 and 432 MHz, 
	// f(VCO clock) = f(PLL clock input) × (PLLN / PLLM)
	// f(PLL general clock output) = f(VCO clock) / PLLP
	// f(USB OTG FS, SDIO, RNG clock output) = f(VCO clock) / PLLQ
 	
	RCC->PLLCFGR = 0;
	RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLSRC); 		// PLLSRC = 0 (HSI 16 Mhz clock selected as clock source)
	RCC->PLLCFGR |= 16 << RCC_PLLCFGR_PLLN_Pos; 	// PLLM = 16, VCO input clock = 16 MHz / PLLM = 1 MHz
	RCC->PLLCFGR |= 336 << RCC_PLLCFGR_PLLN_Pos; 	// PLLN = 336, VCO output clock = 1 MHz * 336 = 336 MHz
	RCC->PLLCFGR |= 4 << RCC_PLLCFGR_PLLP_Pos; 	// PLLP = 4, PLLCLK = 336 Mhz / PLLP = 84 MHz
	RCC->PLLCFGR |= 7 << RCC_PLLCFGR_PLLQ_Pos; 	// PLLQ = 7, USB Clock = 336 MHz / PLLQ = 48 MHz

	// Enable Main PLL Clock
	RCC->CR |= RCC_CR_PLLON; 
	while ((RCC->CR & RCC_CR_PLLRDY) == 0);  // Wait until PLL ready
	
	
	// FLASH configuration block
	// enable instruction cache, enable prefetch, set latency to 2WS (3 CPU cycles)
	FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_2WS;

	// Configure the HCLK, PCLK1 and PCLK2 clocks dividers
	// AHB clock division factor
	RCC->CFGR &= ~RCC_CFGR_HPRE; // 84 MHz, not divided
	// PPRE1: APB Low speed prescaler (APB1)
	RCC->CFGR &= ~RCC_CFGR_PPRE1; 
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2; // 42 MHz, divided by 2
	// PPRE2: APB high-speed prescaler (APB2)
	RCC->CFGR &= ~RCC_CFGR_PPRE2; // 84 MHz, not divided
	
	// Select PLL as system clock source 
	// 00: HSI oscillator selected as system clock
	// 01: HSE oscillator selected as system clock
	// 10: PLL selected as system clock
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_1;
	// while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL);

  	// Configure the Vector Table location add offset address 
//	VECT_TAB_OFFSET  = 0x00UL; // Vector Table base offset field. 
                                   // This value must be a multiple of 0x200. 
  	SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; // Vector Table Relocation in Internal FLASH 

}




static void configure_STEPPER_pin(){
  // Enable the clock to GPIO Port A	
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   
		
	// GPIO Mode: Input(00), Output(01), AlterFunc(10), Analog(11, reset)
	GPIOA->MODER &= ~(3UL<<(2*A1));  
	GPIOA->MODER |=   1UL<<(2*A1);      // Output(01)
	
	GPIOA->MODER &= ~(3UL<<(2*B1));  
	GPIOA->MODER |=   1UL<<(2*B1);      // Output(01)
	
	GPIOA->MODER &= ~(3UL<<(2*A2));  
	GPIOA->MODER |=   1UL<<(2*A2);      // Output(01)

	GPIOA->MODER &= ~(3UL<<(2*B2));  
	GPIOA->MODER |=   1UL<<(2*B2);      // Output(01)
	
	// GPIO Speed: Low speed (00), Medium speed (01), Fast speed (10), High speed (11)
	GPIOA->OSPEEDR &= ~(3U<<(2*A1));
	GPIOA->OSPEEDR |=   2U<<(2*A1);  // Fast speed
	
	GPIOA->OSPEEDR &= ~(3U<<(2*B1));
	GPIOA->OSPEEDR |=   2U<<(2*B1);  // Fast speed
	
	GPIOA->OSPEEDR &= ~(3U<<(2*A2));
	GPIOA->OSPEEDR |=   2U<<(2*A2);  // Fast speed
	
	GPIOA->OSPEEDR &= ~(3U<<(2*B2));
	GPIOA->OSPEEDR |=   2U<<(2*B2);  // Fast speed
	
	// GPIO Output Type: Output push-pull (0, reset), Output open drain (1) 
	GPIOA->OTYPER &= ~(1U<<A1);      // Push-pull
	GPIOA->OTYPER &= ~(1U<<B1);      // Push-pull
	GPIOA->OTYPER &= ~(1U<<A2);      // Push-pull
	GPIOA->OTYPER &= ~(1U<<B2);      // Push-pull
	
	// GPIO Push-Pull: No pull-up, pull-down (00), Pull-up (01), Pull-down (10), Reserved (11)
	GPIOA->PUPDR  &= ~(3U<<(2*A1));  // No pull-up, no pull-down
	GPIOA->PUPDR  &= ~(3U<<(2*B1));  // No pull-up, no pull-down
	GPIOA->PUPDR  &= ~(3U<<(2*A2));  // No pull-up, no pull-down
	GPIOA->PUPDR  &= ~(3U<<(2*B2));  // No pull-up, no pull-down
	
}


static void turn_on_A1(){
	GPIOA->ODR |= 1U << A1;
}

static void turn_on_B1(){
	GPIOA->ODR |= 1U << B1;
}
static void turn_on_A2(){
	GPIOA->ODR |= 1U << A2;
}

static void turn_on_B2(){
	GPIOA->ODR |= 1U << B2;
}


static void turn_off_A1(){
	GPIOA->ODR &= ~(1U << A1);
}
static void turn_off_B1(){
	GPIOA->ODR &= ~(1U << B1);
}
static void turn_off_A2(){
	GPIOA->ODR &= ~(1U << A2);
}
static void turn_off_B2(){
	GPIOA->ODR &= ~(1U << B2);
}


/*static void toggle_LED(){
	GPIOA->ODR ^= (1 << LED_PIN);
}
*/

int main(void){
	uint32_t i;
	uint32_t delay;
	delay = 100000;
	
	enable_HSI();
	configure_STEPPER_pin();
	turn_off_A1();
	turn_off_B1();
	turn_off_A2();
	turn_off_B2();

	
  // Dead loop & program hangs here
	while(1){
		  
		  for(i=0; i<delay; i++); // simple delay
			turn_on_A1();
			turn_off_B1();
			turn_off_A2();
			turn_off_B2();
		
		  for(i=0; i<delay; i++); // simple delay
			turn_off_A1();
			turn_on_B1();
			turn_off_A2();
			turn_off_B2();
		
		  for(i=0; i<delay; i++); // simple delay
			turn_off_A1();
			turn_off_B1();
			turn_on_A2();
			turn_off_B2();
		
		  for(i=0; i<delay; i++); // simple delay
			turn_off_A1();
			turn_off_B1();
			turn_off_A2();
			turn_on_B2();
		
	}
}

