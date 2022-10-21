**This software and instructions are provided as is, without warranty of any kind. This is a hobby project. Using this might damage your equipment, cause injury or death. Use at your own risk.**

# NanoEls-imperial
Cheap DIY Electronic Lead Screw (ELS) based on Arduino Nano for metal lathes.   Supports Metric and Imperial measurements

This is a modified version of NanoELS with added support for inch measurements and tpi pitch.  Only h2.ino has been modified.  Internal measurements are stored in thousandths of a mm. 

## Other Changes:
 - Changed Display format
 - Unlimited list of preset pitches, unlimited list of step sizes

## Known Bugs
 - Cumulative error when moving position in inch or tpi mode due to rounding (minor effect when step is set to 0.001" or 0.01")
 - Not currently possible to directly set non-integer tpi pitches.   They can be set using pitch mode.   ie. 11.5tpi can only be set as 0.087" pitch

## Manual:
 - B1 Change Display Mode (Angle/Rpm/Version Info)
 - B2 Cycle Move Step size (s/m/l)
 - B3 Cycle Preset Pitch
 - B4 Cycle Unit Mode (mm/in/tpi)
 - B5 Disabled/Debug Info
