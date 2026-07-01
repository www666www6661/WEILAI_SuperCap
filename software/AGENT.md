# AGENT.md

## Scope
Instructions for AI coding agents working in this STM32G474 project.

## Project Facts
- Build system: CMake (`CMakePresets.json`)
- Generated code: `Core/`, `Drivers/`, `.ioc`
- Custom code: `bsp/`, `Src/dev/`, `Src/comp/`
- Important ADC files: `bsp/bsp_adc.c`, `bsp/bsp_adc.h`, `Core/Src/adc.c`, `Core/Src/main.c`, `Core/Src/stm32g4xx_it.c`

## Hard Rules
1. Prefer modifying only **one file per request** unless a tightly-coupled fix requires more.
2. Prefer minimal, verifiable changes.
3. In CubeMX-generated files, prefer `USER CODE BEGIN/END` regions.
4. Do not assume an IRQ handler existing means NVIC is enabled.
5. During hardware bug fixing, prefer instrumentation and register evidence over broad refactors.

## Build
Use Debug unless user asks otherwise.

```bash
cmake --build /home/concon/Documents/SuperCap/build/Debug 2>&1 
```

## OpenOCD / GDB
Use **SWD** for target connection.

elf file location : './build/Debug/SuperCap_WEILAI.elf'

### Session-derived toolchain pitfalls
- Before claiming a hardware fault, first distinguish **target power present** from **SWD actually connected**. `VTarget` being valid does **not** mean DP/AP communication is working.
- If OpenOCD reports `cannot read IDR`, prioritize checking probe wiring / target connection / probe type before analyzing firmware logic.
- Do not assume the probe is J-Link unless the user context supports it. If connection fails, explicitly confirm whether the probe is **J-Link** or **ST-Link** and switch the OpenOCD interface script accordingly.
- Prefer adding an explicit adapter speed for first connection attempts, e.g. `-c "adapter speed 1000"`. If unstable, try lower speeds before concluding the target is inaccessible.
- If OpenOCD says a port is already in use, check for an existing `openocd` process before starting another one.
- When using OpenOCD one-shot commands, prefer `shutdown` over `exit` to avoid hanging server sessions and to make command output easier to capture.
- When OpenOCD console output does not show expected `mdw` results, do not assume the reads failed; re-run with simpler one-shot command structure and reduced command count.
- On this machine, `arm-none-eabi-gdb` may be absent. Check debugger availability in this order:
  1. `arm-none-eabi-gdb`
  2. `gdb-multiarch`
  3. system `gdb`
- If only host `gdb` is available, do **not** assume it can properly drive OpenOCD for Cortex-M register/symbol inspection. Prefer pure OpenOCD register/memory reads, or ask the user to provide/install a cross GDB if deeper symbol-aware debug is required.
- A plain host `gdb` talking to OpenOCD may show symptoms such as:
  - `monitor command not supported by this target`
  - unknown architecture warnings
  - truncated remote register packets
  These should be treated as **tool mismatch**, not target-firmware evidence.
- For register inspection via OpenOCD, prefer direct peripheral base addresses (`mdw`) over debugger expressions when cross-GDB support is uncertain.
- For RAM validation, always resolve symbol addresses from the current ELF (for example with `nm`) before reading memory, and do not assume a previous session's address is still valid.
- When validating “ADC is converting”, remember that halting the core freezes the observation point. Prefer `reset run; sleep N; halt; mdw ...` rather than reading immediately after reset-halt.
- When validating runtime buffers, confirm that firmware was actually reflashed in the same session. If needed, use OpenOCD `flash write_image erase <elf>` in the same command chain.
- If command output capture is flaky through telnet/netcat against OpenOCD, prefer direct OpenOCD `-c` command chains instead of piping commands into port 4444.
- When using `addr2line`, occasional DWARF warnings do not automatically invalidate the resolved location if a sensible source path is still returned.

Typical OpenOCD:

