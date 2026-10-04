# PicoBeepMidi
RaspberrypiPicoまたはRP2040ZeroをUSB-MIDI、UART接続してMIDIをBEEPで演奏します。  
250MHzで動作しています。250MHzは公式認定範囲外の設定です。  
UARTは31250bps、RXはGPIO1、TXはGPIO0、音声PWMはGPIO6です。  
GMモードとBEEPモードがあります。  
GMモードは音色がGM風になります。  
BEEPモードは音色がBEEP音になります。ドラムはPSG風になります。  

## 端子
GPIO0 : UART TX  
GPIO1 : UART RX  
GPIO6 : Audio  
UARTでの接続の場合は5Vの電源が必要です  

<img src="https://github.com/kuran-kuran/PicoBeepMidi/blob/0b89364938ee0909366703f0e02b3eb66494f047/PicoMidi_1.jpg" alt="AudioOut + USB接続" width="50%">  
AudioOut + USB接続  

<img src="https://github.com/kuran-kuran/PicoBeepMidi/blob/0b89364938ee0909366703f0e02b3eb66494f047/PicoMidi_2.jpg" alt="AudioOut + MIDI IN接続" width="50%">  
AudioOut + MIDI IN接続  

## 使い方
Windowsの場合はUSBで接続するとMIDIとして認識します。USB接続の場合はVSYSピンの接続は必要ありません。  
UARTで接続する場合は出力側のTXをGPIO 1(RX)に接続して電源5VをVSYSピンに入れてください。(RP2040Zeroは5Vピン)  
UARTで接続する場合は電圧変換が必要かもしれません。  
USBとUARTは同時に接続することはできません。  
どちらか1系統の接続だけしてください。  
音声出力はGPIO 6になっています。モノラルです。
ローパスフィルタとか必要かもしれませんが私はイヤホン端子をGNDとGPIO 6に直接繋げています。これで今のところ音は鳴っています。  
GMモードではPaspberryPiPicoの場合はLEDが明るく点灯します。RP2040Zeroの場合は緑色に点灯します。  
BEEPモードではPaspberryPiPicoの場合はLEDが暗く点灯します。RP2040Zeroの場合は赤色に点灯します。  
BOOTボタンを押すとモードの切り替えができます。モードの切り替えは演奏中も行う事ができます。  
GMモードで音色にクラビネットを指定すると音色がCMU-800になります。  
その場合MIDIチャンネル2だけ特別にCMU-800のBASSの音色になります。  
BEEPモードすべてのチャンネルの音色がBEEP音になりドラムはPSG風になります。  

## ビルド方法
RaspberryPiPicoとRP2040Zeroに対応しています。  
`>` md build  
`>` cd build  
`>` vcvars64 (Windowsのみ)  
を実行後  
| 環境・基板 | コマンド |
| --- | --- |
| Linux・Pico | `cmake .. -DPICO_BOARD=pico` |
| Linux・Zero | `cmake .. -DPICO_BOARD=waveshare_rp2040_zero` |
| Windows・Pico | `cmake .. -G "NMake Makefiles" -DPICO_BOARD=pico` |
| Windows・Zero | `cmake .. -G "NMake Makefiles" -DPICO_BOARD=waveshare_rp2040_zero` |

その後Linuxではmake、Windowsではnmakeするとビルドされます  

## 注意
音声出力はモノラル出力です。  
PANには対応していません。  

