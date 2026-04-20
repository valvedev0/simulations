import math
import random

import pygame

# ==========================================
# 1. INITIALIZATION & SETUP
# ==========================================
pygame.init()
pygame.font.init()

SIM_WIDTH = 800
UI_WIDTH = 320
HEIGHT = 650
screen = pygame.display.set_mode((SIM_WIDTH + UI_WIDTH, HEIGHT))
pygame.display.set_caption("Advanced Optics: Interactive Laser Lab")
clock = pygame.time.Clock()

ui_font = pygame.font.SysFont("Consolas", 14)
ui_bold = pygame.font.SysFont("Consolas", 14, bold=True)
title_font = pygame.font.SysFont("Consolas", 18, bold=True)


# ==========================================
# 2. DEFINING THE ENVIRONMENT CLASSES
# ==========================================
class Mirror:
    def __init__(self, p1, p2, is_border=False):
        self.p1 = pygame.math.Vector2(p1)
        self.p2 = pygame.math.Vector2(p2)
        self.is_border = is_border

        self.line_vec = self.p2 - self.p1
        self.normal = pygame.math.Vector2(-self.line_vec.y, self.line_vec.x)
        if self.normal.length() > 0:
            self.normal = self.normal.normalize()

    def draw(self, surface):
        color = (100, 130, 150) if self.is_border else (200, 255, 255)
        width = 2 if self.is_border else 4
        pygame.draw.line(surface, color, self.p1, self.p2, width)


class ReflectionData:
    def __init__(self, point, angle, distance):
        self.point = point
        self.angle = angle
        self.distance = distance


# ==========================================
# 3. LOGIC FUNCTIONS
# ==========================================
def get_intersection(ray_origin, ray_dir, mirror):
    x1, y1 = ray_origin.x, ray_origin.y
    x2, y2 = ray_origin.x + ray_dir.x, ray_origin.y + ray_dir.y
    x3, y3 = mirror.p1.x, mirror.p1.y
    x4, y4 = mirror.p2.x, mirror.p2.y

    den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
    if den == 0:
        return None

    t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den
    u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / den

    if t > 0 and 0 <= u <= 1:
        return pygame.math.Vector2(x1 + t * (x2 - x1), y1 + t * (y2 - y1))
    return None


def calculate_reflection(incoming_vec, normal_vec):
    dot_product = incoming_vec.dot(normal_vec)
    reflection = incoming_vec - 2 * dot_product * normal_vec
    return reflection.normalize()


def generate_mirrors(count):
    new_mirrors = []
    for _ in range(count):
        x, y = random.randint(50, SIM_WIDTH - 150), random.randint(50, HEIGHT - 150)
        angle = random.uniform(0, math.pi * 2)
        length = random.randint(60, 130)
        p2 = (x + math.cos(angle) * length, y + math.sin(angle) * length)
        new_mirrors.append(Mirror((x, y), p2))
    return new_mirrors


