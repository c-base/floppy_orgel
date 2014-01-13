#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pygame
import sys
import swmixer
import numpy
import math


class Graph:
    def __init__(self, frequency_table, y_min, y_max, values_per_x_divisor):
        self.frequency_table = frequency_table
        self.x_min = frequency_table[0]
        self.x_max = frequency_table[-1]
        self.y_min = y_min
        self.y_max = y_max
        self.x_top_label = "Octave"
        self.x_bottom_label = "Frequency (Hz)"
        self.y_left_label = "Volume (%)"
        self.points = []

        self.x_pos = 75
        self.y_pos = 100
        self.x_size = 600 - 100
        self.y_size = 400 - 100

        #self.x_divisors = 5
        self.vals_per_x_div = values_per_x_divisor

        self.y_divisors = 10
        self.font = self._load_font("./fonts/lcd.ttf", 18)

        self.cur_frequency = 0
        self.cur_volume = 0

    def _load_font(self, font_path, font_size = 32):
        pygame.font.init()
        return pygame.font.Font(font_path, font_size)

    def _draw_grid(self, surface):
        pygame.draw.line(surface, pygame.Color("white"), (self.x_pos, self.y_pos + self.y_size), (self.x_pos + self.x_size, self.y_pos + self.y_size)) # Bottom X-Axis
        pygame.draw.line(surface, pygame.Color("white"), (self.x_pos, self.y_pos), (self.x_pos, self.y_pos + self.y_size)) # Left Y-Axis
        pygame.draw.line(surface, pygame.Color("white"), (self.x_pos + self.x_size, self.y_pos), (self.x_pos + self.x_size, self.y_pos + self.y_size)) # Right Y-Axis

        # divisors
        x_divisors = len(self.frequency_table) / self.vals_per_x_div
        y_div_pixels = self.y_size / self.y_divisors
        x_div_pixels = self.x_size / x_divisors

        for i in range(1, x_divisors):
            pygame.draw.line(surface, pygame.Color("white"),
                             (self.x_pos + i * x_div_pixels, self.y_pos),
                             (self.x_pos + i * x_div_pixels, self.y_pos + self.y_size)) # x-divisors

        for i in range(0, self.y_divisors):
            pygame.draw.line(surface, pygame.Color("white"),
                             (self.x_pos, self.y_pos + i * y_div_pixels),
                             (self.x_pos + self.x_size, self.y_pos + i * y_div_pixels)) # y-divisors


    def _draw_text(self, surface):
        x_divisors = len(self.frequency_table) / self.vals_per_x_div
        y_div_pixels = self.y_size / self.y_divisors

        for i in range(x_divisors + 1): # x-values - bottom (frequenies)
            x_div_pixels = self.x_size / x_divisors
            #x_text = self.font.render(str(int(i * (self.x_max / x_divisors))), 0, pygame.Color("green"))
            x_text = self.font.render("%d" % round(self.frequency_table[i * self.vals_per_x_div]), 0, pygame.Color("green"))
            surface.blit(x_text, (self.x_pos + i * x_div_pixels - x_text.get_width() / 2, self.y_pos + self.y_size + 12))

        for i in range(x_divisors): # x-values - top (octaves)
            x_div_pixels = self.x_size / x_divisors
            x_text = self.font.render("%d" % i, 0, pygame.Color("yellow"))
            surface.blit(x_text, (self.x_pos + i * x_div_pixels + x_div_pixels / 2, self.y_pos - x_text.get_height()))

        # labels
        x_top_label = self.font.render(self.x_top_label, 0, pygame.Color("yellow"))
        x_bottom_label = self.font.render(self.x_bottom_label, 0, pygame.Color("green"))

        surface.blit(x_top_label,
                     (self.x_pos + (x_divisors * x_div_pixels) / 2 - x_top_label.get_width() / 2,
                      self.y_pos - 2 * x_top_label.get_height()))

        surface.blit(x_bottom_label,
                     (self.x_pos + (x_divisors * x_div_pixels) / 2 - x_bottom_label.get_width() / 2,
                      self.y_pos + self.y_size + 2 * x_bottom_label.get_height()))



        for i in range(self.y_divisors + 1): # y-values
            y_text = self.font.render(str(100 - (i * (self.y_max / self.y_divisors))), 0, pygame.Color("green"))
            surface.blit(y_text, (self.x_pos - 12 - y_text.get_width(), self.y_pos + i * y_div_pixels - y_text.get_height() / 2))


    def _frequency_to_note(self, frequency):
        return math.log(math.pow(frequency, 12.0) / math.pow(self.frequency_table[0], 12.0), 2.0)

    def _transform_coords(self, x, y):
        cur_note = self._frequency_to_note(x)
        max_note = self._frequency_to_note(self.frequency_table[-1])
        percent = cur_note / max_note

        abs_x_pos = self.x_pos + self.x_size * percent
        abs_y_pos = self.y_pos + self.y_size - y * self.y_size / self.y_max

        return (abs_x_pos, abs_y_pos)


    def _draw_data(self, surface):
        prev_abs_x_pos = None
        prev_abs_y_pos = None

        for i in range(len(self.points)):
            # transform coordinates for graph
            abs_x_pos, abs_y_pos = self._transform_coords(self.points[i][0], self.points[i][1])

            if prev_abs_x_pos is not None and prev_abs_y_pos is not None:
                pygame.draw.line(surface, pygame.Color("yellow"),
                                 (prev_abs_x_pos, prev_abs_y_pos),
                                 (abs_x_pos, abs_y_pos))
            prev_abs_x_pos = abs_x_pos
            prev_abs_y_pos = abs_y_pos

    def add_point(self, x, y):
        self.points.append((x, y))

    def draw(self, surface):
        self._draw_grid(surface)
        self._draw_text(surface)
        self._draw_data(surface)


