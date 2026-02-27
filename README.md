# Freezer Defrost Timer (TIM01-6.2)

Automatic defrost cycle controller for a freezer compressor, built on **ATtiny13**.

## How It Works

The firmware cycles through three states:

| State | Duration | Description |
|-------|----------|-------------|
| **Cooling** | 8 hours | Compressor runs normally |
| **Active Defrost** | 40 min | Heater is on; defrost thermostat monitored |
| **Passive Defrost** | 2 min | Brief pause after thermostat signals defrost complete |

- A **defrost thermostat** (PB2) can cut the active defrost short when temperature rises above threshold.
- The current cooling timer value is periodically saved to **EEPROM**, so the cycle resumes correctly after a power outage.
- A **watchdog timer** (2 s) provides hardware-level fault recovery.
- A **test button** (PB1) lets you fast-forward to the next state transition (5 s).

## Pin Assignment (ATtiny13)

| Pin | Direction | Function |
|-----|-----------|----------|
| PB0 | Output | MOSFET + relay (0 = compressor, 1 = defrost heater) |
| PB1 | Input (pull-up) | Test button (active low) |
| PB2 | Input | Defrost thermostat (1 = cold, 0 = warm) |
| PB3 | Output | Active defrost LED |
| PB4 | Output | Passive defrost / status LED |

## Building

Open `tim01-6.2.atsln` in **Atmel Studio 6.2** (or Microchip Studio) and build. The toolchain is AVR-GCC targeting ATtiny13.

A pre-built `.hex` file is available on the [Releases](../../releases) page.

## Original Device

This firmware is a replacement timer for the **TIM-01** defrost controller.  
Datasheet (PDF): [tim-01_155.pdf](https://monitor.espec.ws/files/tim-01_155.pdf)

## License

MIT