# ==========================================
# 4. STATE & INITIALIZATION
# ==========================================
mirror_count = 8
mirrors = generate_mirrors(mirror_count)
laser_origin = pygame.math.Vector2(SIM_WIDTH // 2, HEIGHT // 2)
MAX_BOUNCES = 60
auto_rotate = False
rotation_angle = 0.0

# UI Buttons
btn_plus = pygame.Rect(SIM_WIDTH + 20, 140, 40, 30)
btn_minus = pygame.Rect(SIM_WIDTH + 70, 140, 40, 30)
btn_reset = pygame.Rect(SIM_WIDTH + 120, 140, 80, 30)
btn_auto = pygame.Rect(SIM_WIDTH + 210, 140, 90, 30)

# ==========================================
# 5. MAIN LOOP
# ==========================================
running = True
while running:
    mouse_pos = pygame.math.Vector2(pygame.mouse.get_pos())

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        if event.type == pygame.MOUSEBUTTONDOWN:
            if btn_plus.collidepoint(event.pos):
                mirror_count = min(30, mirror_count + 1)
                mirrors = generate_mirrors(mirror_count)
            elif btn_minus.collidepoint(event.pos):
                mirror_count = max(0, mirror_count - 1)
                mirrors = generate_mirrors(mirror_count)
            elif btn_reset.collidepoint(event.pos):
                mirrors = generate_mirrors(mirror_count)
            elif btn_auto.collidepoint(event.pos):
                auto_rotate = not auto_rotate
            elif event.pos[0] < SIM_WIDTH:
                laser_origin = pygame.math.Vector2(event.pos)

    if auto_rotate:
        rotation_angle += 0.01
        laser_dir = pygame.math.Vector2(
            math.cos(rotation_angle), math.sin(rotation_angle)
        )
    else:
        laser_dir = mouse_pos - laser_origin
        if laser_dir.length() > 0:
            laser_dir = laser_dir.normalize()
        else:
            laser_dir = pygame.math.Vector2(1, 0)

    # --- RENDERING ---
    screen.fill((5, 5, 10))
    for m in mirrors:
        m.draw(screen)
    pygame.draw.circle(
        screen, (255, 80, 80), (int(laser_origin.x), int(laser_origin.y)), 10, 2
    )
    pygame.draw.circle(
        screen, (255, 30, 30), (int(laser_origin.x), int(laser_origin.y)), 4
    )

    # --- RAYCASTING ---
    cur_origin = pygame.math.Vector2(laser_origin)
    cur_dir = pygame.math.Vector2(laser_dir)
    bounces = []
    total_dist = 0

    for _ in range(MAX_BOUNCES):
        closest_pt, hit_mirror = None, None
        min_d = float("inf")

        for m in mirrors:
            pt = get_intersection(cur_origin, cur_dir, m)
            if pt:
                d = cur_origin.distance_to(pt)
                if 0.1 < d < min_d:
                    min_d, closest_pt, hit_mirror = d, pt, m

        if closest_pt:
            color = (255, 50, 50)
            pygame.draw.line(screen, color, cur_origin, closest_pt, 2)
            pygame.draw.circle(
                screen, (255, 200, 200), (int(closest_pt.x), int(closest_pt.y)), 2
            )

            total_dist += min_d
            angle = cur_dir.angle_to(hit_mirror.normal)
            bounces.append(ReflectionData(closest_pt, angle, min_d))

            cur_origin = closest_pt
            cur_dir = calculate_reflection(cur_dir, hit_mirror.normal)
        else:
            pygame.draw.line(
                screen, (255, 50, 50), cur_origin, cur_origin + cur_dir * 1000, 2
            )
            break

    # --- UI RENDER ---
    pygame.draw.rect(screen, (25, 25, 35), (SIM_WIDTH, 0, UI_WIDTH, HEIGHT))
    pygame.draw.line(screen, (60, 60, 70), (SIM_WIDTH, 0), (SIM_WIDTH, HEIGHT), 2)

    y = 20
    screen.blit(
        title_font.render("OPTICS ANALYTICS", True, (255, 255, 255)),
        (SIM_WIDTH + 20, y),
    )
    y += 40

    stats = [
        f"Mirrors: {mirror_count}",
        f"Bounces: {len(bounces)}",
        f"Distance: {int(total_dist)} px",
        f"Emitter: ({int(laser_origin.x)}, {int(laser_origin.y)})",
    ]
    for s in stats:
        screen.blit(ui_font.render(s, True, (180, 180, 180)), (SIM_WIDTH + 20, y))
        y += 20

    # Draw Buttons
    y = 140
    for rect, label in [
        (btn_plus, "+"),
        (btn_minus, "-"),
        (btn_reset, "Shuffle"),
        (btn_auto, "Auto-Spin"),
    ]:
        color = (
            (50, 70, 90) if rect.collidepoint(pygame.mouse.get_pos()) else (40, 50, 60)
        )
        pygame.draw.rect(screen, color, rect, border_radius=4)
        txt = ui_bold.render(label, True, (255, 255, 255))
        screen.blit(
            txt,
            (rect.centerx - txt.get_width() // 2, rect.centery - txt.get_height() // 2),
        )

    # Reflection Diagnostics List
    y = 190
    pygame.draw.line(
        screen, (60, 60, 70), (SIM_WIDTH + 20, y), (SIM_WIDTH + UI_WIDTH - 20, y)
    )
    y += 15
    screen.blit(
        ui_bold.render("RECENT REFLECTIONS", True, (255, 255, 255)), (SIM_WIDTH + 20, y)
    )
    y += 25

    for i, b in enumerate(bounces[:12]):
        color = (150, 150, 150) if i % 2 == 0 else (120, 120, 120)
        diag = f"#{i + 1:02} | Angle: {abs(b.angle):.1f}° | Dist: {int(b.distance)}px"
        screen.blit(ui_font.render(diag, True, color), (SIM_WIDTH + 20, y))
        y += 18

    if not bounces:
        screen.blit(
            ui_font.render("No reflections active.", True, (100, 100, 100)),
            (SIM_WIDTH + 20, y),
        )

    # Help text
    screen.blit(
        ui_font.render("Click SIM area to move Emitter", True, (100, 110, 120)),
        (SIM_WIDTH + 20, HEIGHT - 30),
    )

    pygame.display.flip()
    clock.tick(60)

pygame.quit()
