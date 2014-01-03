#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
FlyFi - Floppy-Fidelity
=======

Created to fulfill all your floppy music needs.

Created on Mon 06. May. 2013 18:16:42+0100
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
import pygame.midi
from pygame.locals import *
from thread import start_new_thread


class MidiIn(object):
    def __init__(self, midi_event_callback):
        pygame.init()

        pygame.fastevent.init()
        self.event_get = pygame.fastevent.get
        self.event_post = pygame.fastevent.post
        self.midi_event_callback = midi_event_callback
        
        
        pygame.midi.init()
        input_id = pygame.midi.get_default_input_id()
        
        self.i = None
        if input_id != -1:
            self.i = pygame.midi.Input( input_id )
        else:
            print "no midi input found. only file mode available!"
    
    def __del__(self):
        self.i.close()
        pygame.midi.quit()
        pygame.quit()
    
    def _worker_thread(self):
        while(True):
            self._poll_event()
        
    def start_midi_polling(self):
        if self.i != None:
            start_new_thread(self._worker_thread, ())
        
    def stop_midi_polling(self):
        pass
        
    def _poll_event(self):
        if self.i.poll(): # are there MIDI data?
            # read exactly one midi event from the queue. It is possible 
            # to read more midi events at once but it is alot easier to 
            # handle them when reading them one by one.
            midi_events = self.i.read(25) # read maximum 
                        
            # parse midi events
            for event in midi_events:            
                status = event[0][0]
                note = event[0][1]
                velocity = event[0][2] # velocity (volume) is not possible on one floppy. anyway this value is important for pitch bend
                midi_timestamp = event[1]    
                    
                self.midi_event_callback(status, note, velocity, midi_timestamp)
                    

def main():    
    m = MidiIn()
    going = True
    
    while going:
        events = m.event_get()
        for e in events:
            if e.type in [QUIT]:
                going = False
            if e.type in [KEYDOWN]:
                going = False
                
        m.poll_event()
   


if __name__ == "__main__":
    main()
