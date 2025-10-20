# 🚀 Pi Zero Setup Guide

## WHAT YOU NEED:
- Pi Zero 2 W (for WiFi!)
- **MicroSD card: 8-32GB**
- USB cable for power
- Your Mac for setup

---

## STEP 1: Flash SD Card (30 min)

1. **Download Raspberry Pi Imager**: https://www.raspberrypi.com/software/
2. **Insert SD card into Mac**
3. **Open Imager**:
   - Device: "Raspberry Pi Zero 2 W"
   - OS: "Raspberry Pi OS Lite (64-bit)"
   - Storage: Your SD card
   - **Click Settings (⚙️):**
     * Hostname: `winder`
     * Enable SSH: **YES**
     * Username: `pi` 
     * Password: (your choice - remember it!)
     * WiFi: (enter your WiFi credentials)
     * Timezone: Your timezone
4. **Click WRITE** (takes ~5 min)

---

## STEP 2: Enable UART (CRITICAL!)

After flashing, **BEFORE ejecting the SD card**:

1. The SD card mounts as two volumes: `bootfs` and `rootfs`
2. Open `/Volumes/bootfs/config.txt` in a text editor
3. **Add these lines at the end:**
   ```
   enable_uart=1
   dtoverlay=disable-bt
   ```
4. **Save and eject** the SD card

**Why?** This enables the hardware UART on GPIO 14/15 and disables Bluetooth (which conflicts with the UART).

---

## STEP 3: Boot & SSH

1. **Insert SD** into Pi Zero
2. **Power up** (wait 60-90 seconds for first boot)
3. **On your Mac**, open Terminal:
   ```bash
   ssh pi@winder.local
   ```
4. Enter the password you set
5. **You're in!** 🎉

**Troubleshooting:**
- If `winder.local` doesn't work, try finding the IP:
  ```bash
  ping winder.local
  # or check your router's DHCP list
  ```

---

## STEP 4: Install Python Dependencies

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Python serial library
sudo apt install -y python3-pip python3-serial

# Alternative: use pip
pip3 install pyserial
```

---

## STEP 5: Disable Serial Console (Important!)

The Pi Zero uses the UART for a login console by default. We need to disable that:

```bash
sudo raspi-config
```

Navigate to:
- **3 Interface Options**
- **I6 Serial Port**
- **"Login shell over serial?"** → **No**
- **"Serial port hardware enabled?"** → **Yes**
- **Finish** → **Reboot**

After reboot, reconnect via SSH.

---

## STEP 6: Verify UART is Ready

```bash
ls -l /dev/serial0
```

Should show:
```
lrwxrwxrwx 1 root root 7 Oct 20 12:00 /dev/serial0 -> ttyAMA0
```

If you see `ttyS0` instead of `ttyAMA0`, the Bluetooth wasn't disabled correctly. Go back to Step 2.

---

## STEP 7: Add User to dialout Group (Permission Fix)

```bash
sudo usermod -a -G dialout $USER
# Log out and back in for this to take effect
```

---

## STEP 8: Copy Test Files to Pi Zero

On your **Mac**:

```bash
cd /Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder
he
```

This copies the `pi_zero` folder (with `test_uart.py`) to the Pi.

---

## STEP 9: Test (Once Pico is wired up)

On the **Pi Zero** (via SSH):

```bash
cd ~/pi_zero
python3 test_uart.py
```

**Expected output:**
```
==================================================
Pi Zero UART Test
==================================================
✅ Opened /dev/serial0 @ 115200 baud

📤 Sending: PING
📥 Waiting for response...
✅ Received: PONG

🎉 SUCCESS! UART communication working!
```

---

## Complete Setup Checklist

- [ ] Flash SD card with Raspberry Pi OS Lite
- [ ] Edit `config.txt` to enable UART and disable BT
- [ ] Boot Pi Zero and SSH in
- [ ] Install `python3-serial`
- [ ] Disable serial console with `raspi-config`
- [ ] Verify `/dev/serial0` exists
- [ ] Add user to `dialout` group
- [ ] Copy test files to Pi
- [ ] Wire up Pi ↔ Pico (see `WIRING.md`)
- [ ] Build and flash Pico firmware
- [ ] Run `test_uart.py`

---

## Next Steps

Once UART test works:
1. Build full winding control system
2. Add motor commands
3. Add encoder feedback
4. Create web interface (optional)

🎸 **You're ready to wind some pickups!**

