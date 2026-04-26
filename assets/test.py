import serial

ser = serial.Serial('COM4', 115200, timeout=10)  # Adjust COM port and baud rate as needed
print("Serial port opened:", ser.name)

while True:
    userinput = input("Enter command (open,keyexchange,pinexchange,filetransferrequest,sessionclose,quit)\n>> ").strip().lower()
    if userinput == "open":
        # OPEN SESSION 
        # start byte (0xAA), message type (0x01 for session open, payload length (1 byte), payload (variable length), CRC (2 bytes)
        msg_session_open = bytes([0xAA,0x01,0x01,0x41,0x00,0x00]) # full message, dummy crc
        ser.write(msg_session_open)  # Send the session open message to the MSP
        print("Sent session open:", ' '.join(f'{b:02X}' for b in msg_session_open))

    elif userinput == "keyexchange":
        # EXCHANGE KEY
        # start byte (0xAA), message type 0x02 for key exchange, payload length (0x20=32 bytes), payload (32 byte key), CRC (2 bytes)
        msg_key_exchangeheader = bytes([0xAA,0x02,0x20])
        msg_key_exchangepayload = bytes.fromhex('00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF') # 32 bytes of dummy key data
        msg_key_crc = bytes([0x00,0x00]) # dummy crc
        msg_key_exchange = msg_key_exchangeheader + msg_key_exchangepayload + msg_key_crc
        ser.write(msg_key_exchange)  # Send the key exchange message to the MSP
        print("Sent key exchange:", ' '.join(f'{b:02X}' for b in msg_key_exchange))

    elif userinput == "pinexchange":
        # EXCHANGE PIN
        # start byte (0xAA), message type 0x03 for pin exchange, payload length (0x06=6 bytes), payload (6 bytes), CRC (2 bytes)
        msg_pin_exchangeheader = bytes([0xAA,0x03,0x06])
        msg_pin_exchangepayload = "123456" # the default pin
        msg_pin_crc = bytes([0x00,0x00]) # dummy crc
        msg_pin_exchange = msg_pin_exchangeheader + msg_pin_exchangepayload.encode() + msg_pin_crc
        ser.write(msg_pin_exchange)  # Send the pin exchange message to the MSP
        print("Sent pin exchange:", ' '.join(f'{b:02X}' for b in msg_pin_exchange))
    
    elif userinput == "sessionclose":
        # CLOSE SESSION 
        # start byte (0xAA), message type (0x0F for session close, payload length (1 byte = 0x00), payload (0 bytes), CRC (2 bytes)
        msg_session_close = bytes([0xAA,0x0F,0x00,0x00,0x00]) # full message, dummy crc
        ser.write(msg_session_close)  # Send the session close message to the MSP
        print("Sent session close:", ' '.join(f'{b:02X}' for b in msg_session_close))
    
    elif userinput == "filetransferrequest":
        # FILE TRANSFER REQUEST
        direction = input("Read or write? (r/w)\n>> ").strip().lower()
        if direction == 'r':
            direction_byte = bytes([0x77]) # 77 for read
        elif direction == 'w':
            direction_byte = bytes([0x72]) # 72 for write
        else:
            print("Starting over...")
            break
        file_id = input("Enter file ID (2B hex)\n>> ").strip()
        # start byte (0xAA), message type 0x20 for file transfer request, payload length (3 bytes), payload (1B direction, 2B file ID), CRC (2 bytes)
        msg_file_transfer_requestheader = bytes([0xAA,0x20,0x03])
        msg_file_transfer_requestcrc = bytes([0x00,0x00]) # dummy crc
        msg_file_transfer_request = msg_file_transfer_requestheader + direction_byte + bytes.fromhex(file_id) + msg_file_transfer_requestcrc
        ser.write(msg_file_transfer_request)  # Send the file transfer request message to the MSP
        print("Sent file transfer request:", ' '.join(f'{b:02X}' for b in msg_file_transfer_request))
    
    elif userinput == "file":
        # FILE TRANSFER
        # start byte (0xAA), message type 0x21 for file transfer, payload length (variable, up to 88B), payload (file data), CRC (2 bytes)
        file_bytes = bytes.fromhex('00112233445566778899AABBCCDDEEFF') # 16B dummy file data, adjust length as needed (up to 88 bytes)
        msg_file_transferheader = bytes([0xAA,0x21, len(file_bytes)])
        msg_file_transfercrc = bytes([0x00,0x00]) # dummy crc
        msg_file_transfer = msg_file_transferheader + file_bytes + msg_file_transfercrc
        ser.write(msg_file_transfer)  # Send the file transfer message to the MSP
        print("Sent file transfer:", ' '.join(f'{b:02X}' for b in msg_file_transfer))

    elif userinput == "quit":
        print("Exiting...")
        break
    message = ser.readline()
    print("MSP response:", message)
    message = ser.readline()
    print("MSP response:", message)
ser.close()