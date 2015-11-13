[Wireless Networking Tips](Basic.md)
> http://www.gowifi.co.nz/wifi-basics/getting-the-most-out-of-your-wireless-network.html
Important



Make sure you power off any wireless equipment before you remove the antenna to change it, as you can damage the radio transmitter if you don't.

Wireless Forums



If you are looking for a place to post ideas and problems you have regarding wireless networking and associated equipment visit http://www.wirelessforums.org/.  They have over 1200 members and it’s a great place to find solutions to your wireless questions.
EnGenius Firmware, Drivers and Technical Support

For the latest drivers and technical information for the Senao/EnGenius product see here for more details http://www.engeniustech.com/datacom/support/
MikroTik Technical Support

The best place to find out more information about setup procedures, issues or anything to do with the MikroTik RouterBoards is the MikroTik Forums. Vist them here http://forum.mikrotik.com/
Coax Seal Tape



Many wireless installers forget about this vital part of their installations and end up with expensive repairs a few months later.  It is very important to seal all outside connector joints to prevent moisture from entering. It is best to use a self bonding type tape as these tend to give the best seal. Check under our Tools section for “Coax Seal Tape”

Getting the most out of your Wireless Network



If you are having problems with your wireless signal there are some things to check for first. Look for other 2.4 GHz equipment nearby like cordless phones, blue tooth devices, microwave ovens etc. Your neighbours may also have a wireless access point on the same channel as yours. A good program to check this is Netstumbler, which is a free 802.11b/g scanning program you can download off the net. The makeup of the building will also affect the signal, things such as metal studs in walls, concrete, concrete fibreboard walls, aluminium cladding, and foil-backed insulation in the walls or under the cladding, pipes, electrical wiring and furniture. Tinted windows will block 2.4 GHz signals also, as will wire mesh with holes smaller than a quarter wavelength (about 2.5cm), wire mesh with holes up to 12.5cm will also affect signal as well.

Here are some common building materials with their average attenuation.

Plasterboard wall: 3dB

Glass wall with metal frame: 6dB

Block wall: 4dB

Office window: 3dB

Metal door: 6dB

Metal door in brick wall: 12.4dB

Keep your access point as high as possible and away from metal objects and concrete walls as these can block and/or reflect the signal causing signal degradation. When Using PCI client adaptors in the back of a desktop PC the signal can be blocked by the PC itself and the signal can be badly affected. One solution for this is to use our rubber duck antenna on a magnetic base with a 1.5 metre cable so it can be lifted up away from behind the PC. PCMCIA client adaptors in laptops can also have problems as their antennas are normally small and lay horizontal with the computer, this can reduce signal quality and strength as well. The performance of a PCMCIA Wifi card is often well below that of the laptops that have built in cards and antenna, as the antenna are normally larger and are built into the screen so are in a more vertical angle. Some PCMCIA cards come with an external connector so a better, higher gain antenna can be plugged in. If you use a USB client adaptor make sure you use the extension cord and base to keep the USB unit away from the computer and high up for best performance. If you are going between floors on a multi story house tilt the antenna to give better coverage as shown in the picture below.

Higher antenna gain means that the signal will be focused more in one direction as in the picture below; you do not get more power from antenna gain, just a more focused beam of energy to give a greater distance.


Diagram 1.



Gain Description



Using the highest gain antenna is not always the best. As a higher gain antenna has a more narrow focused signal beam, it does travel further, but there is a smaller sweet spot. This can make point to point links more tricky to align, and they can easily go out of alignment if the antenna mast moves slightly. If you have too higher gain on an omni directional antenna, you can have problems with multipathing as the narrow signal beam can reflect off objects like metal and concrete in the area, and the different reflections can arrive at the client radio out of phase with the main signal. Also with a high gain omni the signal is focused into a more narrow vertical beamwidth. This can mean the areas above and below the antenna may have poor coverage. When choosing a Rubber duck antenna for your building, you need to consider your surroundings first as mentioned above, some times the 7dBi antenna will give you better performance than the 9dBi.  If your problem is due to distance in a wooden house the 9dBi is good, but if it is due to metal and concrete blocking the signal the 7dBi can be better as the 9dBi has a more concentrated beam and will reflect more off the surfaces, causing multipathing problems that reduce signal quality. The 5dBi and 7dBi antenna will give you the boost you need in most situations.

Radio Frequency Line of Sight (RF LOS)


Radio frequency LOS is very important when designing point to point links, as RF LOS is not the same as visual LOS. The Fresnel Zone is an important part of the RF link because it defines an area around the LOS that can introduce RF signal interference if blocked. Objects in the Fresnel Zone such as trees, hilltops, and buildings can diffract, reflect or absorb signal away from the receiver, changing the RF LOS. These objects can cause degradation or complete signal loss. To have a viable RF link 60% of the Fresnel Zone must be free of obstructions. The Fresnel Zone is at its widest in the middle of the link. There are formulae that can work out what clearance you need at certain points along the link but most of the time it is best just to make sure there is plenty of gap, and do a wireless site survey first. A common misconception is that you can use a high gain directional antenna to "blast" through trees, buildings, concrete and other objects, this almost never works. A directional antenna that can transmit 30km in free space won't even go 100 meters if there is an iron roof and a tree in the way. The best way to penetrate through objects is to use a higher powered access point.



Diagram 2.



Fresnel Zone Diagram





Note: It is important when using antennas for longer distance communications that you have similar strength radios and similar gain antennas at each end. If you don't you run the risk of being able receive signal, but not transmit back for two way communication. Your link will only be as good as the weakest end. It is no good putting a 15dbi omni on your roof and trying to use a laptop to communicate to it 2km away, as you will be able to receive signal from it, but the laptop will not be able to transmit enough signal back.

Antenna Polarization



It is important when installing antennas that both ends have the same polarization, otherwise there will be almost no communication between them at all. Polarization is the relationship between the antennas physical orientation and the electric field that is parallel to the radiating element. Omni directional antennas tend to have vertical polarization due to the fact that they stand vertically. This can be why some PCMCIA cards don't perform well as they sit horizontal with the laptop computer while the access point has a vertical mounted antenna. With an access point this can be improved by tilting the rubber duck antenna sideways, but not always as there is normally enough signal bouncing around off the walls in most buildings anyway that one reflected signal will be the correct polarization. Most directional antennas can be installed in either vertical or horizontal polarization; you just have to have the same at both ends.

Good Signal Can't be Guaranteed



Even when all the above precautions are taken into account, no one can guarantee a good link, as there are many more factors that could cause problems. Go Wireless NZ sells equipment in good faith and can only accept returns due to faulty equipment as per the warranty. If equipment must be returned for any other reason there will be a 15% re-stocking fee, as often the equipment cannot be re-sold as new again.

Pigtails and Extension Cables



When ordering pigtails and extension cables to connect your access point to an antenna, try and keep the cables as short as possible for best performance. For longer runs we use CA-400 which has a loss of around 0.22dB per metre at 2.5Ghz, even though this seems low it still adds up on longer cables. We use Times Microwave LMR195 for shorter cables under 2 metres’ and this has a loss of around 0.6dB per metre at 2.5GHz. Remember to take these losses into account when designing you wireless network. It is also very important to seal all outside connector joints to prevent moisture from entering them.  Once a coax cable has had moisture in it, it will never be able to be used again.


Last Updated ( Friday, 25 April 2008 )