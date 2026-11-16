**Subject: Ending the Zadig step for users: your call on the PSoC trees**

Stew,

Zadig is the step new users trip over most, so I'd like to get rid of it. Here's what it would take, so you can decide whether your trees join in.

**The idea**
Windows 10 and 11 have their own USB driver for devices like ours, called WinUSB. A device can tell Windows "use WinUSB for this interface" while it's being plugged in. Windows then loads that driver by itself: no Zadig, no driver download and no certificate. Many devices do this already (Microsoft calls it "OS descriptors").

**What changes in the firmware**
- A small fixed block of data, plus an answer to one extra question Windows asks at plug-in. It applies only to the control interface (interface 0); the audio interface is untouched.
- The USB device version number goes up by one, because Windows remembers what a radio said the first time it saw it. The new number makes Windows ask again.
- Nothing changes in how the radio runs, sounds or calibrates. It only affects the moment it's plugged into a Windows PC.
- Other gadgets that share our USB ID (16C0:05DC) aren't affected, because only a radio that gives the answer gets WinUSB.

**What changes on the PC side (mine)**
The Windows server (ms-sdr) moves to the newer USB library that works with WinUSB. The Pi server already uses that library, so I'll reuse its code. I'll keep the old Zadig driver working too, so nobody has to change anything.

**What users see**
- New PC, radio with the new firmware: plug in and it works.
- PC already set up with Zadig: no change, still works.
- Radio with older firmware: Zadig as today, until its firmware is updated.

**Black Pill flashing too**
When the pill is put into flash mode, its small bootloader takes over the USB connection, and Windows sees it as a different device. Today that also needs Zadig, once per PC. The bootloader is our own code, so it can give Windows the same "use WinUSB" answer. Then flashing a pill from Windows needs no Zadig either.

The bootloader can only be put on a pill with an ST-Link. That's no problem: we haven't sold any pill boards, and every pill gets its first flash with an ST-Link when it's built anyway. So every pill that ships gets the new bootloader from day one. Only our own pills (yours and mine) would need it burned again with the ST-Link.

So for Black Pill radios, users would never need Zadig at all, either to run the radio or to update it. PSoC radios already update without Zadig (LOADER uses a driver built into Windows), so with the new firmware they wouldn't need it either.

**Raspberry Pi and Linux: no change**
Linux never asks the question Windows asks at plug-in, so it simply ignores the new answer. The radio shows up on the Pi exactly as it does now. The Pi servers, the USB access rules, flashing and the audio all stay as they are. I checked that nothing on the Pi or Linux side looks at the USB device version number, so raising it does no harm there. After each new firmware I'd do one quick run on the Pi (receive, transmit, a flash) to confirm nothing changed.

**Order**
1. I do the Black Pill and Proficio MKII PTT first and test them on my rig.
2. Once they work, I send you the exact change from MKII PTT. It should be a small, self-contained piece you can copy into the other trees (MKII ATU, Ultimus, Geminus, Legacy).
3. Your trees go when it suits you, or not at all. Radios on firmware without it simply keep using Zadig.

**Your call**
- Are you OK with this going into the PSoC firmware in general?
- Do you want to do your trees yourself from my example, have me do them for you to check, or leave them as they are for now?

No rush on an answer.

Ron
