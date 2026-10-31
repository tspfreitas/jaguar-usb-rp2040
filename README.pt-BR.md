<div align="center">

# Controle Atari Jaguar → USB (RP2040)

**Um adaptador USB para o controle do Atari Jaguar, feito com a RP2040-Zero.**
Direcional completo, A/B/C, Option, Pause e o teclado numérico de 12 teclas, reconhecido como um gamepad USB padrão.

[![Licença: GPL v2+](https://img.shields.io/badge/Licen%C3%A7a-GPL%20v2%2B-blue.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/YOUR_USER/jaguar-usb-rp2040?include_prereleases)](../../releases)
[![Build](https://github.com/YOUR_USER/jaguar-usb-rp2040/actions/workflows/build.yml/badge.svg)](../../actions)

[English](README.md) · **Português**

<!-- Substitua por uma foto do adaptador pronto -->
<img src="docs/images/hero.jpg" alt="Adaptador Atari Jaguar para USB pronto" width="640">

</div>

---

## Índice

- [Recursos](#recursos)
- [O que você precisa](#o-que-você-precisa)
- [Ligações](#ligações)
- [Instalação (gravando o firmware)](#instalação-gravando-o-firmware)
- [Usando no Windows](#usando-no-windows)
- [Mapeamento dos botões](#mapeamento-dos-botões)
- [Compilando o código-fonte](#compilando-o-código-fonte)
- [Créditos e licença](#créditos-e-licença)
- [Apoie o projeto](#apoie-o-projeto)
- [Aviso legal](#aviso-legal)

## Recursos

- **Gamepad USB HID** padrão: não precisa de drivers.
- **Todas as 21 entradas** do controle do Jaguar: direcional (2 eixos), A, B, C, Option, Pause e as 12 teclas do teclado (17 botões).
- USB full-speed nativo (TinyUSB) no RP2040, com polling USB de 1 ms.
- Firmware gravado por arrastar e soltar (UF2), sem precisar de gravador.
- Firmware pronto em [Releases](../../releases), ou compile você mesmo com o GitHub Actions em poucos minutos.

## O que você precisa

- 1× **RP2040-Zero** (ou outra placa RP2040 compatível com GPIOs livres suficientes)
- 1× controle de Atari Jaguar e uma forma de acessar o **conector DB15** (um plugue DB15 ou um cabo de extensão que possa ser sacrificado)
- Fios e, se necessário, ferro de solda

## Ligações

> ⚠️ **Alimente o controle com 3V3, nunca com 5 V.** Os pinos do RP2040 **não** toleram 5 V.

| Pino do DB15 do Jaguar | Função | RP2040-Zero |
|:---:|---|:---:|
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
| 7 | VCC | **3V3** |
| 9 | GND | GND |

A pinagem fica nas tabelas `COL_PINS` e `ROW_PINS` no início do `main.c`, então é fácil de alterar.

<!-- Adicione suas fotos de instalação abaixo. Mantenha os nomes dos arquivos ou edite os caminhos. -->
<p align="center">
  <img src="docs/images/wiring.jpg" alt="Ligação entre o conector do Jaguar e a RP2040-Zero" width="48%">
  <img src="docs/images/install-1.jpg" alt="Adaptador montado" width="48%">
</p>
<p align="center">
  <img src="docs/images/install-2.jpg" alt="Adaptador dentro da caixa" width="48%">
  <img src="docs/images/install-3.jpg" alt="Adaptador conectado a um PC" width="48%">
</p>

## Instalação (gravando o firmware)

1. Baixe o **`jaguar_usb_rp2040.uf2`** na [última release](../../releases/latest) (Rev 1.0).
2. Segure o botão **BOOT** da RP2040-Zero, conecte-a ao PC pelo USB e depois solte o botão.
3. Aparece um disco chamado **RPI-RP2**. Copie o arquivo `.uf2` para ele.
4. A placa reinicia sozinha e passa a aparecer como um gamepad. Pronto.

Para atualizar o firmware no futuro, repita os mesmos passos com o novo `.uf2`.

## Usando no Windows

1. Pressione <kbd>Win</kbd> + <kbd>R</kbd>, digite `joy.cpl` e tecle Enter.
2. Selecione **Atari Jaguar Controller** e clique em **Propriedades**.
3. Aperte os botões do controle e confira se os botões correspondentes acendem e se os eixos se movem.

<!-- Substitua por um print da janela de propriedades do controle de jogo do Windows -->
<p align="center">
  <img src="docs/images/windows-joy-cpl.png" alt="Configurações de controle de jogo do Windows mostrando o controle do Jaguar" width="70%">
</p>

## Mapeamento dos botões

| Botão HID / eixo | Entrada do Jaguar |
|:---:|---|
| Eixo X | Direcional esquerda / direita |
| Eixo Y | Direcional cima / baixo |
| Botão 1 | A |
| Botão 2 | B |
| Botão 3 | C |
| Botão 4 | Option |
| Botão 5 | Pause |
| Botões 6 a 14 | Teclado 1 a 9 |
| Botão 15 | Teclado \* |
| Botão 16 | Teclado 0 |
| Botão 17 | Teclado # |


**Ajustes:** se alguma tecla ler de forma instável (por exemplo, com cabo longo), aumente o `SETTLE_US` no `main.c`. O VID/PID USB em `usb_descriptors.c` são valores de teste do TinyUSB; troque-os se for distribuir o adaptador.

## Créditos e licença

Este projeto é um **port** do excelente firmware
[**jaguar_usb**](https://github.com/raphnet/jaguar_usb) de
**Raphaël Assénat ([raphnet](https://www.raphnet.net))**, originalmente escrito para
microcontroladores AVR (ATmega8 / ATmega168) com a pilha V-USB.
A lógica de leitura da matriz do controle e o formato do relatório HID vêm do trabalho dele.
Todo o crédito é do autor original.

**O que mudou neste port**

- A pilha USB por software (V-USB) e todo o código específico de AVR foram removidos.
- O USB agora é tratado pelo [TinyUSB](https://github.com/hathach/tinyusb) no controlador USB nativo do RP2040.
- Voltado para a RP2040-Zero, compilado com o [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

**Licença**

Este projeto é distribuído sob a **GNU General Public License, versão 2 ou (a seu critério) qualquer versão posterior** (GPL-2.0-or-later), os mesmos termos do original. Veja o arquivo [LICENSE](LICENSE). Se você redistribuir ou modificar, deve manter os avisos de copyright originais e licenciar suas alterações nos mesmos termos.

| Componente | Licença |
|---|---|
| Firmware original jaguar_usb (Raphaël Assénat) | GPL-2.0-or-later |
| Este port para RP2040 | GPL-2.0-or-later |
| Raspberry Pi Pico SDK (dependência de compilação) | BSD-3-Clause |
| TinyUSB (dependência de compilação) | MIT |

Se você gosta desse tipo de trabalho, considere também apoiar o autor original em [raphnet.net](https://www.raphnet.net).

## Apoie o projeto

Este adaptador é gratuito e de código aberto. Se ele foi útil e você quiser ajudar a financiar futuros projetos de hardware retrô, doações são muito bem-vindas e totalmente opcionais. Elas não dão direito a suporte nem a funcionalidades especiais.

<!-- Edite os links abaixo com as suas páginas e apague os que não for usar. -->
- ☕ **Ko-fi:** ko-fi.com/thomasfreitas
- 🇧🇷 **Pix:** `dev.freitas@gmail.com`

Dar uma estrela no repositório e compartilhar fotos da sua montagem também ajuda muito.

## Aviso legal

Este é um projeto de hobby fornecido **no estado em que se encontra, sem garantia de qualquer tipo**. Você é responsável pelas suas ligações e por qualquer dano ao seu equipamento. *Atari* e *Jaguar* são marcas registradas de seus respectivos proprietários. Este projeto não é afiliado nem endossado pela Atari ou pela raphnet.
