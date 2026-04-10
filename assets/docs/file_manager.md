# HSM File Manager (FM) Design - Version: 1.0

The HSM will store 8 blocks distributed across 1 sector of Flash memory (1 KB). The purpose of this document is to outline the architecture of the File Manager module, its dependencies, and the current Flash memory layout.

## Flash Memory Layout

### Blocks Layout
* **Capacity:** 8 slots (128 Bytes each) = 1 Flash sector (1 KB)
* **Atomic Operations:** The MCU handles 128B as the native atomic write operation
* **Slot Structure:** 40 Bytes (Metadata & Crypto) + 88 Bytes (Payload)
* **Sector 1:** 40 Bytes (Metadata & Crypto) + 88 Bytes (Payload)

**Note on Chunk Index:** This field was removed as it is redundant for our sequential model. Its 2 bytes were merged into the `Reserved` field.

![FileManager](../images/file_manager_layout.png)

### Key Layout
This 1KB sector is reserved exclusively for the Auth key persistence

* **Slot Structure**: 8 available slots (128B each).
* **Implementation**: The Security Block is statically assigned to Slot 0.

Access Control: Restricted to the Auth Engine. Any Router UART request targeting this sector is rejected by the File Manager.

![KeyLayout](../images/fm_key_layout.png)
---

## Dependencie 1: UART Protocol

The UART frame structure:

Previous:

| SoF | Message ID | Payload Length | Payload | Checksum |
| --- | --- | --- | --- | --- |
| 2 Bytes | 2 Bytes | 2 Bytes | 0 - 88 Bytes | 2 Bytes |

New:

| SoF | Message ID | Block ID | Payload Length | Payload | Checksum |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 2 Bytes | 2 Bytes | 2 Bytes | 2 Bytes | 0 - 88 Bytes | 2 Bytes |

The File Manager requires these shared fields from the router:

- `Block ID` (prev. File ID): The Host will request blocks sequentially (e.g., Blocks 1 to 8 to reconstruct the QR code)
- `Payload Length`: Max 88 Bytes
- `Payload`: QR code image or JSON credentials

---

## Dependency 2: Crypto Module (AES-GCM)

### 1. Block Encryption (Write Operation)
- **Inputs (Block Manager -> Crypto):**
  - `Plaintext`: Data payload (max 88 Bytes)
  - `AAD`: Block ID + Payload Length
- **Outputs (Crypto -> Block Manager):**
  - `IV / Nonce`: 12 Bytes (newly generated via TRNG per write)
  - `Ciphertext`: 88 Bytes
  - `Auth Tag`: 16 Bytes

### 2. Block Decryption (Read Operation)
* **Inputs (Block Manager -> Crypto):**
  * `Ciphertext`: 88 Bytes.
  * `IV / Nonce`: 12 Bytes.
  * `Auth Tag`: 16 Bytes.
  * `AAD`: Block ID + Payload Length.
* **Outputs (Crypto -> Block Manager):**
  * `Status`: Success (`STATUS_OK`) or Authentication Failure (`ERROR_AUTH_FAILED`).
  * `Plaintext`: 88 Bytes (Provided only if Status is OK).

---

## Dependency 3: Secure Boot & CSC (Customer Secure Configuration)

The CSC acts as the root of trust during the MCU boot sequence. It executes in high-privilege mode before the main application starts. 

The CSC is  responsible for managing the hardware firewalls and the persistence of the Master Key. The CSC bypasses the F Manager and interacts directly with the `NONMAIN` Flash sector.

### CSC Boot Sequence Flow:

1. CSC starts privileged with access to all Flash sectors
2. checks dedicated MAIN Flash sector for existing Master Key
   - first boot → TRNG generates 32-byte key, writes to Flash, loads into Keystore
   - subsequent boots → reads existing key from Flash, loads into Keystore
3. `SYSCTL.SECCFG.FWEPROTMAIN` locks write/erase access to the key sector
4. INITDONE → Keystore writes disabled, hands off to main app

The threat: `FWEPROTMAIN` only blocks writes/erases, not reads. If debug access isn't locked in NONMAIN (a dedicated Flash section that handles policies like blocking JTAG), the key sector is readable via debugger. fix is locking debug access unconditionally in NONMAIN.
