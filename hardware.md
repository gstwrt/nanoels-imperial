**This software and instructions are provided as is, without warranty of any kind. This is a hobby project. Using this might damage your equipment, cause injury or death. Use at your own risk.**

# Mechanical part of the ELS

![image](https://user-images.githubusercontent.com/517919/192345232-676fb1cd-a764-4a84-8c82-6f79fa8574a3.png)

Any electronic lead screw requires connecting an encoder to the lathe spindle and motor to the leadscrew.
This page provides some general guidance but every lathe is different, start a new [discussion](https://github.com/kachurovskiy/nanoels/discussions) if you have questions.

# Encoder

Tested with the [following 600 steps encoder](https://www.amazon.de/-/en/gp/product/B015GYY7XU). They are also available on eBay/AliExpress as "optical rotary encoder".

## Encoder base

![Encoder base](https://github.com/kachurovskiy/nanoels/raw/main/h1/encoder/encoder-base-38.5mm-hole-m4-screw.png)

[STL file](https://github.com/kachurovskiy/nanoels/raw/main/h1/encoder/encoder-base-38.5mm-hole-m4-screw.stl)

To be glued to the lathe wall with the double-sided carpet sticky tape. Holds well if surfaces are properly cleaned with isopropyl alcohol or acetone.

Using an M4 set screw is optional.

Alternative encoder base allowing for a thicker cable: https://www.thingiverse.com/thing:5156081

## Encoder gear

![Encoder gear](https://github.com/kachurovskiy/nanoels/raw/main/h1/encoder/encoder-gear-60t-6.1mm-bore.png)

[60 teeth gear STL file](https://github.com/kachurovskiy/nanoels/raw/main/h1/encoder/encoder-gear-60t-6.1mm-bore.stl)

Check the distance between the spindle gear and the lathe housing: for the provided conical gear to fit, it has to be 23mm.

Encoder gear should fit loosely to the spindle allowing for it to move ~1 degree back and forth when the spindle is idle - it reduces the noise and forces applied to the encoder.

For spindles with 56 teeth, try [56 teeth gear model by kingjamez](https://www.thingiverse.com/thing:4754021).

# Stepper and driver

**People report problems with open loop steppers losing steps. Closed loop stepper and driver are strongly recommended. They're 50% more expensive but well worth it.**

I used the following one but most Nema 23 or higher steppers should work.

- Nema 23 Stepper Motor Bipolar 1.8deg 3.0 Nm 4.2A 57x57x113mm 4 Wires Open Loop
- DM556Y Driver 1.7-5.6A DC20V~50V

Cheap drivers like TB6600 are not recommended, they are very rough and noisy.
DM556 from AliExpress is not very good either, they require reducing acceleration and max speed in the settings or it will lose steps during jogging (`PULSE_DELTA_US` from `7` to `2` and `PULSE_MAX_US` from `2000` to `1500`).
Generally, paying for a better brand such as "Rtelligent" or "STEPPERONLINE" is worth the money as their drivers work noticeably better than brandless black boxes.

It's suggested to run the stepper in the 200 steps mode or 400 if your driver doesn't support full steps.
Microstepping will lower maximum usable rpm 2x for each microstep increase.

## Stepper power supply

48V power supply is used on NEMA 23 for maximum power (check your driver max voltage) but 24V might also work for light turning.

## Stepper mount

![Stepper mount](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-mount-nema23-28mm-hole.png)

[STL file](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-mount-nema23-28mm-hole.stl)

Dimensions of the lead screw front bearing housing for the provided 3D-printed part to fit: 28mm outside diameter, 12mm wide.

Stepper should rest on e.g. a piece of rubber when attached to the lathe - provided 3D printed part is only held by friction and is not designed to hold the full weight of the stepper long term.

A rock-solid but more expensive (35€) option is to order a 10mm steel laser cut out stepper mount: https://imgur.com/a/exO5Atl

It's also possible to cut out the stepper mount manually from plywood: https://imgur.com/a/CSmeSm2 ([PDF](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/nema23-mount-plywood.pdf))

Alternative stepper mount with a pinch bolt: https://www.thingiverse.com/thing:5156087

## Stepper to leadscrew

When connecting the gears, make sure to leave ~0.5mm space between them for optimal threading. This can be achieved e.g. by temporarily placing a piece of paper between them. Add grease and spread it between the gear teeth, remove the excess.

### Option 1: plastic gear

![Stepper gear](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-gear-nema23-50t-10mm-10mm.png)

[STL file](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-gear-nema23-50t-10mm-10mm.stl)

This gear is designed to fit the NEMA 23 shaft (10mm diameter / 9.5mm narrow dimension) without any set screws. Gear is 10mm wide, entire part is 20mm wide.

Using this gear can be very noisy on certain RPMs.

### Option 2: adapter for the lathe gear

![Stepper gear adapter](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-to-gear-adapter-12mm-10mm.png)

[STL file](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/stepper-to-gear-adapter-12mm-10mm.stl)

If you have 2 metal gears that add up to 100 tooth together (e.g. 2x50 tooth gears) that came with the lathe, it's possible to simply use one of them on the leadscrew and one on the stepper with the help of the adapter above. It can be somewhat noisy on certain RPMs but otherwise works well.

### Option 3: HTD 5M belt connection

![Stepper HTD 5M pulleys](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/htd5m-assembly.jpg)

[STL pulley 8mm bore](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/htd5m-16t-8b.stl)

[STL pulley 10mm bore](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/htd5m-16t-10b.stl)

[STL pulley 12mm bore](https://github.com/kachurovskiy/nanoels/raw/main/h1/stepper/htd5m-16t-12b.stl)

This is the best stepper to leadscrew connection method that I tried so far but it requires printing 2 parts above and buying a 180mm HTD 5M belt which is not expensive (2-10€) but can be hard to find.

This is the quietest option that also doesn't need greasing. 2 pulleys and a belt fit tight on the normal stepper mount shown above without a need for a tensioner.

16T-5M pulleys with various inner bore diameters are also available on AliExpress for 5-6€ each.

