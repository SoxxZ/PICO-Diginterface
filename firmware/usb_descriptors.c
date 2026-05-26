#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
    _PID_MAP(MIDI, 3) | _PID_MAP(AUDIO, 4) | _PID_MAP(VENDOR, 5) )

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_HEADSET_MONO_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

#if CFG_TUSB_MCU == OPT_MCU_LPC175X_6X || CFG_TUSB_MCU == OPT_MCU_LPC177X_8X || CFG_TUSB_MCU == OPT_MCU_LPC40XX
  #define EPNUM_AUDIO_IN    0x03
  #define EPNUM_AUDIO_OUT   0x03
#elif CFG_TUSB_MCU == OPT_MCU_CXD56
  #define EPNUM_AUDIO_IN    0x01
  #define EPNUM_AUDIO_OUT   0x02
#elif CFG_TUSB_MCU == OPT_MCU_NRF5X
  #define EPNUM_AUDIO_IN    0x08
  #define EPNUM_AUDIO_OUT   0x08
#elif defined(TUD_ENDPOINT_ONE_DIRECTION_ONLY)
  #define EPNUM_AUDIO_IN    0x01
  #define EPNUM_AUDIO_OUT   0x02
#else
  #define EPNUM_AUDIO_IN    0x01
  #define EPNUM_AUDIO_OUT   0x01
#endif

#define EPNUM_CDC_NOTIF   0x03
#define EPNUM_CDC_OUT     0x04
#define EPNUM_CDC_IN      0x04

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_AUDIO_HEADSET_MONO_DESCRIPTOR(2, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN | 0x80),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_COM, 6, EPNUM_CDC_NOTIF | 0x80, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN | 0x80, 64),
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "TinyUSB",
    "PICO-Diginterface",
    NULL,
    "RP2040 Microphone",
    "RP2040 Speaker",
    "RP2040 CAT Control",
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;

    switch (index) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[0], 2);
            chr_count = 1;
            break;

        case STRID_SERIAL:
            chr_count = board_usb_get_serial(_desc_str + 1, 32);
            break;

        default:
            if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
                return NULL;

            const char *str = string_desc_arr[index];
            chr_count = strlen(str);
            size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
            if (chr_count > max_count) chr_count = max_count;

            for (size_t i = 0; i < chr_count; i++) {
                _desc_str[1 + i] = str[i];
            }
            break;
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
