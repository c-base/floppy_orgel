"""
 3D Starfield Simulation
 Developed by Leonel Machava <leonelmachava@gmail.com>
 
http://codeNtronix.com
 
 
http://twitter.com/codentronix 
"""

import pygame, math
from random import randrange

class Point:
    def __init__(self, x,y):
        self.x = x
        self.y = y

class Simulation:
    def __init__(self, num_stars, max_depth):
        pygame.init()
 
        self.screen = pygame.display.set_mode((640, 480))
        pygame.display.set_caption("Ein beweglicher Punkt")
 
        self.clock = pygame.time.Clock()
        
        self.p_current = Point(120, 160)
        self.size  = 5
        self.vel_x = 0 # pixel pro sekunde
        self.vel_y = 0 # pixel pro sekunde

    def move_to_target(self, p_current, p_target, velocity):
        dx = p_target.x - p_current.x
        dy = p_target.y - p_current.y

        p_current.x = p_current.x + dx * velocity / math.sqrt(math.pow(dx, 2) + math.pow(dy, 2))
        p_current.y = p_current.y + dy * velocity / math.sqrt(math.pow(dx, 2) + math.pow(dy, 2))

       # 4 - Player
       "1; rot; rot; blau; blau;" # teams initialisieren
       "2; 10; 10; ..." # aktuelle koordinaten von den Spielern
       "3; 200; 122" # schießen

       ""

    #  G   A   G
    #  H   H   A
    # sin cos tan

        #winkel = tan( (self.tar_x - self.x) / (self.tar_y - self.y) )
        #vel_x = 


    def run(self):
        """ Main Loop """
        while 1:
            # Lock the framerate at 50 FPS.
            self.clock.tick(1)
 
            # Handle events.
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    return

            p_target = Point(150, 180)
                         
            self.screen.fill((0,0,0))
            self.screen.fill((255, 255, 255), (self.p_current.x, self.p_current.y, self.size, self.size))
            pygame.display.flip()
            self.move_to_target(self.p_current, p_target, 10)

 
if __name__ == "__main__":
    Simulation(512, 32).run()
