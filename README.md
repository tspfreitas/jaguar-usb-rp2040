# Atari Jaguar -> USB (RP2040-Zero)

Porte do firmware [raphnet/jaguar_usb](https://github.com/raphnet/jaguar_usb)
(ATmega8/168 + V-USB) para o RP2040 com TinyUSB. Gamepad HID com 2 eixos
(direcional) e 17 botões (A, B, C, Option, Pause e as 12 teclas do teclado
numérico). Licença: GPL v2 (herdada do projeto original).

## Ligação

| Jaguar (DB15) | Função | RP2040-Zero |
|---|---|---|
| 1 | Coluna 0 | GP0 |
| 2 | Coluna 1 | GP1 |
| 3 | Coluna 2 | GP2 |
| 4 | Coluna 3 | GP3 |
| 10 | Linha 0 | GP4 |
| 6 | Linha 1 | GP5 |
| 13 | Linha 2 | GP6 |
| 12 | Linha 3 | GP7 |
| 11 | Linha 4 | GP8 |
| 14 | Linha 5 | GP9 |
| 7 | VCC | **3V3** (nunca 5 V) |
| 9 | GND | GND |

Os pinos ficam na tabela `COL_PINS` / `ROW_PINS` em `main.c`.

## Gerar o .uf2

### Opção A: GitHub Actions (sem instalar nada)

1. Crie um repositório e envie estes arquivos (inclusive `.github/`).
2. Abra a aba Actions, execute "build-uf2" (ou apenas faça um push).
3. Baixe o artefato `jaguar_usb_rp2040-uf2`; dentro está o `jaguar_usb_rp2040.uf2`.

### Opção B: local (Linux/WSL)

```
sudo apt install cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
export PICO_SDK_PATH=~/pico-sdk
cmake -B build -S .
cmake --build build -j
```

O arquivo sai em `build/jaguar_usb_rp2040.uf2`.

## Gravar

Segure o botão BOOT da RP2040-Zero, conecte o USB e solte. Ela aparece como
um disco (RPI-RP2). Copie o `.uf2` para ele; a placa reinicia sozinha.

## Ajustes

- `SETTLE_US` (main.c): tempo de assentamento por coluna. Se alguma tecla ler
  de forma instável, aumente (o original usava 2000 us).
- `USB_VID` / `USB_PID` (usb_descriptors.c): valores de teste do TinyUSB.
  Troque se for distribuir; não reutilize o VID/PID da raphnet.
