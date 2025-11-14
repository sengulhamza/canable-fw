//
// can: initializes and provides methods to interact with the FDCAN peripheral
//

#include "stm32h7xx_hal.h"
#include "slcan.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "led.h"
#include "error.h"


// Private variables
static FDCAN_HandleTypeDef hfdcan1;
static FDCAN_FilterTypeDef filter;
static uint32_t prescaler;
static can_bus_state_t bus_state = OFF_BUS;
static uint8_t can_autoretransmit = ENABLE;
static can_txbuf_t txqueue = {0};


// Initialize FDCAN peripheral settings, but don't actually start the peripheral
void can_init(void)
{
    // Initialize GPIO for FDCAN transceiver 
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    //PD0     ------> FDCAN1_RX
    //PD1     ------> FDCAN1_TX
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Initialize CAN transceiver control pins
    //PC9     ------> CAN_NSTB (active low, give LOW to enable)
    //PC6     ------> CAN_DTR_EN
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // Enable CAN transceiver (NSTB is active low)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);  // NSTB = LOW (enabled)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);    // DTR_EN = HIGH


    // Initialize default FDCAN filter configuration
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0x0000;
    filter.FilterID2 = 0x0000;


    // default to 500 kbit/s
    prescaler = 5;  // 50MHz / 5 / 20 = 500kbps
    hfdcan1.Instance = FDCAN1;
    bus_state = OFF_BUS;

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);

}


// Start the FDCAN peripheral
void can_enable(void)
{
    if (bus_state == OFF_BUS)
    {
    	hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    	hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    	hfdcan1.Init.AutoRetransmission = can_autoretransmit ? ENABLE : DISABLE;
    	hfdcan1.Init.TransmitPause = DISABLE;
    	hfdcan1.Init.ProtocolException = DISABLE;
    	
    	// Nominal bit timing for classic CAN (20 TQ: 1 Sync + 13 Seg1 + 6 Seg2)
    	// Sample point at 70% (14/20)
    	hfdcan1.Init.NominalPrescaler = prescaler;
    	hfdcan1.Init.NominalSyncJumpWidth = 1;
    	hfdcan1.Init.NominalTimeSeg1 = 13;
    	hfdcan1.Init.NominalTimeSeg2 = 6;
    	
    	// Message RAM configuration
    	hfdcan1.Init.MessageRAMOffset = 0;
    	hfdcan1.Init.StdFiltersNbr = 1;
    	hfdcan1.Init.ExtFiltersNbr = 0;
    	hfdcan1.Init.RxFifo0ElmtsNbr = 16;
    	hfdcan1.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    	hfdcan1.Init.RxFifo1ElmtsNbr = 0;
    	hfdcan1.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
    	hfdcan1.Init.RxBuffersNbr = 0;
    	hfdcan1.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
    	hfdcan1.Init.TxEventsNbr = 0;
    	hfdcan1.Init.TxBuffersNbr = 0;
    	hfdcan1.Init.TxFifoQueueElmtsNbr = 16;
    	hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    	hfdcan1.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
    	
        HAL_FDCAN_Init(&hfdcan1);

        HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);
        HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

        HAL_FDCAN_Start(&hfdcan1);
        bus_state = ON_BUS;

        led_blue_on();
    }
}


// Disable the FDCAN peripheral and go off-bus
void can_disable(void)
{
    if (bus_state == ON_BUS)
    {
        HAL_FDCAN_Stop(&hfdcan1);
        bus_state = OFF_BUS;

        led_green_on();
    }
}


