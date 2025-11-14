//
// CANable firmware
//

#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "slcan.h"
#include "system.h"
#include "led.h"
#include "error.h"


int main(void)
{
    // Initialize peripherals
    system_init();
    can_init();
    led_init();
    usb_init();

    led_blue_blink(2);
    
    // Turn on blue LED to indicate USB is connected
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);

    // Send test message to CAN bus for 5 seconds on startup
    uint32_t test_start = HAL_GetTick();
    uint8_t test_sent = 0;
    
    // Storage for status and received message buffer
    FDCAN_RxHeaderTypeDef rx_msg_header;
    uint8_t rx_msg_data[8] = {0};
    uint8_t msg_buf[SLCAN_MTU];


    while(1)
    {
        cdc_process();
        led_process();
        can_process();
        
        // Send test CAN message for first 5 seconds
        if(!test_sent && (HAL_GetTick() - test_start < 5000))
        {
            // Send test message every 500ms
            static uint32_t last_test = 0;
            if(HAL_GetTick() - last_test > 500)
            {
                FDCAN_TxHeaderTypeDef test_header;
                test_header.Identifier = 0x123;
                test_header.IdType = FDCAN_STANDARD_ID;
                test_header.TxFrameType = FDCAN_DATA_FRAME;
                test_header.DataLength = 8 << 16;  // 8 bytes
                test_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
                test_header.BitRateSwitch = FDCAN_BRS_OFF;
                test_header.FDFormat = FDCAN_CLASSIC_CAN;
                test_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
                test_header.MessageMarker = 0;
                
                uint8_t test_data[8] = {0x54, 0x45, 0x53, 0x54, 0x00, 0x00, 0x00, 0x00}; // "TEST"
                can_tx(&test_header, test_data);
                
                last_test = HAL_GetTick();
            }
        }
        else if(!test_sent)
        {
            test_sent = 1;  // Mark test as complete after 5 seconds
        }

        // If CAN message receive is pending, process the message
        if(is_can_msg_pending(FDCAN_RX_FIFO0))
        {
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK)
			{
				uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);

				// Transmit message via USB-CDC
				if(msg_len)
				{
					CDC_Transmit_FS(msg_buf, msg_len);
				}
			}
        }
    }
}

