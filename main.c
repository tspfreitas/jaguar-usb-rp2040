/*
 * Atari Jaguar controller to USB adapter (RP2040 port)
 *
 * Copyright (C) 2009-2016 Raphaël Assénat   (original AVR firmware, raphnet)
 * Copyright (C) 2026 <YOUR NAME>             (RP2040 / TinyUSB port)
 *
 * This is a port of raphnet/jaguar_usb (https://github.com/raphnet/jaguar_usb).
 * The V-USB software USB stack and the AVR-specific code were replaced by
 * TinyUSB running on the RP2040's native USB controller. The controller
 * matrix scanning logic and the HID report layout come from the original.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Wiring (Jaguar DB15 -> RP2040-Zero):
 *
 *   Pin 1  (column 0) -> GP0      Pin 10 (row 0) -> GP4
 *   Pin 2  (column 1) -> GP1      Pin 6  (row 1) -> GP5
 *   Pin 3  (column 2) -> GP2      Pin 13 (row 2) -> GP6
 *   Pin 4  (column 3) -> GP3      Pin 12 (row 3) -> GP7
 *                                 Pin 11 (row 4) -> GP8
 *   Pin 7  (VCC)      -> 3V3      Pin 14 (row 5) -> GP9
 *   Pin 9  (GND)      -> GND
 *
 * WARNING: power the controller from 3V3, never from 5 V (RP2040 GPIOs
 * are not 5 V tolerant).
 */
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"

/* Settling time after driving a column low. Increase if readings are
 * unstable (long cables); the original AVR firmware used 2000 us. */
#define SETTLE_US 100

/* Interval between controller scans */
#define POLL_US 2000

/* Columns: pins 1, 2, 3 and 4 of the Jaguar connector */
static const uint8_t COL_PINS[4] = { 0, 1, 2, 3 };

/* Rows, in the same bit order as the original code (coldat):
 * bit0 = pin 10, bit1 = pin 6, bit2 = pin 13,
 * bit3 = pin 12, bit4 = pin 11, bit5 = pin 14 */
static const uint8_t ROW_PINS[6] = { 4, 5, 6, 7, 8, 9 };

typedef struct __attribute__((packed)) {
    uint8_t x;
    uint8_t y;
    uint8_t btns1;
    uint8_t btns2;
    uint8_t btns3;
} jag_report_t;

typedef struct {
    uint8_t x, y;
    uint8_t btnA, btnB, btnC;
    uint8_t btnOPT, btnPAUSE;
    uint8_t btnsKp[12];  /* 1-9, *, 0, # */
} jag_state_t;

static jag_report_t cur_report;
static jag_report_t sent_report;

static void jaguar_init(void)
{
    for (int i = 0; i < 4; i++) {
        gpio_init(COL_PINS[i]);
        gpio_put(COL_PINS[i], 0);
        gpio_set_dir(COL_PINS[i], GPIO_IN);
        gpio_pull_up(COL_PINS[i]);
    }
    for (int i = 0; i < 6; i++) {
        gpio_init(ROW_PINS[i]);
        gpio_set_dir(ROW_PINS[i], GPIO_IN);
        gpio_pull_up(ROW_PINS[i]);
    }
}

/* Each column is activated by pulling it low; rows have pull-ups,
 * so a pressed key reads as bit 0 in coldat. */
static void read_controller(jag_state_t *st)
{
    uint8_t coldat[4];

    for (int col = 0; col < 4; col++) {
        gpio_set_dir(COL_PINS[col], GPIO_OUT);   /* output, driven low */
        sleep_us(SETTLE_US);

        uint8_t d = 0;
        for (int r = 0; r < 6; r++) {
            if (gpio_get(ROW_PINS[r]))
                d |= (uint8_t)(1u << r);
        }
        coldat[col] = d;

        gpio_set_dir(COL_PINS[col], GPIO_IN);    /* back to input with pull-up */
    }

    memset(st, 0, sizeof(*st));
    st->x = 0x80;
    st->y = 0x80;

    st->btnPAUSE = !(coldat[3] & 0x02);
    st->btnOPT   = !(coldat[0] & 0x01);
    st->btnC     = !(coldat[1] & 0x01);
    st->btnB     = !(coldat[2] & 0x01);
    st->btnA     = !(coldat[3] & 0x01);

    st->btnsKp[2] = !(coldat[0] & 0x10);  /* 3 */
    st->btnsKp[1] = !(coldat[1] & 0x10);  /* 2 */
    st->btnsKp[0] = !(coldat[2] & 0x10);  /* 1 */
    if (!(coldat[3] & 0x10)) st->x = 0xFF; /* East */

    st->btnsKp[5] = !(coldat[0] & 0x08);  /* 6 */
    st->btnsKp[4] = !(coldat[1] & 0x08);  /* 5 */
    st->btnsKp[3] = !(coldat[2] & 0x08);  /* 4 */
    if (!(coldat[3] & 0x08)) st->x = 0x00; /* West */

    st->btnsKp[8] = !(coldat[0] & 0x04);  /* 9 */
    st->btnsKp[7] = !(coldat[1] & 0x04);  /* 8 */
    st->btnsKp[6] = !(coldat[2] & 0x04);  /* 7 */
    if (!(coldat[3] & 0x04)) st->y = 0xFF; /* South */

    st->btnsKp[11] = !(coldat[0] & 0x20); /* # */
    st->btnsKp[10] = !(coldat[1] & 0x20); /* 0 */
    st->btnsKp[9]  = !(coldat[2] & 0x20); /* * */
    if (!(coldat[3] & 0x20)) st->y = 0x00; /* North */
}

static void jaguar_update(void)
{
    jag_state_t st;
    uint8_t b1 = 0, b2 = 0, b3 = 0, bit;
    int i;

    read_controller(&st);

    if (st.btnA)     b1 |= 0x01;
    if (st.btnB)     b1 |= 0x02;
    if (st.btnC)     b1 |= 0x04;
    if (st.btnOPT)   b1 |= 0x08;
    if (st.btnPAUSE) b1 |= 0x10;

    for (i = 0, bit = 0x20; bit; i++, bit <<= 1)
        if (st.btnsKp[i]) b1 |= bit;

    for (bit = 0x01; bit; i++, bit <<= 1)
        if (st.btnsKp[i]) b2 |= bit;

    if (st.btnsKp[11])
        b3 |= 0x01;

    cur_report.x = st.x;
    cur_report.y = st.y;
    cur_report.btns1 = b1;
    cur_report.btns2 = b2;
    cur_report.btns3 = b3;
}

/* ------------------------- TinyUSB callbacks ------------------------ */

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type;
    uint16_t len = sizeof(cur_report);
    if (reqlen < len) len = reqlen;
    memcpy(buffer, &cur_report, len);
    return len;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}

/* ------------------------------ main ------------------------------- */

int main(void)
{
    jaguar_init();
    tusb_init();

    jaguar_update();
    bool pending = true;   /* send the initial state */
    uint32_t last_poll = time_us_32();

    while (true) {
        tud_task();

        if ((uint32_t)(time_us_32() - last_poll) >= POLL_US) {
            last_poll = time_us_32();
            jaguar_update();
            if (memcmp(&cur_report, &sent_report, sizeof(cur_report)) != 0)
                pending = true;
        }

        if (pending && tud_hid_ready()) {
            if (tud_hid_report(0, &cur_report, sizeof(cur_report))) {
                sent_report = cur_report;
                pending = false;
            }
        }
    }
    return 0;
}
