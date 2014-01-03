"""
 3D Starfield Simulation
 Developed by Leonel Machava <leonelmachava@gmail.com>
 
http://codeNtronix.com
 
 
http://twitter.com/codentronix
 
"""
import pygame, math, sys
from random import randrange
from operator import itemgetter
from BitmapFont.BitmapFont import BitmapFontScroller


class Point3D:
    def __init__(self, x = 0, y = 0, z = 0):
        self.x, self.y, self.z = float(x), float(y), float(z)
 
    def rotateX(self, angle):
        """ Rotates the point around the X axis by the given angle in degrees. """
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        y = self.y * cosa - self.z * sina
        z = self.y * sina + self.z * cosa
        return Point3D(self.x, y, z)
 
    def rotateY(self, angle):
        """ Rotates the point around the Y axis by the given angle in degrees. """
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        z = self.z * cosa - self.x * sina
        x = self.z * sina + self.x * cosa
        return Point3D(x, self.y, z)
 
    def rotateZ(self, angle):
        """ Rotates the point around the Z axis by the given angle in degrees. """
        rad = angle * math.pi / 180
        cosa = math.cos(rad)
        sina = math.sin(rad)
        x = self.x * cosa - self.y * sina
        y = self.x * sina + self.y * cosa
        return Point3D(x, y, self.z)
 
    def project(self, win_width, win_height, fov, viewer_distance):
        """ Transforms this 3D point to 2D using a perspective projection. """
        factor = fov / (viewer_distance + self.z)
        x = self.x * factor + win_width / 2
        y = -self.y * factor + win_height / 2
        return Point3D(x, y, self.z)



class Simulation:
    def __init__(self, num_stars, max_depth):
        pygame.init()
 
        self.screen = pygame.display.set_mode((640, 480))
        pygame.display.set_caption("3D Starfield Simulation (visit codeNtronix.com)")
 
        self.clock = pygame.time.Clock()
        self.num_stars = num_stars
        self.max_depth = max_depth
 
        self.init_stars()
        self.init_3dcube()

    def init_scroller(self):
        pass

    def move_and_draw_scroller(self):
        pass

 
    def init_stars(self):
        """ Create the starfield """
        self.stars = []
        for i in range(self.num_stars):
            # A star is represented as a list with this format: [X,Y,Z]
            star = [randrange(-25,25), randrange(-25,25), randrange(1, self.max_depth)]
            self.stars.append(star)
 
    def move_and_draw_stars(self):
        """ Move and draw the stars """
        origin_x = self.screen.get_width() / 2
        origin_y = self.screen.get_height() / 2
 
        for star in self.stars:
            # The Z component is decreased on each frame.
            star[2] -= 0.19
 
            # If the star has past the screen (I mean Z<=0) then we
            # reposition it far away from the screen (Z=max_depth)
            # with random X and Y coordinates.
            if star[2] <= 0:
                star[0] = randrange(-25,25)
                star[1] = randrange(-25,25)
                star[2] = self.max_depth
 
            # Convert the 3D coordinates to 2D using perspective projection.
            k = 128.0 / star[2]
            x = int(star[0] * k + origin_x)
            y = int(star[1] * k + origin_y)
 
            # Draw the star (if it is visible in the screen).
            # We calculate the size such that distant stars are smaller than
            # closer stars. Similarly, we make sure that distant stars are
            # darker than closer stars. This is done using Linear Interpolation.
            if 0 <= x < self.screen.get_width() and 0 <= y < self.screen.get_height():
                size = (1 - float(star[2]) / self.max_depth) * 5
                shade = (1 - float(star[2]) / self.max_depth) * 255
                self.screen.fill((shade,shade,shade),(x,y,size,size))


    # 3D cube
    def init_3dcube(self):
        self.vertices = [
            Point3D(-1,1,-1),
            Point3D(1,1,-1),
            Point3D(1,-1,-1),
            Point3D(-1,-1,-1),
            Point3D(-1,1,1),
            Point3D(1,1,1),
            Point3D(1,-1,1),
            Point3D(-1,-1,1)
        ]
 
        # Define the vertices that compose each of the 6 faces. These numbers are
        # indices to the vertices list defined above.
        self.faces  = [(0,1,2,3),(1,5,6,2),(5,4,7,6),(4,0,3,7),(0,4,5,1),(3,2,6,7)]
 
        # Define colors for each face
        self.colors = [(255,0,255),(255,0,0),(0,255,0),(0,0,255),(0,255,255),(255,255,0)]
 
        self.angle = 0
    
 
    def run(self):
        """ Main Loop """
        
        bmfs = BitmapFontScroller(self.screen, "fonts/1/bubsy.bmp", 0, 300)
        bmfs.set_text("LE.COON.PRAESENTIERT:.FLOPPY.ORGEL.V2.0.")

        while 1:
            # Lock the framerate at 50 FPS.
            self.clock.tick(50)

            # Handle events.
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    return
 
            self.screen.fill((0, 0, 0))
            self.move_and_draw_stars()
            #self.move_and_draw_3dcube()

            bmfs.tick()
            pygame.display.flip()

 

    def move_and_draw_3dcube(self):
    # It will hold transformed vertices.
        t = []
 
        cube_width   = 200
        cube_height = 200

        for v in self.vertices:
            # Rotate the point around X axis, then around Y axis, and finally around Z axis.
            r = v.rotateX(self.angle).rotateY(self.angle).rotateZ(self.angle)
            # Transform the point from 3D to 2D
            p = r.project(cube_width, cube_height, 128, 4)
            # Put the point in the list of transformed vertices
            t.append(p)
        # Calculate the average Z values of each face.
        avg_z = []
        i = 0
        for f in self.faces:
            z = (t[f[0]].z + t[f[1]].z + t[f[2]].z + t[f[3]].z) / 4.0
            avg_z.append([i,z])
            i = i + 1
 
        # Draw the faces using the Painter's algorithm:
        # Distant faces are drawn before the closer ones.
        for tmp in sorted(avg_z,key=itemgetter(1),reverse=True):
            face_index = tmp[0]
            f = self.faces[face_index]
            pointlist = [(t[f[0]].x, t[f[0]].y), (t[f[1]].x, t[f[1]].y),
                         (t[f[1]].x, t[f[1]].y), (t[f[2]].x, t[f[2]].y),
                         (t[f[2]].x, t[f[2]].y), (t[f[3]].x, t[f[3]].y),
                         (t[f[3]].x, t[f[3]].y), (t[f[0]].x, t[f[0]].y)]
            pygame.draw.polygon(self.screen,self.colors[face_index],pointlist)
 
        self.angle += 1
 
 
if __name__ == "__main__":
    Simulation(512, 32).run()




