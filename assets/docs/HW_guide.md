# UART Echo Project Setup - Version: 1.0
(Jumper pins J17 and J18 on top two pins)
Connect board to computer with USB cable
In your OS’ Device Manager, look for COM port for “XDS110 Class Application/User UART”
Open a Serial Console terminal at View > Console > Serial Console. 
Top right of Serial Console window has a few option buttons, far left of them is “Connect/Disconnect COM Port”
Port: COM port from before
Baud Rate: 9600
Parity: None
Data Size: 8
Stop Bits: 1
Start Debugging UART Echo project. Execution will halt at main(), press continue
All characters typed into Serial Console should be printed back to you, showing UART communication is functioning correctly.
FYI the buffer is 1 character, so it will loop on 1 character, not strings.