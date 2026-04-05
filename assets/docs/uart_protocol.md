# HSM UART Communication Protocol

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
| 2 Bytes | 2 Bytes | 2 Bytes | 0 - 1024 Bytes | 2 Bytes |

## Start of Frame Indicator (SoF)

The Start of Frame indicator is **0xAA**. All UART frames will begin with these two bytes to mark the beginning of the frame.
Because all frames also have a Payload Length field, an End of Frame indicator will not be necessary.

## Message ID

This field identifies the type of message the frame is, and thus how the payload should be interpreted or routed. Possible Message ID values are as follows:

| Message Type | Byte Value | Is Payload Encrypted? | Payload Content | Relevant Host Command | Sender |
| --- | --- | --- | --- | --- | --- |
| Session Open | 0x00 | No | None? | AUTH | Host | 
| Key Exchange | 0x01 | No | Key exchange information? | AUTH | Both |
| PIN Exchange | 0x02 | Yes? | PIN, result of PIN attempt | AUTH | Both |
| Session Close | 0x0F | No | None | CLOSE | Host (Both?) |
| Status Query | 0x10 | No | Requested status/filesystem information | STATUS | Host |
| Status Response | 0x11 | No | Requested status/filesystem information | STATUS | HSM |
| File Transfer Request | 0x10 | No | Whether request is for an upload or download, name/path of requested file | READ/WRITE | Host |
| File Start | 0x11 | Yes | First block of file | READ/WRITE | Both |
| File Block | 0x12 | Yes | Any block of file | READ/WRITE | Both |
| File End | 0x13 | Yes | Final block of file | READ/WRITE | Both |
| File Transfer Complete | 0x14 | No | Checksum for whole file verification | READ/WRITE | Both |
| File Request ACK | 0xF0 | No | Whether file transfer can be performed or not (ie fail if requested file does not exist) | READ/WRITE | Both |
| File Block ACK | 0xF1 | No | None | READ/WRITE | Both |
| File Transfer Complete ACK | 0xF2 | No | Whether checksum verification succeeded or not | READ/WRITE | Both |

## Payload Length

Length of the Payload, in bytes. Possible values are 0-1024.

## Payload

The data being sent itself. The only part of the frame which may be encrypted.

## Checksum

The MSPM0 board has a built in cyclic redundancy checker, supporting both CRC-16 and CRC-32. We will use CRC-16 for checksum generation, resulting in a two byte checksum generated from the payload.
If there is no payload, the checksum field should be 0x00.
