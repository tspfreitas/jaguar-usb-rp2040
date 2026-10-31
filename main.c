/*
 * Atari Jaguar controller -> USB (RP2040-Zero)
 *
 * Porte do firmware raphnet/jaguar_usb (Copyright (C) 2009-2016 Raphaël Assénat,
 * GNU GPL v2 ou posterior) para o RP2040 com TinyUSB. A V-USB foi removida
 * (o RP2040 tem USB nativo); a lógica de leitura da matriz e o formato do
 * relatório HID foram mantidos. Este código continua sob GPL v2.
 *
 * Ligação (Jaguar DB15 -> RP2040-Zero):
 *
 *   Pino 1  (colunas) -> GP0     Pino 10 (linha 0) -> GP4
 *   Pino 2  (colunas) -> GP1     Pino 6  (linha 1) -> GP5
 *   Pino 3  (colunas) -> GP2     Pino 13 (linha 2) -> GP6
 *   Pino 4  (colunas) -> GP3     Pino 12 (linha 3) -> GP7
 *                                Pino 11 (linha 4) -> GP8
 *   Pino 7  (VCC)     -> 3V3     Pino 14 (linha 5) -> GP9
 *   Pino 9  (GND)     -> GND
 *
 * ATENÇÃO: alimente o controle com 3V3, nunca com 5 V (os pinos do RP2040
 * não toleram 5 V).
 */

#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"

/* Tempo de assentamento após ativar uma coluna. Aumente se houver leitura
 * instável (cabos longos); o projeto original usava 2000 us. */
#define SETTLE_US 100

/* Intervalo entre varreduras do controle */
#define POLL_US 2000

/* Colunas: pinos 1, 2, 3 e 4 do conector do Jaguar */
static const uint8_t COL_PINS[4] = { 0, 1, 2, 3 };

/* Linhas, na mesma ordem de bits do código original (coldat):
 * bit0 = pino 10, bit1 = pino 6, bit2 = pino 13,
 * bit3 = pino 12, bit4 = pino 11, bit5 = pino 14 */
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

/* Cada coluna é ativada puxando-a para nível baixo; as linhas têm pull-up,
 * então tecla pressionada = bit 0 em coldat. */
static void read_controller(jag_state_t *st)
{
    uint8_t coldat[4];

    for (int col = 0; col < 4; col++) {
        gpio_set_dir(COL_PINS[col], GPIO_OUT);   /* saída em nível baixo */
        sleep_us(SETTLE_US);

        uint8_t d = 0;
        for (int r = 0; r < 6; r++) {
            if (gpio_get(ROW_PINS[r]))
                d |= (uint8_t)(1u << r);
        }
        coldat[col] = d;

        gpio_set_dir(COL_PINS[col], GPIO_IN);    /* volta a entrada com pull-up */
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
    if (!(coldat[3] & 0x10)) st->x = 0xFF; /* Leste */

    st->btnsKp[5] = !(coldat[0] & 0x08);  /* 6 */
    st->btnsKp[4] = !(coldat[1] & 0x08);  /* 5 */
    st->btnsKp[3] = !(coldat[2] & 0x08);  /* 4 */
    if (!(coldat[3] & 0x08)) st->x = 0x00; /* Oeste */

    st->btnsKp[8] = !(coldat[0] & 0x04);  /* 9 */
    st->btnsKp[7] = !(coldat[1] & 0x04);  /* 8 */
    st->btnsKp[6] = !(coldat[2] & 0x04);  /* 7 */
    if (!(coldat[3] & 0x04)) st->y = 0xFF; /* Sul */

    st->btnsKp[11] = !(coldat[0] & 0x20); /* # */
    st->btnsKp[10] = !(coldat[1] & 0x20); /* 0 */
    st->btnsKp[9]  = !(coldat[2] & 0x20); /* * */
    if (!(coldat[3] & 0x20)) st->y = 0x00; /* Norte */
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

/* ----------------------- callbacks do TinyUSB ----------------------- */

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
    bool pending = true;   /* envia o estado inicial */
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
