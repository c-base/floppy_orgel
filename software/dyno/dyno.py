#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pygame
import sys
import swmixer
import numpy


class Graph:
    def __init__(self):
        self.x_scale = 440
        self.y_scale = 100
        self.x_top_label = "Octave"
        self.x_bottom_label = "Frequency (Hz)"
        self.y_left_label = "Volume (%)"
        self.points = []

        self.x_pos = 75
        self.y_pos = 100
        self.x_size = 600 - 100
        self.y_size = 400 - 100

        self.x_divisors = 5
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
        x_div_pixels = self.x_size / self.x_divisors
        y_div_pixels = self.y_size / self.y_divisors

        for i in range(1, self.x_divisors):
            pygame.draw.line(surface, pygame.Color("white"),
                             (self.x_pos + i * x_div_pixels, self.y_pos),
                             (self.x_pos + i * x_div_pixels, self.y_pos + self.y_size)) # x-divisors

        for i in range(0, self.y_divisors):
            pygame.draw.line(surface, pygame.Color("white"),
                             (self.x_pos, self.y_pos + i * y_div_pixels),
                             (self.x_pos + self.x_size, self.y_pos + i * y_div_pixels)) # y-divisors


    def _draw_text(self, surface):
        x_div_pixels = self.x_size / self.x_divisors
        y_div_pixels = self.y_size / self.y_divisors

        for i in range(self.x_divisors + 1): # x-values - bottom (frequenies)
            x_text = self.font.render(str(i * (self.x_scale / self.x_divisors)), 0, pygame.Color("green"))
            surface.blit(x_text, (self.x_pos + i * x_div_pixels - x_text.get_width() / 2, self.y_pos + self.y_size + 12))

        for i in range(self.x_divisors): # x-values - top (octaves)
            x_text = self.font.render("%d" % i, 0, pygame.Color("yellow"))
            surface.blit(x_text, (self.x_pos + i * x_div_pixels + x_div_pixels / 2, self.y_pos - x_text.get_height()))

        # labels
        x_top_label = self.font.render(self.x_top_label, 0, pygame.Color("yellow"))
        x_bottom_label = self.font.render(self.x_bottom_label, 0, pygame.Color("green"))

        surface.blit(x_top_label,
                     (self.x_pos + (self.x_divisors * x_div_pixels) / 2 - x_top_label.get_width() / 2,
                      self.y_pos - 2 * x_top_label.get_height()))

        surface.blit(x_bottom_label,
                     (self.x_pos + (self.x_divisors * x_div_pixels) / 2 - x_bottom_label.get_width() / 2,
                      self.y_pos + self.y_size + 2 * x_bottom_label.get_height()))



        for i in range(self.y_divisors + 1): # y-values
            y_text = self.font.render(str(100 - (i * (self.y_scale / self.y_divisors))), 0, pygame.Color("green"))
            surface.blit(y_text, (self.x_pos - 12 - y_text.get_width(), self.y_pos + i * y_div_pixels - y_text.get_height() / 2))

        # Current frequency
        freq_text = self.font.render("Frequency: %d Hz (--)" % self.cur_frequency, 0, pygame.Color("red"))
        surface.blit(freq_text, (self.x_pos, self.y_pos - 75))

        # Current volume
        freq_text = self.font.render("Volume: %d%%" % self.cur_volume, 0, pygame.Color("red"))
        surface.blit(freq_text, (self.x_pos + self.x_size - freq_text.get_width(), self.y_pos - 75))


    def draw(self, surface):
        self._draw_grid(surface)
        self._draw_text(surface)


def main():
    pygame.init()
    pygame.display.set_caption("Floppy Drive Dyno Test")
    screen = pygame.display.set_mode((640, 480))
    clock = pygame.time.Clock()
    graph = Graph()

    swmixer.init(samplerate=44100, chunksize=1024, stereo=False, microphone=True)
    micdata = []
    micsnd = None
    vol = 0
    prev_vol = 0
    frame = 0

    while 1:
        # Lock the framerate at 50 FPS.
        clock.tick(50)
        swmixer.tick()
        micdata = swmixer.get_microphone()
        micsnd = swmixer.Sound(data=micdata)
        micsnd.play()

        vol = micsnd.data.max() #max 32767
        #if vol > 12000 and prev_vol < 4000 and prev_vol != 0:
        print "Volume level: %d%%" % (vol * 100 / 32767)


        # Handle events.
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return

        screen.fill((0, 0, 0))
        graph.draw(screen)
        pygame.display.flip()



if __name__ == "__main__":
    main()







