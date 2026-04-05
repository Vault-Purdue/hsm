# Requirements
HSM Requirements doc.

# PC Module Requirements
## UART Driver
## Client Tool
### Config
Baud rate:

Data bits:

Stop bits:

Parity:

Flow control:
### User Interface

## Host Crypto Module
Encrypt/Decrypt using <> algorithm

# HSM MSPM0L2228

Update headings per module structure

## Drivers
### UART Driver
PA10, PA11 pins, UART0 for back channel UART over XDS110.
### Timer Driver
### TRNG Driver
### Key Storage Driver
### AESADV Driver
Execute encryption and decryption using <> algorithm on command.
### Internal Flash Driver

## Modules
### UART CMD Router
Process UART message (CT CMD).

Provide PIN to auth engine.

Initiate PT/CT commands to Crypto Module.

Provide file fragments to File Manager.

### Auth Engine
Pass or reject PIN.

Utilize hold-off timer from Timer driver.
### State Machine
Determine state.

Provide state to File manager.
### Crypto Module
Utilize TRNG, Key storage, and AES module to encrypt and decrypt provided file.
### File Manager
Check current state and store file in internal flash mem.