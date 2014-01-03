# -*- coding: utf-8 -*-
"""
FlyFi - Floppy-Fidelity

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

    FlyFi is using tango icons: <http://tango.freedesktop.org/>.
"""

from MidiFileIn import MidiFileIn
from FloppyOut import FloppyOut
import pygame.midi
import sys
import signal
import profile

class FlyFiNoGui(object):
    def signal_handler(self, signal, frame):
        print "stopping"
        self.midi_fin.stop()
        print "resetting floppies"
        self.fout.reset_floppies()
        print "exitting..."
        sys.exit()

    def __init__(self):
        self.running = True
        self.fout = FloppyOut()    
        self.midi_fin = MidiFileIn(self.cb_midi_event_list)

        signal.signal(signal.SIGINT, self.signal_handler)

        #debug
        pygame.midi.init() #debug
        self.mout = pygame.midi.Output(pygame.midi.get_default_output_id())
    
    def cb_midi_event_list(self, event_list):
        # parsing the events
        # ==================
        # only note on, note off and pitch wheel range are
        # important for us, so the other midi events are just ignored.
        
        # status, data1, data2, tick
        
        event_str = None
        channel = None

        
        for event in event_list:
            status = event.statusmsg
            tick = event.tick
            
            # also note off events will be stored here. They will just get a frequency of 0.
            note_on_list = [] # [ [channel, tone], [channel, tone] ... ]
        
            if event.name == "Note Off":
                status += event.channel # due to buggy python-midi lib
            
                channel = status - 0x80 + 1
                midi_note = event.data[0]
                velocity = event.data[1]
                
                event_str = "Chan %s Note off" % channel
                
                note_on_list.append([channel, 0])
                self.mout.note_off(midi_note, velocity, channel - 1) # only for debugging. remove later!!!
                
            elif event.name == "Note On":
                status += event.channel # due to buggy python-midi lib
                
                channel = status - 0x90 + 1
                midi_note = event.data[0]
                velocity = event.data[1]
                
                event_str = "Chan %s Note on" % channel

                if velocity > 0:
                    note_on_list.append([channel, midi_note])
                    self.mout.note_on(midi_note, velocity, channel - 1) # only for debugging. remove later!!!      
                else:
                    note_on_list.append([channel, 0]) # note off
                    self.fout.stop_note(channel) # a volume of 0 is the same as note off
                 
            elif event.name == "Set Tempo":
                self.midi_fin.set_bpm(event.bpm)
              
            elif event.name == "Program Change": # Chan Program change (change instrument)
                if self.mout is not None:
                    self.mout.set_instrument(event.data[0], event.channel)

#            elif status >= 0xE0 and status <= 0xEF: # pitch bend (TODO: don't ignore!)
#                channel = status - 0xE0 + 1
#                velocity = event.data[1]
#                pitch_value = 128 * velocity
#                event_str = "Chan %s pitch bend with value %s and" % (channel, pitch_value)     
            else:
                event_str = "unknown event (0x%0X)" % (status)
                
            if event_str != None:    
                pass
                #print "%s with note %s and velocity %s @ %s" % (event_str, midi_note, velocity, tick)
        
                self.fout.play_notes(note_on_list)
            
        
    #obsolete            
    def cb_midi_event(self, status, data1, data2, tick):
        # parsing the events
        # ==================
        # only note on, note off and pitch wheel range are
        # important for us, so the other midi events are just ignored.
        event_str = None
        channel = None
    
        if status >= 0x80 and status <= 0x8F: # Note Off
            channel = status - 0x80 + 1
            midi_note = data1
            velocity = data2
        
            event_str = "Chan %s Note off" % channel
        
            self.fout.stop_note(channel)
        elif status >= 0x90 and status <= 0x9F: # Note On
            channel = status - 0x90 + 1
            midi_note = data1
            velocity = data2
        
            event_str = "Chan %s Note on" % channel

            if velocity > 0:
                self.fout.play_note(channel, midi_note)      
            else:
                self.fout.stop_note(channel) # a volume of 0 is the same as note off
         
        elif status >= 0xA0 and status <= 0xAF: # Polyphonic Aftertouch (ignore)
            return
        elif status >= 0xB0 and status <= 0xBF: # Chan Control mode change (ignore)
            return
        elif status >= 0xC0 and status <= 0xCF: # Chan Program change (ignore)
            return
        elif status >= 0xD0 and status <= 0xDF: # Channel Aftertouch (ignore)
            return
        elif status >= 0xE0 and status <= 0xEF: # pitch bend (TODO: don't ignore!)
            channel = status - 0xE0 + 1
            velocity = data2
            pitch_value = 128 * velocity
            event_str = "Chan %s pitch bend with value %s and" % (channel, pitch_value)     
        else:
            event_str = "unknown event (0x%0X)" % (status)
            print "%s with note %s and velocity %s @ %s" % (event_str, midi_note, velocity, tick)
            return
        
        if event_str != None:    
            pass
            #print "%s with note %s and velocity %s @ %s" % (event_str, midi_note, velocity, tick)
 
    def connect_to_serial_ports(self, serial_port):
        for i in range(0, 16):
            active = True
            floppy_channel = i + 1
            port_str = serial_port
            self.fout.configure_midi_channel(i, active, floppy_channel, port_str)

        self.fout.connect_to_serial_ports()

    def playMidiFile(self):
        self.midi_fin.play_nogui()
 
   
            
def main():
    flyfi = FlyFiNoGui()
            
    serial_port = sys.argv[1]
    midi_file = sys.argv[2]

    flyfi.connect_to_serial_ports(serial_port)     
    flyfi.midi_fin.open_midi_file(midi_file)
    
    #profile.runctx('flyfi.playMidiFile()', globals(), locals())
    flyfi.playMidiFile()

if __name__ == "__main__":
    main()