// Set the bitrate of the FDCAN peripheral
// FDCAN clock is 50 MHz (from PLL1Q), bit time = 20 TQ (1+13+6)
void can_set_bitrate(enum can_bitrate bitrate)
{
    if (bus_state == ON_BUS)
    {
        // cannot set bitrate while on bus
        return;
    }

    switch (bitrate)
    {
        case CAN_BITRATE_10K:
        	prescaler = 250;  // 50MHz / 250 / 20 = 10kbps
            break;
        case CAN_BITRATE_20K:
        	prescaler = 125;  // 50MHz / 125 / 20 = 20kbps
            break;
        case CAN_BITRATE_50K:
        	prescaler = 50;   // 50MHz / 50 / 20 = 50kbps
            break;
        case CAN_BITRATE_100K:
            prescaler = 25;   // 50MHz / 25 / 20 = 100kbps
            break;
        case CAN_BITRATE_125K:
            prescaler = 20;   // 50MHz / 20 / 20 = 125kbps
            break;
        case CAN_BITRATE_250K:
            prescaler = 10;   // 50MHz / 10 / 20 = 250kbps
            break;
        case CAN_BITRATE_500K:
            prescaler = 5;    // 50MHz / 5 / 20 = 500kbps
            break;
        case CAN_BITRATE_750K:
            prescaler = 4;    // 50MHz / 4 / 20 = 625kbps (closest to 750kbps)
            break;
        case CAN_BITRATE_1000K:
            prescaler = 2;    // 50MHz / 2 / 20 = 1250kbps (use prescaler 3 for ~833kbps if too high)
            break;
        case CAN_BITRATE_INVALID:
        default:
            prescaler = 5;    // default to 500kbps
            break;
    }

    led_green_on();
}


// Set FDCAN peripheral to silent mode
void can_set_silent(uint8_t silent)
{
    if (bus_state == ON_BUS)
    {
        // cannot set silent mode while on bus
        return;
    }
    if (silent)
    {
    	hfdcan1.Init.Mode = FDCAN_MODE_BUS_MONITORING;
    } else {
    	hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    }

    led_green_on();
}


// Enable/disable auto-retransmission
void can_set_autoretransmit(uint8_t autoretransmit)
{
    if (bus_state == ON_BUS)
    {
        // Cannot set autoretransmission while on bus
        return;
    }
    if (autoretransmit)
    {
    	can_autoretransmit = ENABLE;
    } else {
    	can_autoretransmit = DISABLE;
    }

    led_green_on();
}


// Send a message on the FDCAN bus
uint32_t can_tx(FDCAN_TxHeaderTypeDef *tx_msg_header, uint8_t* tx_msg_data)
{
	// Check if space available in the buffer (FIXME: wastes 1 item)
	if( ((txqueue.head + 1) % TXQUEUE_LEN) == txqueue.tail)
	{
		error_assert(ERR_FULLBUF_CANTX);
		return HAL_ERROR;
	}

	// Copy header struct into array
	txqueue.header[txqueue.head] = *tx_msg_header;

	// Copy data into array (DLC is in bytes for FDCAN)
	uint8_t data_length = (tx_msg_header->DataLength >> 16) & 0x0F;
	if (data_length > 8) data_length = 8;
	for(uint8_t i=0; i<data_length; i++)
	{
		txqueue.data[txqueue.head][i] = tx_msg_data[i];
	}

	// Increment the head pointer
	txqueue.head = (txqueue.head + 1) % TXQUEUE_LEN;

	return HAL_OK;
}


// Process messages in the TX output queue
void can_process(void)
{
    if((txqueue.tail != txqueue.head) && (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0))
	{
		// Transmit FDCAN frame
		uint32_t status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txqueue.header[txqueue.tail], txqueue.data[txqueue.tail]);
		txqueue.tail = (txqueue.tail + 1) % TXQUEUE_LEN;

		led_green_on();

		// This drops the packet if it fails (no retry). Failure is unlikely
		// since we check if there is a TX FIFO free.
		if(status != HAL_OK)
		{
			error_assert(ERR_CAN_TXFAIL);
		}
	}
}


// Receive message from the FDCAN bus RXFIFO
uint32_t can_rx(FDCAN_RxHeaderTypeDef *rx_msg_header, uint8_t* rx_msg_data)
{
    uint32_t status = HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, rx_msg_header, rx_msg_data);
	led_blue_on();
    return status;
}


// Check if an FDCAN message has been received and is waiting in the FIFO
uint8_t is_can_msg_pending(uint8_t fifo)
{
    if (bus_state == OFF_BUS)
    {
        return 0;
    }
    return(HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0);
}


// Return reference to FDCAN handle
FDCAN_HandleTypeDef* can_gethandle(void)
{
	return &hfdcan1;
}


// Callback for FIFO0 full
void HAL_FDCAN_RxFifo0FullCallback(FDCAN_HandleTypeDef *hfdcan)
{
	error_assert(ERR_CANRXFIFO_OVERFLOW);
}