def main():

    def get_note_name_from_frequency(frequency):
        def _frequency_to_note(freq):
            return round(math.log(math.pow(freq, 12.0) / math.pow(2 * 8.17575, 12.0), 2.0))

        note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
        note = _frequency_to_note(frequency)
        octave = note / 12
        tone = int(note % 12)

        return "%s%d" % (note_names[tone], octave)


    pygame.init()
    pygame.display.set_caption("Floppy Drive Dyno Test")
    screen = pygame.display.set_mode((640, 480))
    clock = pygame.time.Clock()
    swmixer.init(samplerate=44100, chunksize=1024, stereo=False, microphone=True)

    frequencies = [8.17575 * math.pow(2, note / 12.0) for note in range(12, 5 * 12 + 12 + 1)]
    graph = Graph(frequencies, 0, 100, 12)

    f_i = 0
    fps = 100
    fps_i = 0

    font = pygame.font.Font("./fonts/lcd.ttf", 18)

    while 1:
        clock.tick(fps) # Lock the framerate
        swmixer.tick()
        micdata = swmixer.get_microphone()
        micsnd = swmixer.Sound(data = micdata)
        vol = micsnd.data.max() # max 32767
        vol_percent = (vol * 100 / 32767)

        if fps_i == fps / 4 and f_i < len(frequencies):
            if f_i < len(frequencies):
                graph.add_point(frequencies[f_i], vol_percent)

                screen.fill((0, 0, 0))
                # Current frequency
                freq_text = font.render("Frequency: %.2f Hz (%s)" %
                                        (frequencies[f_i],
                                         get_note_name_from_frequency(frequencies[f_i])), 0, pygame.Color("red"))
                screen.blit(freq_text, (75, 25))

                # Current volume
                freq_text = font.render("Volume: %d%%" % vol_percent, 0, pygame.Color("red"))
                screen.blit(freq_text, (screen.get_width() - freq_text.get_width() - 50, 25))
            fps_i = 0
            f_i += 1

        fps_i += 1

        # Handle events.
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return






        graph.draw(screen)
        pygame.display.flip()



if __name__ == "__main__":
    main()







