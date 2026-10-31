/*
 * Descritores USB do adaptador Atari Jaguar -> USB (RP2040).
 *
 * O relatório HID tem 5 bytes, com o mesmo layout do projeto original
 * raphnet/jaguar_usb (2 eixos de 8 bits + 17 botões):
 *   byte 0: X   (0x00 = esquerda, 0x80 = centro, 0xFF = direita)
 *   byte 1: Y   (0x00 = cima,     0x80 = centro, 0xFF = baixo)
 *   byte 2: botões 1..8   (A, B, C, Option, Pause, teclas 1, 2, 3)
 *   byte 3: botões 9..16  (teclas 4..9, *, 0)
 *   byte 4: botão 17      (tecla #), bits 1..7 = padding
 */

#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"

/* VID/PID de teste do TinyUSB. Troque se for distribuir o adaptador.
 * NÃO use o VID/PID da raphnet. */
#define USB_VID 0xCafe
#define USB_PID 0x4A47

static const uint8_t desc_hid_report[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x15, 0x00,       //     Logical Minimum (0)
    0x26, 0xFF, 0x00, //     Logical Maximum (255)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x02,       //     Report Count (2)
    0x81, 0x02,       //     Input (Data, Variable, Absolute)
    0xC0,             //   End Collection
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (Button 1)
    0x29, 0x11,       //   Usage Maximum (Button 17)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x11,       //   Report Count (17)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x07,       //   Report Count (7)
    0x81, 0x03,       //   Input (Constant) - padding
    0xC0              // End Collection
};

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

#define EPNUM_HID         0x81
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    // interface 0, sem boot protocol, endpoint IN 0x81, 8 bytes, intervalo de 1 ms
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, 8, 1)
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

static char serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *string_desc_arr[] = {
    (const char[]){0x09, 0x04},   // 0: idioma (inglês, 0x0409)
    "DIY",                        // 1: fabricante
    "Atari Jaguar Controller",    // 2: produto
    serial_str                    // 3: número de série
};

static uint16_t desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;

        if (index == 3 && serial_str[0] == '\0')
            pico_get_unique_board_id_string(serial_str, sizeof(serial_str));

        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31)
            chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++)
            desc_str[1 + i] = str[i];
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc_str;
}
