# PicoBeepMidi
RaspberrypiPicoをUSB-MIDIまたはUART接続してMIDIをBEEPで演奏する  

## 端子
GPIO 0 : UART TX  
GPIO 1 : UART RX  
GPIO 6 : Audio  

## 使い方
Windowsの場合はUSBで接続するとMIDIとして認識します。USB接続の場合はVSYSピンの接続は必要ありません。  
UARTで接続する場合は出力側のTXをGPIO 1(RX)に接続して電源5VをVSYSピンに入れてください。(RP2040Zeroは5Vピン)  
UARTで接続する場合は電圧変換が必要かもしれません。  
USBとUARTは同時に接続することはできません。  
どちらか1系統の接続だけしてください。  
音声出力はGPIO 6になっています。モノラルです。
ローパスフィルタとか必要かもしれませんが私はイヤホン端子をGNDとGPIO 6に直接繋げています。これで今のところ音は鳴っています。  

## Windowsでのビルド
`>` md build  
`>` cd build  
`>` vcvars64  
`>` cmake .. -G "NMake Makefiles"  
または (Pythonのパスを指定する時)  
`>` cmake .. -G "NMake Makefiles" -DPython3_EXECUTABLE=D:\Apps\python3\python3.exe  
をしてから  
`>` nmake  

## Linuxでのビルド
$ mkdir build  
$ cd build  
$ cmake ..  
$ make  

