// =============================================================================
// tusb_config.h - TinyUSB Configuration for Pico Coil Winder
// Based on TinyUSB configuration requirements
// =============================================================================

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// Enable device stack
#define CFG_TUD_ENABLED 1

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

// Root Hub Port Configuration (must be defined before tusb.h is included)
#define CFG_TUSB_RHPORT0_MODE 0x01  // OPT_MODE_DEVICE from TinyUSB

// CDC Configuration
#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256

// Interface count
#define CFG_TUD_INTERFACE_MAX 2

// Endpoint count
#define CFG_TUD_ENDPOINT_MAX 3

// Buffer size
#define CFG_TUD_BUF_SIZE 1024

// VID/PID
#define CFG_TUD_VENDOR_CODE 0x01
#define CFG_TUD_PRODUCT_CODE 0x01

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
