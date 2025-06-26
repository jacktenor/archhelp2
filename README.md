# ArchHelp

**Warning**: this installer performs destructive actions on disks. Make sure
you understand what it does before running it.

## Running the installer

The application expects to have root privileges because most of the operations
(partitioning, formatting and mounting) rely on `pkexec`/`sudo`. Launch the
binary with `sudo` or `pkexec`:

```bash
sudo ./ArchHelp
```

Without elevated privileges the formatting step will fail and the ISO will not
be copied to `/mnt`, which results in the "Arch Linux ISO not found" error on
the final page.

1. **Download ISO** – press the *Download* button and wait for the progress
   bar to complete.
2. **Prepare Drive** – select the target disk and click *Prepare Drive*.
3. **Prepare for EFI** – use this if you want UEFI boot. It converts the
   drive to GPT and creates an EFI System Partition plus a root partition.
4. **Create Default Partitions** – optional helper for creating a simple
   layout (legacy BIOS mode).
5. **Continue through the wizard** – follow the prompts to mount the ISO and
   install the base system.

Make absolutely sure you selected the correct drive – the installer will wipe
it completely.

## Building from source

Ensure the Qt development tools are installed. On Debian or Ubuntu based
distributions you can install them with:

```bash
sudo apt-get install qtbase5-dev qt5-qmake build-essential
```

Then run `qmake` followed by `make` in the project directory:

```bash
qmake ArchHelp.pro
make -j$(nproc)
```

The resulting `ArchHelp` binary can be found in the same folder.
