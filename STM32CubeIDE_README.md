# Opening CANable Firmware in STM32CubeIDE

This project can now be opened and built in STM32CubeIDE while maintaining full Makefile compatibility.

## Quick Start

### Option 1: Import Existing Project

1. **Open STM32CubeIDE**
2. Go to **File → Import...**
3. Select **General → Existing Projects into Workspace**
4. Browse to the `canable-fw` directory
5. Select the project and click **Finish**

### Option 2: Clone and Import

```bash
git clone https://github.com/sengulhamza/canable-fw.git
cd canable-fw
git checkout copilot/port-cunable-firmware
```

Then import as described in Option 1.

## Building the Firmware

### In STM32CubeIDE

1. **Right-click** on the project in Project Explorer
2. Select **Build Project** or press **Ctrl+B**
3. The compiled binaries will be in the `Debug/` folder:
   - `canable-fw.elf` - ELF file with debug symbols
   - `canable-fw.bin` - Binary file for flashing
   - `canable-fw.hex` - Intel HEX format

### Using Makefile (Terminal)

The traditional Makefile build still works:

```bash
make clean
make -j
```

Binaries will be in the `build/` folder.

## Project Configuration

### Hardware Configuration (STM32H723VET6)

The `.ioc` file contains the complete pin and peripheral configuration:

**Pins:**
- **USB**: PA11 (DM), PA12 (DP) - USB_OTG_HS in FS mode
- **FDCAN1**: PD0 (RX), PD1 (TX) with AF9
- **CAN Control**: PC9 (NSTB, active low), PC6 (DTR_EN)
- **LEDs**: PE0 (Red), PE1 (Green), PE2 (Blue)
- **Oscillator**: PH0/PH1 - 8MHz HSE

**Clocks:**
- System: 400MHz
- FDCAN: 50MHz
- USB: HSI48 (48MHz)

### Modifying Configuration with CubeMX

1. **Double-click** `canable-fw.ioc` in Project Explorer
2. STM32CubeMX will open embedded in the IDE
3. Make your changes (pins, clocks, peripherals)
4. Click **Project → Generate Code** (or Ctrl+S)
5. CubeMX will update the configuration files

**Important**: The project uses custom source files. CubeMX will:
- ✅ Update `stm32h7xx_hal_conf.h`
- ✅ Update clock configuration in `system_stm32h7xx.c`
- ✅ Update interrupt handlers in `interrupts.c`
- ❌ Will NOT overwrite your custom application code (marked with USER CODE sections)

### Project Structure

```
canable-fw/
├── .cproject              # Eclipse CDT configuration (IDE build)
├── .project               # Eclipse project file
├── .ioc                   # STM32CubeMX configuration
├── Makefile               # Traditional Makefile (for CLI builds)
├── STM32H723VET6_FLASH.ld # Linker script
├── src/                   # Source files
│   ├── main.c
│   ├── can.c              # FDCAN implementation
│   ├── slcan.c            # SLCAN protocol
│   ├── usb_device.c
│   └── ...
├── inc/                   # Header files
├── Drivers/               # STM32 HAL drivers
│   ├── STM32H7xx_HAL_Driver/
│   └── CMSIS/
└── Middlewares/           # USB middleware
```

## Debugging

### Debug Configuration

1. **Right-click** on project → **Debug As → STM32 C/C++ Application**
2. STM32CubeIDE will auto-detect your ST-LINK debugger
3. Select the `.elf` file from the Debug folder
4. Click **Debug**

### Debug Features

- ✅ Breakpoints
- ✅ Variable inspection
- ✅ Memory browser
- ✅ Register view
- ✅ SWV (Serial Wire Viewer) for printf debugging
- ✅ Live expressions

## Flashing

### From STM32CubeIDE

1. **Right-click** on project → **Run As → STM32 C/C++ Application**
2. Or use **Debug** mode (recommended for testing)

### Using DFU (Device Firmware Update)

```bash
# Put device in DFU mode (BOOT0=1, reset)
sudo dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D Debug/canable-fw.bin
```

### Using Makefile

```bash
make flash  # Uses OpenOCD with ST-LINK
```

## Troubleshooting

### "Project not recognized"

- Make sure all these files exist:
  - `.project`
  - `.cproject`
  - `canable-fw.ioc`
  
### Build errors

- **Clean the project**: Project → Clean...
- **Rebuild indexes**: Project → C/C++ Index → Rebuild
- **Check toolchain**: Window → Preferences → STM32Cube → Toolchain

### CubeMX code generation issues

- Backup your code before regenerating
- Use `/* USER CODE BEGIN */` and `/* USER CODE END */` comments to protect custom code
- Review generated code carefully after regeneration

### Indexer not working

- Right-click project → Index → Rebuild
- Or: Window → Preferences → C/C++ → Indexer → Enable indexer

## Switching Between IDE and Makefile

Both build systems work independently:

```bash
# Clean both
make clean
rm -rf Debug/ Release/

# Build with Makefile
make -j

# Build with IDE
# Use IDE's Build Project button
```

## Additional Resources

- [STM32CubeIDE User Guide](https://www.st.com/resource/en/user_manual/um2609-stm32cubeide-user-guide-stmicroelectronics.pdf)
- [STM32H7 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0468-stm32h723733-stm32h725735-and-stm32h730-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [FDCAN Peripheral Guide](https://www.st.com/resource/en/application_note/an5348-fdcan-peripheral-on-stm32-devices-stmicroelectronics.pdf)

## Notes

- The `.ioc` file is the source of truth for hardware configuration
- Both Makefile and IDE builds use the same source files
- IDE-specific files (`.cproject`, `.project`) are now tracked in git
- Build artifacts (`Debug/`, `Release/`, `build/`) are ignored by git
