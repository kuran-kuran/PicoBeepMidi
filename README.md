# PicoBeepMidi
RaspberrypiPicoをUSB-MIDIまたはUART接続してMIDIをBEEPで演奏する  

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

