# PicoMidi
RaspberrypiPicoをUSB-MIDIにする  

## Windowsでのビルド
`> md build  `
`> cd build  `
`> vcvars64  `
`> cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug  `
`または (Pythonのパスを指定する時)  `
`> cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DPython3_EXECUTABLE=D:\Apps\python3\python3.exe  `
`または (リリース時は)  `
`> cmake .. -G "NMake Makefiles"  `
`をしてから  `
`> nmake  `

## Linuxでのビルド
$ mkdir build  
$ cd build  
$ cmake ..  
$ make  

