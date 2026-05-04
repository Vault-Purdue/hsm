# HSM UART Communication Protocol - Version: 2.0

UART will be used for communication between the host and the HSM. The host CLI program will exchange messages with the HSM UART CMD Router, according to the protocol following.
None of this is finalized; this is all subject to change.

## UART Settings

UART communication will be held with the following standard settings:  
Baud rate: 115200 bps  
Parity: none  
Data size: 8  
Stop bits: 1

## UART Protocol Overview

One UART "frame" is composed of the following structure:

| SoF | Message ID | Payload Length | Payload | Checksum |
| --- | --- | --- | --- | --- |
| 1 Byte | 1 Byte | 1 Byte | 0 - 128 Bytes | 2 Bytes |

## Start of Frame Indicator (SoF)

The Start of Frame indicator is **0xAA**. All UART frames will begin with this byte to mark the beginning of the frame.
Because all frames also have a Payload Length field, an End of Frame indicator will not be necessary.

## Message ID

This field identifies the type of message the frame is, and thus how the payload should be interpreted or routed. Possible Message ID values are as follows:

| Message Type | Byte Value | Is Payload Encrypted? | Payload Content | Payload Length (bytes) | Relevant Host Command | Sender | Routing Criteria
| --- | --- | --- | --- | --- | --- | --- | --- |
| Session Open | 0x01 | No | `0x41` ('A' of Auth) | 1 | AUTH | Both | state == WAIT_FOR_UART |
| Key Exchange | 0x02 | No | Public Key | 32 | AUTH | Both | state == WAIT_FOR_UART |
| PIN Exchange | 0x03 | Yes? | ASCII PIN digits | 6 | AUTH | Host | state == WAIT_FOR_PIN |
| Session Close | 0x0F | No | None | 0 | CLOSE | Host (Both?) | any state |
| File Transfer Request | 0x20 | No | 1B direction (`0x77`: write, `0x72`: read) + 1B File ID | 2 | READ/WRITE | Host | state == UNLOCKED |
| File Contents | 0x21 | Yes | File contents itself | 88 | READ/WRITE | Both | state == UNLOCKED |
| File Request ACK | 0xF0 | No | `0x00`: approved, `0x01`: rejected | 1 | READ/WRITE | HSM | N/A |
| File Transfer Complete ACK | 0xF1 | No | `0x00`: checksum OK, `0x01`: mismatch | 1 | READ/WRITE | HSM | N/A |
| Pin Exchange ACK | 0xF2 | Yes | `0x00`: success, `0x01`: fail (lockout failure code?) | 1 | AUTH | HSM | N/A |

## Payload Length

Length of the Payload, in bytes. Possible values are 0-128.

## Payload

The data being sent itself. The only part of the frame which may be encrypted.

If the Payload is encrypted, its first 16 bytes will contain the auth tag for GCM.

## Checksum

The MSPM0 board has a built in cyclic redundancy checker, supporting both CRC-16 and CRC-32. We will use CRC-16 for checksum generation, resulting in a two byte checksum generated from the payload.
If there is no payload, the checksum field should be 0x00.
