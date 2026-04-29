# Vault HSM — CSC + LCD branch

How to flash and run this on the dev board.

## Setup

1. Download this branch as a ZIP and extract.
2. Install UniFlash from <https://www.ti.com/tool/UNIFLASH> if you don't have it.
3. Plug the LP-MSPM0L2228 dev board into your computer.
4. Open UniFlash. It should auto-detect the board:

![Detected Devices](readme_assets/04_detected_devices.png)

Click **Start**.

## Factory Reset (first time only)

Go to **Settings & Utilities**:

![Settings & Utilities](readme_assets/02_settings_utilities.png)

Set Erase method to **Erase MAIN and NONMAIN sectors only**:

![Erase Configuration](readme_assets/01_erase_config.png)

Press **Factory reset auto**:

![Factory Reset](readme_assets/03_factory_reset.png)

Close UniFlash, unplug the board, then reconnect it and reopen UniFlash. Go back to Settings & Utilities and switch the erase method to **Erase MAIN and NONMAIN necessary sectors only**.

## Build

In CCS, build both projects:
- `customer_secure_code_LP_MSPM0L2228_nortos_ticlang/`
- `hsm/`

## Flash

Go to the **Program** tab:

![Program](readme_assets/05_program_tab.png)

Click **Browse** under Flash Image(s):

![Browse](readme_assets/06_browse_button.png)

Navigate to `hsm-main\customer_secure_code_LP_MSPM0L2228_nortos_ticlang\Debug\` and double-click the `.out` file:

![CSC out](readme_assets/07_csc_out_file.png)

It'll appear in the list:

![CSC loaded](readme_assets/08_csc_loaded.png)

Press the **(+)** to add another image. Navigate to `hsm-main\hsm\Debug\` and double-click `hsm.out`:

![HSM out](readme_assets/09_hsm_out_file.png)

Click **Load Images**.

## Run

Press the **MSP_NRST** button on the dev board. After a moment, **LED3** should turn on for ~1 second and off for ~1 second in a steady heartbeat.

## LCD (optional)

To enable the LCD display, in CCS right-click the `hsm` project → Properties → Build → Tools → Arm Compiler → Predefined Symbols → add `LCD_ENABLE=1`. Rebuild and re-flash. The LCD will show test progression on boot.