```bash
openocd -f interface/jlink.cfg -c "transport select swd" -c "adapter speed 1000" -f target/stm32g4x.cfg
```

If the user explicitly says they are using ST-Link, switch to the matching interface config.

Typical GDB:

```bash
arm-none-eabi-gdb /home/concon/Documents/SuperCap/build/Debug/<firmware>.elf
```

Debugger availability fallback:

```bash
which arm-none-eabi-gdb || which gdb-multiarch || which gdb
```

OpenOCD process check:

```bash
ps -ef | grep openocd | grep -v grep
```

Common GDB sequence:

```gdb
target extended-remote :3333
monitor reset halt
load
monitor reset halt
```

Useful breakpoints:

```gdb
b ADC1_2_IRQHandler
b HAL_ADCEx_InjectedConvCpltCallback
b HardFault_Handler
```

Useful ADC checks:

```gdb
p/x ADC1->ISR
p/x ADC1->IER
p/x ADC1->JSQR
p/x ADC1->JDR1
p/x ADC1->JDR2
```

Useful OpenOCD direct reads when GDB is unavailable/mismatched:

```bash
openocd -f interface/jlink.cfg -c "transport select swd" -c "adapter speed 1000" -f target/stm32g4x.cfg \
  -c "init" \
  -c "reset run" \
  -c "sleep 1000" \
  -c "halt" \
  -c "mdw 0x50000000 8" \
  -c "mdw 0x50000100 8" \
  -c "shutdown"
```

Useful symbol lookup before RAM inspection:

```bash
arm-none-eabi-nm /home/concon/Documents/SuperCap/build/Debug/SuperCap_WEILAI.elf | grep g_bsp_adc1_dbg
```

Do not assume old RAM addresses are still valid after a rebuild.

## ADC Debug Focus
For ADC1 injected dual-context issues:
- verify expected context, actual JSQR, ISR flags, and JDR values together
- do not trust JDR data without a valid completion window
- prefer adding small debug counters/snapshots instead of large rewrites
- compare before/after OpenOCD evidence after each fix

For this project specifically, also avoid these false conclusions:
- `bsp_adc_Buf == 0` alone does not prove trigger routing is wrong; it may also mean ADC start/enable failed or result paths were never armed.
- `IER == 0` / inactive ADC status at runtime is stronger evidence of startup-chain failure than of HRTIM trigger misrouting.
- If `main` loop is running but ADC buffers remain zero, check `bsp_adc_start()` return values before changing trigger topology.

### Current project-specific ADC context
- This project's current ADC issue is on **ADC1 injected queue handling**, not regular ADC DMA.
- Injected queue currently has 2 contexts:
  - `context1`: `HRTIM TRG4 -> CH1, CH3`
  - `context2`: `HRTIM TRG2 -> CH2, CH8`
- Injected results are currently processed by polling in the main loop, so timing windows matter.
- Do not assume `JSQR == expected_next_context` alone means the current `JDRx` values are valid.
- When debugging, inspect `ISR`, `JSQR`, `JDR1`, `JDR2`, last-context state, and debug counters together.

### Preferred hardware validation loop
1. Make one small code change.
2. Build Debug firmware.
3. Flash firmware.
4. Use OpenOCD to inspect both ADC registers and RAM debug variables.
5. Compare before/after evidence before making another change.

### Preferred troubleshooting order for probe/debug issues
1. Confirm probe type (J-Link vs ST-Link).
2. Confirm only one OpenOCD instance is running.
3. Confirm SWD wiring and target voltage.
4. Retry with explicit lower adapter speed.
5. Confirm cross-GDB availability before attempting symbol-aware remote debugging.
6. If cross-GDB is unavailable, fall back to OpenOCD `mdw`-based inspection.

## Working Style
When reporting:
- separate facts from hypotheses
- give exact commands when asking for hardware validation
- keep changes small and easy to revert

## When using the `patch` tool:
- each invocation can modify only one file; if you need to modify multiple files, you must invoke the `patch` tool separately for each modification.


