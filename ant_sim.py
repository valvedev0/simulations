import pygame
import random

# ==========================================
# 1. INITIALIZATION & SETUP
# ==========================================
pygame.init()
pygame.font.init() # Initialize the font module for our UI

# Define Window Dimensions (Split into Simulation and UI)
SIM_WIDTH = 800
UI_WIDTH = 300
HEIGHT = 600
screen = pygame.display.set_mode((SIM_WIDTH + UI_WIDTH, HEIGHT))
pygame.display.set_caption("Advanced Ant Simulation")
clock = pygame.time.Clock()

# Set up a font for the display partition
ui_font = pygame.font.SysFont("Consolas", 14)
title_font = pygame.font.SysFont("Consolas", 18, bold=True)

# ==========================================
# 2. DEFINING THE ENVIRONMENT CLASSES
# ==========================================
class Target:
    def __init__(self, name, color):
        # Keep targets within the simulation area, away from edges
        self.pos = pygame.math.Vector2(random.randint(50, SIM_WIDTH - 50), random.randint(50, HEIGHT - 50))
        self.color = color
        self.name = name
        self.ants_arrived = 0

class Obstacle:
    def __init__(self):
        # Place obstacles randomly in the simulation area
        self.pos = pygame.math.Vector2(random.randint(100, SIM_WIDTH - 100), random.randint(100, HEIGHT - 100))
        self.radius = random.randint(20, 45) # Different sized obstacles

# ==========================================
# 3. DEFINING THE ANT CLASS
# ==========================================
class Ant:
    def __init__(self, target_obj):
        # Start at a random position on the left edge of the screen
        self.pos = pygame.math.Vector2(random.randint(10, 50), random.randint(10, HEIGHT - 10))
        self.velocity = pygame.math.Vector2(0, 0)
        self.speed = random.uniform(0.6, 0.8)
        
        # Assign the target and inherit its color
        self.target = target_obj
        self.color = target_obj.color
        
        # Path and Analytics Tracking
        self.path = [pygame.math.Vector2(self.pos)]
        self.distance_traveled = 0.0
        self.arrived = False

    def update(self, obstacles):
        if self.arrived:
            return # Stop processing if already at the target

        distance_to_target = self.pos.distance_to(self.target.pos)
        
        # Check for arrival
        if distance_to_target < 10:
            self.arrived = True
            self.target.ants_arrived += 1
            return 

        # --- 1. SEEK BEHAVIOR ---
        direction = self.target.pos - self.pos 
        if direction.length() > 0:
            direction = direction.normalize()

        # --- 2. WANDER BEHAVIOR ---
        wander = pygame.math.Vector2(random.uniform(-0.5, 0.5), random.uniform(-0.5, 0.5))

        # --- 3. OBSTACLE AVOIDANCE ---
        avoidance = pygame.math.Vector2(0, 0)
        for obs in obstacles:
            dist_to_obs = self.pos.distance_to(obs.pos)
            sight_radius = obs.radius + 30 # Ant can "see" 30 pixels ahead of the obstacle
            
            if dist_to_obs < sight_radius and dist_to_obs > 0:
                # Calculate vector pushing away from obstacle
                push_vector = self.pos - obs.pos
                push_vector = push_vector.normalize()
                
                # The closer the ant, the stronger the push (Inverse proportion)
                strength = (sight_radius - dist_to_obs) / sight_radius
                avoidance += push_vector * (strength * 3.0) # Multiply by 3.0 to make the push dominant

        # --- COMBINE & MOVE ---
        self.velocity = direction + wander + avoidance
        
        if self.velocity.length() > 0:
            self.velocity = self.velocity.normalize() * self.speed
            
        old_pos = pygame.math.Vector2(self.pos)
        self.pos += self.velocity

        # Calculate distance traveled this frame
        self.distance_traveled += self.pos.distance_to(old_pos)

        # Path recording
        if self.pos.distance_to(self.path[-1]) > 5:
            self.path.append(pygame.math.Vector2(self.pos))

    def draw(self, surface):
        if len(self.path) > 1:
            pygame.draw.lines(surface, self.color, False, self.path, 1)
        pygame.draw.circle(surface, self.color, (int(self.pos.x), int(self.pos.y)), 3)

# ==========================================
# 4. CREATING THE SIMULATION ENTITIES
# ==========================================
# Create Targets with specific colors
targets = [
    Target("Red Base", (255, 50, 50)),
    Target("Blue Base", (50, 150, 255)),
    Target("Green Base", (50, 255, 50))
]

# Create 5 random obstacles
obstacles = [Obstacle() for _ in range(5)]

# Create 30 ants, distributing them evenly among the targets
ants = []
for i in range(30):
    assigned_target = targets[i % len(targets)] # Loops through targets
    ants.append(Ant(assigned_target))

# ==========================================
# 5. THE MAIN APPLICATION LOOP
# ==========================================
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # A. Render Simulation Background
    screen.fill((30, 30, 30)) 
    
    # B. Render UI Partition Background (Right side)
    ui_rect = pygame.Rect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT)
    pygame.draw.rect(screen, (15, 15, 20), ui_rect)
    # Draw a dividing line
    pygame.draw.line(screen, (100, 100, 100), (SIM_WIDTH, 0), (SIM_WIDTH, HEIGHT), 2)

    # C. Draw Obstacles (Gray circles)
    for obs in obstacles:
        pygame.draw.circle(screen, (100, 100, 100), (int(obs.pos.x), int(obs.pos.y)), obs.radius)
        # Draw a faint outline showing the "avoidance zone"
        pygame.draw.circle(screen, (70, 70, 70), (int(obs.pos.x), int(obs.pos.y)), obs.radius + 30, 1)

    # D. Draw Targets
    for t in targets:
        pygame.draw.circle(screen, t.color, (int(t.pos.x), int(t.pos.y)), 12)
        pygame.draw.circle(screen, (255, 255, 255), (int(t.pos.x), int(t.pos.y)), 12, 2) # White border

    # E. Update State and Draw Ants
    for ant in ants:
        ant.update(obstacles)
        ant.draw(screen)

    # F. Render Analytics to the UI Partition
    y_offset = 20
    title_surface = title_font.render("SIMULATION RESULTS", True, (255, 255, 255))
    screen.blit(title_surface, (SIM_WIDTH + 20, y_offset))
    y_offset += 40

    for t in targets:
        # Calculate average distance for ants belonging to this target
        target_ants = [a for a in ants if a.target == t]
        avg_dist = sum(a.distance_traveled for a in target_ants) / len(target_ants) if target_ants else 0
        
        # Render Text
        text_name = ui_font.render(f"Target: {t.name}", True, t.color)
        text_arrived = ui_font.render(f"Arrived: {t.ants_arrived} / {len(target_ants)}", True, (200, 200, 200))
        text_dist = ui_font.render(f"Avg Path Length: {avg_dist:.0f} px", True, (200, 200, 200))
        
        screen.blit(text_name, (SIM_WIDTH + 20, y_offset))
        screen.blit(text_arrived, (SIM_WIDTH + 20, y_offset + 20))
        screen.blit(text_dist, (SIM_WIDTH + 20, y_offset + 40))
        y_offset += 80 # Add spacing for the next target group

    pygame.display.flip()
    clock.tick(60)

pygame.quit()