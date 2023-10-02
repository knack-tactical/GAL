# GAL
A 3D printed, gun aiming laser device

[Link to Falstad Circuit](https://tinyurl.com/2n3jw32g)

## Updated hardware from Swampdonkey's verison

Digi-key BOM: https://www.digikey.com/en/mylists/list/YLE6AURR2X

[M4x6x6mm Heatset Inserts](https://a.co/d/dzFERyy)

[850nm 3W LED](https://a.co/d/1zIGPuo)

[LED Lens](https://a.co/d/ejgL7wE)

## Things still in development
I am still working on a better retaining method for the laser zeroing, so any suggestions are appreciated!

I have designed a PCB for the GAL, which can be found [here](https://github.com/knack-69/GAL/blob/main/GAL_PCB.zip).

Also there are a few lasers I have tested with. The 980nm one is supposedly [30mW](https://www.aliexpress.us/item/2261799874302117.html). The other is a [5mW 850nm laser](https://www.aliexpress.us/item/2251832689759781.html) which is smaller and the one I will probably go with. For the 850nm laser, I bought the 'adjustable glass623' version, which performs well. Lots of testing on this front is needed. If you have any suggestions for lasers in this band, it would be appreciated as well.

## Suggested Print/Support Settings

I'd like to preface this by saying I'm not a 3D printing guru, my printers usually prints decently when I tell it to, so I don't mess around too much with the settings.
All .stl files are already in the suggested print orientation.

I would recommend not using a super large layer height (max I did was 0.18) as both male and female threads do better with smaller layer heights. Most of these pieces do not have very much interal wall volume, so using 3 walls and a lower infill (20%) should be plenty fine. Same goes with Top and Bottom layers. I usually go with 3 bottom layers and 3, which will make sure you don't see any of the infill through the top, which is especially important on the Lid, as the whole top surface is what you'll be looking at.

TL;DR: Layer Height: 0.18 | Walls: 3 | Bottom: 3 | Top: 3 | Infill 20%

### GAL Body

- Supports everywhere
- I had no issues with adhesion/warping

### GAL Lid

- Supports only on buildplate
  - This is because of the vertical laser adjustment nut hole is very small and getting supports out of it sounds terrible
- I had some issues with warping, but that might have just been me.
  - My issues with warping were because the long edge of the lid was directly at the edge of a piece of blue painters tape, which I use for bed adhesion.


### GAL Rail Mount. Battery Cover, Lens Lid

- No supports needed

### Selector Cap

- Supports everywhere
