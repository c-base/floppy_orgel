#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
FlyFi - Floppy-Fidelity
=======

Created to fulfill all your floppy music needs.

Created on Tue 06-01-2013_05:17:42+0100
@author: Ricardo (XeN) Band <xen@c-base.org>,
         Stephan (coon) Thiele <coon@c-base.org>

    This file is part of FlyFi.

    FlyFi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FlyFi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FlyFi.  If not, see <http://www.gnu.org/licenses/>.

    Diese Datei ist Teil von FlyFi.

    FlyFi ist Freie Software: Sie können es unter den Bedingungen
    der GNU General Public License, wie von der Free Software Foundation,
    Version 3 der Lizenz oder (nach Ihrer Option) jeder späteren
    veröffentlichten Version, weiterverbreiten und/oder modifizieren.

    FlyFi wird in der Hoffnung, dass es nützlich sein wird, aber
    OHNE JEDE GEWÄHELEISTUNG, bereitgestellt; sogar ohne die implizite
    Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
    Siehe die GNU General Public License für weitere Details.

    Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
    Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
"""

__author__ = "Ricardo (XeN) Band <xen@c-base.org>, \
              Stephan (coon) Thiele <coon@c-base.org>"
__copyright__ = "Copyright (C) 2013 Ricardo Band, Stephan Thiele"
__revision__ = "$Id$"
__version__ = "0.1"


import serial
import struct
import math

class FloppyOut():

    class MidiChannel():
        def __init__(self, active, floppy_channel, serial_port):
            self.active = active
            self.floppy_channel = floppy_channel
            self.serial_port = serial_port


    def __init__(self):
        self.MAX_CHANNELS = 16
        self.ARDUINO_RESOLUTION = 40 # the timer of the arduino fires every 40 miliseconds
        self.BAUDRATE = 9600
        self.midi_channels = []
        self._used_serial_ports = {}

        #################################
        # generate frequencies of tones #
        #################################
        
        # The floppies are only able to play tones until about tone 71.
        # To prevent damage to the floppy drives, the upper value should be limited.
        # Tones which are too low may not sound good anymore but won't damage the drive.
        # The lowest value MUST not be 0, since this acts as note off
                 

        first_note = 1 # B -1 ???
        last_note = 71 # B4
        
        
        # First generate a list with zeros for all 108 midi notes.
        # midi notes with a frequency of zero won't be played by the microcontroller.
        
        self.midiFrequencies = [0 for i in range(128)]
        self.midiHalfperiods = [0 for i in range(128)]
        # The first midi note 0 has a frequency of 8.17575Hz
        # Based on this, all other midi notes frequencies can be calculated by multiplying the 
        # frequency of the first midi note 0 (8.17575Hz) by the factor which results by 
        # extracting the twelfth root of two with the power of the target midi note.
      
        for midi_note in range(first_note, last_note + 1):
            frequency = 8.17575 * math.pow(2, midi_note / 12.0)
        
            self.midiFrequencies[midi_note] = frequency
            self.midiHalfperiods[midi_note] = (1000000.0 / frequency) / (2.0 * self.ARDUINO_RESOLUTION) # period in microseconds
        
       
        # TODO: Load mappings from a settings file here
        for i in range(1, self.MAX_CHANNELS + 1):
            self.midi_channels.append(self.MidiChannel(True, i, None))


    def configure_midi_channel(self, midi_channel, active, floppy_channel, port_str):
        self.midi_channels[midi_channel - 1].active = active
        self.midi_channels[midi_channel - 1].floppy_channel = floppy_channel
        self.midi_channels[midi_channel - 1].serial_port = port_str


    def connect_to_serial_ports(self):		
        self._used_serial_ports.clear()

        for i in range(0, self.MAX_CHANNELS):
            ser = self._create_serial_port(self.midi_channels[i].serial_port, self.BAUDRATE)
            self._used_serial_ports[self.midi_channels[i].serial_port] = ser
			
        for port in self._used_serial_ports.iterkeys():
            print "debug: trying to connect to: %s" % port
            self._used_serial_ports[port].open()

            
    def close_serial_ports(self):
        for port in used_serial_ports:
		    port.close()

        

    def _create_serial_port(self, port_str, baudrate):
        ser = serial.Serial()

        ser.port = port_str
        ser.baudrate = baudrate
        ser.timeout = 0
        ser.parity = serial.PARITY_NONE
        ser.stopbits = serial.STOPBITS_ONE
        ser.bytesize = serial.EIGHTBITS

        return ser

    
          
    def play_tone(self, midi_channel, frequency): 
        if midi_channel < 1 or midi_channel > self.MAX_CHANNELS:
            raise Exception("channel '%d' out of range. it has to be between 1 - %d" % (midi_channel, self.MAX_CHANNELS) )

        if frequency != 0:
            half_period = (1000000.0 / frequency) / (2.0 * self.ARDUINO_RESOLUTION) # period in microseconds
        else:
            half_period = 0

        # build 3 byte data packet for floppy
        # 1: physical_pin (see microcontroller code for further information)
        # 2: half_period
 
        physical_pin = (self.midi_channels[midi_channel - 1].floppy_channel - 1) * 2
        data = struct.pack('B', physical_pin) + struct.pack('>H', int(half_period))
        
        try:
            self._used_serial_ports[self.midi_channels[midi_channel - 1].serial_port].write(data)
        except:
            pass #print "serial port error"
        
    
    def play_tones(self, channel_frequency_list):
        if midi_channel < 1 or midi_channel > self.MAX_CHANNELS:
            raise Exception("channel '%d' out of range. it has to be between 1 - %d" % (midi_channel, self.MAX_CHANNELS) )
            
        if channel_frequency_list == []:
            return
         
        data = ""
            
        for tone in channel_frequency_list:
            channel = tone[0]
            frequency = tone[1]
            
            if tone.frequency != 0:
                half_period = (1000000.0 / frequency) / (2.0 * self.ARDUINO_RESOLUTION) # period in microseconds
            else:
                half_period = 0

            # build 3 byte data packet for floppy
            # 1: physical_pin (see microcontroller code for further information)
            # 2: half_period
     
            physical_pin = (self.midi_channels[tone.midi_channel - 1].floppy_channel - 1) * 2
            data += struct.pack('B', physical_pin) + struct.pack('>H', int(half_period))

        try:
            self._used_serial_ports[self.midi_channels[midi_channel - 1].serial_port].write(data)
        except:
            pass #print "serial port error"
    

    # this is the same as play_note with the exception, that you can give multiple tones
    # to it, so it will only call the serial.write() function once, since it is
    # a very expensive kernel call
    def play_notes(self, channel_note_list):
        #if midi_channel < 1 or midi_channel > self.MAX_CHANNELS:
        #    raise Exception("channel '%d' out of range. it has to be between 1 - %d" % (midi_channel, self.MAX_CHANNELS) )
        if not len(channel_note_list): # list is empty
            return
        
        data = ""
        
        for note in channel_note_list:
            midi_channel = note[0]
            midi_note = note[1]
            
            # build 3 byte data packet for floppy
            # 1: physical_pin (see microcontroller code for further information)
            # 2: half_period
     
            half_period = self.midiHalfperiods[midi_note]
            physical_pin = (self.midi_channels[midi_channel - 1].floppy_channel - 1) * 2
            data += struct.pack('B', physical_pin) + struct.pack('>H', int(half_period))
            
        try:
            # This is solution only works, when all drives are using the same serial port!
            # TODO: sort all events by serial ports and THEN do the sends
            self._used_serial_ports[self.midi_channels[midi_channel - 1].serial_port].write(data)
            #print "len: %d" % len(data)
        except:
            pass #print "serial port error"

    def play_note(self, midi_channel, midi_note):
        if midi_channel < 1 or midi_channel > self.MAX_CHANNELS:
            raise Exception("channel '%d' out of range. it has to be between 1 - %d" % (midi_channel, self.MAX_CHANNELS) )
            
        half_period = self.midiHalfperiods[midi_note]
        physical_pin = (self.midi_channels[midi_channel - 1].floppy_channel - 1) * 2
        data = struct.pack('B', physical_pin) + struct.pack('>H', int(half_period))
        
        try:
            self._used_serial_ports[self.midi_channels[midi_channel - 1].serial_port].write(data)
        except:
            pass #print "serial port error"
            
                   
    def stop_note(self, floppy_channel):
        self.play_tone(floppy_channel, 0)
        

    def reset_floppies(self):
        for port in self._used_serial_ports.iterkeys():
            ser = self._used_serial_ports[port]
            print "debug: resetting port: %s" % repr(port)
            ser.write( struct.pack('B', 100) )
 
        
        
def main():
    fl = FloppyOut()
    fl.map_serial_port_to_channel(1, "/dev/ttyUSB0")
    fl.map_serial_port_to_channel(2, "/dev/ttyUSB0")
    fl.connect_serial_ports()

    fl.play_tone(1, 0)
    fl.play_tone(2, 0)


if __name__ == "__main__":
    main()
