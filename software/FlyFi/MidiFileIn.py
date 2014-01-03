#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
FlyFi - Floppy-Fidelity
=======

Created to fulfill all your floppy music needs.

Created on Mon 11. May. 2013 03:43:42+0100
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


import pygame
from pygame.locals import *
from thread import start_new_thread
import time
import sys

import midi

# on windows use time.clock
gettimeus = time.clock

# on raspi use time.time
#gettimeus = time.time

class MidiFileIn(object):
    def __init__(self, midi_event_list_callback):
        self.midi_file = None
        self.midi_file_event_num = 0
        self.midi_file_current_pos = 0
        self.cur_event_item = None
        
        self.seconds_per_tick = None
        self.midi_event_list_callback = midi_event_list_callback
        
        # timing
        self.time_start = 0
        self.time_to_wait = 0
        
        self.midi_events = [] # the whole midi file will be converted in this list
        
            
    # (depricated) dirty sleep solution which results in a high cpu load. (but is very accurate)            
    def _dirty_sleep(self, seconds): # use time.clock on windows and time.clock() on linux
        start = gettimeus() * 1000 * 1000
        
        micros = 0
        while True:
            micros = gettimeus() * 1000 * 1000
            if (micros - start) >= seconds * 1000 * 1000:
                break


    # This function will play the midi-file.
    # It is implemented asynchonously and has to be called as often as possible for proper playback.
    
    def _tick_play(self):
        # since the python-midi library seems not to be consistent (sometimes the channels of the events are missing),
        # the callback will only be called on Note On, Note Off (and later pitch bend) events.
        elapsed_time = gettimeus() * 1000 * 1000 - self.time_start
        
        # delay until executing next midi event...
        if elapsed_time >= self.time_to_wait:
            cur_event_list = self.cur_event_item[1]
            
            # if the elapsed time is 10% greater than the maximum time to wait, give out a warning
            delay = elapsed_time - self.time_to_wait
            if self.time_to_wait != 0 and elapsed_time > self.time_to_wait * 1.10:
                print "Performance problem. time to wait: %dus, elapsed time: %dus, delay: %.2f%%" % (self.time_to_wait, elapsed_time, elapsed_time * 100.0 / self.time_to_wait - 100)

            # calculate when it's time to play the next tones
            self.time_start = gettimeus() * 1000 * 1000
            ticks_to_wait = self.cur_event_item[0]
            event_list = self.cur_event_item[1]
            self.time_to_wait = ticks_to_wait * self.seconds_per_tick * 1000 * 1000
        
            self.midi_event_list_callback(event_list) # play the current tones
            self.midi_file_current_pos += 1
            
            if self.midi_file_current_pos != self.midi_file_event_num:
                self.cur_event_item = self.midi_events[self.midi_file_current_pos]
            else:
                self.tick = self._tick_nonplay
                print "[MidiFileIn.py] debug: finished playing!"
        
    def _tick_nonplay(self):
        pass
    
    def tick(self):        
        pass
                
            
    # depricated
    def _worker_thread(self):
        print "[MidiFileIn.py] debug: start playing..."
                   
        # This is the mainloop for playing the midi-file.
        # Previously this dirty sleep function has been layed out to a function
        # and was called at the end of the loop.
        # This resulted in slowdowns of the song on very complex midi files like the imperial march, where 
        # the function already needs much of the midi-tick time while generating the midi-event lists.
        #
        # In this new way, only the rest of the time (or no if late) has to be wait at the end of the loop, 
        # so slowdowns of a song will be minimized.
        
        for event_item in self.midi_events:
            ticks_to_wait = event_item[0]
            event_list = event_item[1]
            
            
            # since the python-midi library seems not to be consistent (sometimes the channels of the events are missing),
            # the callback will only be called on Note On, Note Off (and later pitch bend) events.
            start = gettimeus() * 1000 * 1000
            self.midi_event_list_callback(event_list)
            elapsed_time = gettimeus() * 1000 * 1000 - start        
            time_to_wait = ticks_to_wait * self.seconds_per_tick * 1000 * 1000
            
            # delay until executing next midi event...
            while elapsed_time < time_to_wait:
                elapsed_time = gettimeus() * 1000 * 1000 - start 
            delay = elapsed_time - time_to_wait
           
            # if the elapsed time is 10% greater than the maximum time to wait, give out a warning
            if time_to_wait != 0 and elapsed_time > time_to_wait * 1.10:
                print "Performance problem. time to wait: %dus, elapsed time: %dus, delay: %.2f%%" % (time_to_wait, elapsed_time, elapsed_time * 100.0 / time_to_wait - 100)
            
        print "[MidiFileIn.py] debug: finished playing!"
        
    def set_bpm(self, bpm):
        ticks_per_beat = self.midi_file.resolution
        beats_per_minute = bpm 
        self.seconds_per_tick = 1.0/((ticks_per_beat * beats_per_minute) / 60.0)
        
    def stop_midi_polling(self):
        pass

    def update_progress(self, progress):
        sys.stdout.write('\r[{0}] {1}%'.format('#'*(progress/10) + ' '*(10 - progress/10), progress))
        sys.stdout.flush()
        
    def open_midi_file(self, path_to_file):
        print "[MidiFileIn.py] loading midi file: %s..." % path_to_file
        self.midi_file = midi.read_midifile(path_to_file)
        self.midi_events = []
        self.set_bpm(120) #default value which will be overwritten on play (hopefully)
        
        # Converting midi tracks to one list
        # reverse event list, so .pop() can be used
        num_events = 0
        cur_event = 0
        
        for track in self.midi_file:
            num_events += len(track)
            track.reverse()
        
        end_of_track = False
        while not end_of_track:
            end_of_track = True
            ticks_to_wait = None
                
            # get lowest tick of all tracks to calulate the ticks to wait
            # before the next midi tick will be executed
            for track in self.midi_file:
                if len(track) and (ticks_to_wait == None or ticks_to_wait > track[-1].tick):
                    ticks_to_wait = track[-1].tick
                
            event_list = []
            for track_index, track in enumerate(self.midi_file):
                while len(track) and track[-1].tick == 0:
                    event_list.append(track.pop())
                    cur_event += 1
                
                if len(track):
                    track[-1].tick -= ticks_to_wait
                    end_of_track = False
                        
            self.midi_events.append([ticks_to_wait, event_list])
   
            if cur_event % 250 == 0:
                self.update_progress( int((cur_event * 100.0 / num_events)))
            
        self.update_progress(100)

        self.midi_file_event_num = len(self.midi_events)
        self.midi_file_current_pos = 0
        self.cur_event_item = self.midi_events[self.midi_file_current_pos]
        
        sys.stdout.write('\n')
        print "[MidiFileIn.py] midi file loaded and ready to play."
        
        
        
    def get_num_tracks(self):
        return len(self.midi_file) - 1
    
    # starts reading the events of the midi file and
    # calls the specified callback with correct timings
    def play(self): 
        self.tick = self._tick_play
        print "[MidiFileIn.py] debug: start playing..."
       
    def play_nogui(self):
        self._worker_thread()

    def stop(self):
        self.tick = self._tick_nonplay
    
        
                    

