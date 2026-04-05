# HSM Integration Plan
System and module integration plan.

## System

## Module

### AESADV
1. Add AESADV drivers and config to uart_echo. Successfully compile/run.
2. Load/hard-code a test keys and echo both input and encrypted hard-coded string.
3. Echo input, encrypted, decrypted hard-coded string. Decrypted matches input.
3. Check encryption results repeatable using matching python algorithm implementation on PC, with test keys.
4. Send encrypted hard-coded data to PC over UART, decrypt with python implementation using test key.
5. Send PC-sourced string over UART, encrypt, return.
6. Unit test variety of strings to ensure functional guard rails adequate for encryption engine.
