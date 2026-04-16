import pygame
import math
import random

# ==========================================
# 1. INITIALIZATION & SETUP
# ==========================================
pygame.init()
pygame.font.init()

SIM_WIDTH = 800
UI_WIDTH = 300
HEIGHT = 600
screen = pygame.display.set_mode((SIM_WIDTH + UI_WIDTH, HEIGHT))
pygame.display.set_caption("Advanced Optics: Laser Reflection")
clock = pygame.time.Clock()

ui_font = pygame.font.SysFont("Consolas", 14)
title_font = pygame.font.SysFont("Consolas", 18, bold=True)

# ==========================================
# 2. DEFINING THE ENVIRONMENT CLASSES
# ==========================================
class Mirror:
    def __init__(self, p1, p2):
        self.p1 = pygame.math.Vector2(p1)
        self.p2 = pygame.math.Vector2(p2)
        
        # Calculate the line segment vector
        self.line_vec = self.p2 - self.p1
        
        # Calculate the Normal Vector (perpendicular to the surface)
        # By swapping x and y and negating one, we get a 90 degree rotation
        self.normal = pygame.math.Vector2(-self.line_vec.y, self.line_vec.x)
        if self.normal.length() > 0:
            self.normal = self.normal.normalize()

    def draw(self, surface):
        pygame.draw.line(surface, (200, 255, 255), self.p1, self.p2, 4)

# ==========================================
# 3. INTERSECTION & REFLECTION LOGIC
# ==========================================
def get_intersection(ray_origin, ray_dir, mirror):
    """
    Calculates the exact intersection point between a ray and a line segment.
    Uses parametric line equations.
    """
    x1, y1 = ray_origin.x, ray_origin.y
    x2, y2 = ray_origin.x + ray_dir.x, ray_origin.y + ray_dir.y
    x3, y3 = mirror.p1.x, mirror.p1.y
    x4, y4 = mirror.p2.x, mirror.p2.y

    den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
    if den == 0:
        return None # Lines are parallel

    t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den
    u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / den

    # t > 0 means the intersection is exactly in front of the ray (not behind it)
    # 0 <= u <= 1 means the intersection falls strictly within the mirror's segment
    if t > 0 and 0 <= u <= 1:
        pt_x = x1 + t * (x2 - x1)
        pt_y = y1 + t * (y2 - y1)
        return pygame.math.Vector2(pt_x, pt_y)
    return None

def calculate_reflection(incoming_vec, normal_vec):
    """ Applies the reflection formula: R = V - 2(V dot N)N """
    dot_product = incoming_vec.dot(normal_vec)
    reflection = incoming_vec - 2 * dot_product * normal_vec
    return reflection.normalize()

# ==========================================
# 4. CREATING THE SCENE
# ==========================================
# Create a bounding box of mirrors so the laser stays on screen
mirrors = [
    Mirror((10, 10), (SIM_WIDTH - 10, 10)),               # Top
    Mirror((SIM_WIDTH - 10, 10), (SIM_WIDTH - 10, HEIGHT - 10)), # Right
    Mirror((SIM_WIDTH - 10, HEIGHT - 10), (10, HEIGHT - 10)),    # Bottom
    Mirror((10, HEIGHT - 10), (10, 10))                   # Left
]

# Add some random internal mirrors
for _ in range(6):
    x, y = random.randint(50, SIM_WIDTH-150), random.randint(50, HEIGHT-150)
    angle = random.uniform(0, math.pi * 2)
    length = random.randint(80, 150)
    p2_x = x + math.cos(angle) * length
    p2_y = y + math.sin(angle) * length
    mirrors.append(Mirror((x, y), (p2_x, p2_y)))

laser_origin = pygame.math.Vector2(SIM_WIDTH // 2, HEIGHT // 2)
MAX_BOUNCES = 50 # Prevent infinite loops if the laser gets trapped

# ==========================================
# 5. THE MAIN APPLICATION LOOP
# ==========================================
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # Get mouse position to steer the laser
    mouse_x, mouse_y = pygame.mouse.get_pos()
    mouse_pos = pygame.math.Vector2(mouse_x, mouse_y)
    
    # Calculate initial laser direction toward the mouse
    laser_dir = mouse_pos - laser_origin
    if laser_dir.length() > 0:
        laser_dir = laser_dir.normalize()
    else:
        laser_dir = pygame.math.Vector2(1, 0)

    # --- SIMULATION RENDER ---
    screen.fill((10, 10, 15)) 
    
    for m in mirrors:
        m.draw(screen)

    # Draw Laser Origin Emitter
    pygame.draw.circle(screen, (255, 50, 50), (int(laser_origin.x), int(laser_origin.y)), 8)

    # --- RAYCASTING LOOP ---
    current_origin = pygame.math.Vector2(laser_origin)
    current_dir = pygame.math.Vector2(laser_dir)
    bounces = 0
    total_distance = 0

    while bounces < MAX_BOUNCES:
        closest_pt = None
        closest_dist = float('inf')
        hit_mirror = None

        # Check all mirrors to find the closest intersection
        for mirror in mirrors:
            pt = get_intersection(current_origin, current_dir, mirror)
            if pt:
                # Add a tiny offset check to prevent self-intersecting with the mirror we just bounced off
                dist = current_origin.distance_to(pt)
                if dist > 0.1 and dist < closest_dist:
                    closest_dist = dist
                    closest_pt = pt
                    hit_mirror = mirror

        if closest_pt:
            # Draw the laser beam to the intersection point
            pygame.draw.line(screen, (255, 50, 50), current_origin, closest_pt, 2)
            pygame.draw.circle(screen, (255, 255, 255), (int(closest_pt.x), int(closest_pt.y)), 3) # Impact flash
            
            total_distance += closest_dist
            
            # Update origin and direction for the next bounce
            current_origin = closest_pt
            current_dir = calculate_reflection(current_dir, hit_mirror.normal)
            bounces += 1
        else:
            # If it hits nothing, draw a line off to infinity (or screen edge)
            end_pt = current_origin + (current_dir * 2000)
            pygame.draw.line(screen, (255, 50, 50), current_origin, end_pt, 2)
            break # Exit the while loop, ray goes off into the void

    # --- UI PARTITION RENDER ---
    ui_rect = pygame.Rect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT)
    pygame.draw.rect(screen, (20, 20, 25), ui_rect)
    pygame.draw.line(screen, (100, 100, 100), (SIM_WIDTH, 0), (SIM_WIDTH, HEIGHT), 2)

    y_offset = 20
    title_surface = title_font.render("OPTICS ANALYTICS", True, (255, 255, 255))
    screen.blit(title_surface, (SIM_WIDTH + 20, y_offset))
    y_offset += 40

    stats = [
        f"Active Mirrors: {len(mirrors)}",
        f"Laser Bounces:  {bounces} / {MAX_BOUNCES}",
        f"Total Distance: {total_distance:.0f} px"
    ]

    for stat in stats:
        text_surface = ui_font.render(stat, True, (200, 200, 200))
        screen.blit(text_surface, (SIM_WIDTH + 20, y_offset))
        y_offset += 30

    pygame.display.flip()
    clock.tick(60)

pygame.quit()