def main():    

    def cb_midi_event(status, data1, data2, tick):
        # parsing the events
        # ==================
        # only note on, note off and pitch wheel range are
        # important for us, so the other midi events are just ignored.
        event_str = None
        
        channel = None
        
        if status >= 0x80 and status <= 0x8F: # Note Off
            channel = status - 0x80 + 1
            midi_note = data1;
            velocity = data2
            
            event_str = "Chan %s Note off" % channel
            
            self.fout.stop_note(channel)
            mout.note_off(midi_note, velocity, channel - 1) # only for debugging. remove later!!!
        elif status >= 0x90 and status <= 0x9F: # Note On
            channel = status - 0x90 + 1
            midi_note = data1;
            velocity = data2
            
            event_str = "Chan %s Note on" % channel  

            if velocity > 0:
                mout.note_on(midi_note, velocity, channel - 1) # only for debugging. remove later!!!
                self.fout.play_note(channel, midi_note)      
            else:
                mout.note_on(midi_note, velocity, channel - 1) # only for debugging. remove later!!!
                self.fout.stop_note(channel) # a volume of 0 is the same as note off
             
             ##
             
        elif status >= 0xA0 and status <= 0xAF: # Polyphonic Aftertouch (ignore)
            return
        elif status >= 0xB0 and status <= 0xBF: # Chan Control mode change (ignore)
            return
        elif status >= 0xC0 and status <= 0xCF: # Chan Program change (ignore)
            mout.set_instrument(event.data, event.channel)
        elif status >= 0xD0 and status <= 0xDF: # Channel Aftertouch (ignore)
            return
        elif status >= 0xE0 and status <= 0xEF: # pitch bend (TODO: don't ignore!)
            channel = status - 0xE0 + 1
            pitch_value = 128 * velocity
            event_str = "Chan %s pitch bend with value %s and" % (channel, pitch_value)     
        else:
            event_str = "unknown event (0x%0X)" % (status)
            print "%s with note %s and velocity %s @ %s" % (event_str, midi_note, velocity, tick)
            return
            
        if event_str != None:    
            pass
            #print "%s with note %s and velocity %s @ %s" % (event_str, midi_note, velocity, tick)


    mfin = MidiFileIn(cb_midi_event, mout)
    mfin.open_midi_file("fish2.mid")
    mfin.play()
    
    
   


if __name__ == "__main__":
    main()
