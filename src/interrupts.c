//
// interrupts: handle global system interrupts
//

#include "stm32h7xx_hal.h"
#include "interrupts.h"
#include "can.h"
#include "led.h"



// Externs
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;



void NMI_Handler(void)
{
	while(1);
}

void HardFault_Handler(void)
{
	while(1);
}


// Handle USB OTG HS (in FS mode) interrupts
void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}


// Handle SysTick interrupt
void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}


// Handle FDCAN interrupts
void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(can_gethandle());
}

void FDCAN1_IT1_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(can_gethandle());
}
