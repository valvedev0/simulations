import math
import random

import pygame


pygame.init()
pygame.font.init()

SIM_WIDTH = 800
UI_WIDTH = 380
HEIGHT = 650
MAX_BOUNCES = 12
TARGET_SAFE_RADIUS = 54

screen = pygame.display.set_mode((SIM_WIDTH + UI_WIDTH, HEIGHT))
pygame.display.set_caption("Laser Puzzle")
clock = pygame.time.Clock()

ui_font = pygame.font.SysFont("Consolas", 14)
ui_bold = pygame.font.SysFont("Consolas", 15, bold=True)
title_font = pygame.font.SysFont("Consolas", 20, bold=True)
small_font = pygame.font.SysFont("Consolas", 12)


class Mirror:
    def __init__(self, p1, p2):
        self.p1 = pygame.math.Vector2(p1)
        self.p2 = pygame.math.Vector2(p2)

        line_vec = self.p2 - self.p1
        self.normal = pygame.math.Vector2(-line_vec.y, line_vec.x)
        if self.normal.length() > 0:
            self.normal = self.normal.normalize()

    def draw(self, surface):
        pygame.draw.line(surface, (200, 255, 255), self.p1, self.p2, 4)


class Target:
    def __init__(self, x, y, points=20, radius=12):
        self.pos = pygame.math.Vector2(x, y)
        self.points = points
        self.radius = radius
        self.active = True

    def draw(self, surface):
        if not self.active:
            return
        safe_radius = game_state.target_safe_radius()
        zone_surface = pygame.Surface((safe_radius * 2, safe_radius * 2), pygame.SRCALPHA)
        pygame.draw.circle(
            zone_surface,
            (90, 255, 170, 42),
            (safe_radius, safe_radius),
            safe_radius,
        )
        surface.blit(zone_surface, (self.pos.x - safe_radius, self.pos.y - safe_radius))
        pygame.draw.circle(surface, (100, 255, 150), self.pos, self.radius, 3)
        pygame.draw.circle(surface, (180, 255, 210), self.pos, max(2, self.radius - 5), 1)


class Enemy:
    def __init__(self, x, y, vx, vy, radius=14):
        self.pos = pygame.math.Vector2(x, y)
        self.vel = pygame.math.Vector2(vx, vy)
        self.radius = radius
        self.active = True

    def update(self):
        if not self.active:
            return

        self.pos += self.vel

        if self.pos.x < self.radius or self.pos.x > SIM_WIDTH - self.radius:
            self.vel.x *= -1
            self.pos.x = max(self.radius, min(SIM_WIDTH - self.radius, self.pos.x))

        if self.pos.y < self.radius or self.pos.y > HEIGHT - self.radius:
            self.vel.y *= -1
            self.pos.y = max(self.radius, min(HEIGHT - self.radius, self.pos.y))

    def draw(self, surface):
        if not self.active:
            return
        pygame.draw.circle(surface, (255, 90, 90), self.pos, self.radius)
        pygame.draw.circle(surface, (255, 210, 210), self.pos, self.radius, 2)


class ReflectionData:
    def __init__(self, distance):
        self.distance = distance


class GameState:
    STATE_PLAYING = "playing"
    STATE_LEVEL_COMPLETE = "level_complete"
    STATE_GAME_OVER = "game_over"

    def __init__(self):
        self.score = 0
        self.high_score = 0
        self.level = 1
        self.state = self.STATE_PLAYING
        self.total_targets = 0
        self.targets_hit = 0
        self.enemy_count = 0
        self.level_flash = 0.0
        self.game_over_flash = 0.0
        self.game_over_reason = ""
        self.time_remaining = 0.0

    def start_level(self, total_targets, enemy_count):
        self.state = self.STATE_PLAYING
        self.total_targets = total_targets
        self.targets_hit = 0
        self.enemy_count = enemy_count
        self.level_flash = 0.0
        self.game_over_flash = 0.0
        self.game_over_reason = ""
        self.time_remaining = self.level_time_limit()

    def record_target_hit(self, points):
        self.score += points
        self.targets_hit += 1

    def remaining_targets(self):
        return self.total_targets - self.targets_hit

    def beam_speed(self):
        return max(4.0, 14.0 - (self.level - 1) * 1.2)

    def target_safe_radius(self):
        return min(96, TARGET_SAFE_RADIUS + (self.level - 1) * 6)

    def level_time_limit(self):
        return max(12.0, 28.0 - (self.level - 1) * 1.4)

    def update(self, dt):
        self.high_score = max(self.high_score, self.score)
        if self.state == self.STATE_PLAYING:
            self.time_remaining = max(0.0, self.time_remaining - dt)
            if self.time_remaining <= 0.0:
                self.trigger_game_over("Time Up")
        elif self.state == self.STATE_LEVEL_COMPLETE:
            self.level_flash += 0.1
        elif self.state == self.STATE_GAME_OVER:
            self.game_over_flash += 0.1

    def check_level_complete(self):
        if self.state == self.STATE_PLAYING and self.remaining_targets() == 0:
            self.state = self.STATE_LEVEL_COMPLETE

    def trigger_game_over(self, reason):
        if self.state == self.STATE_PLAYING:
            self.state = self.STATE_GAME_OVER
            self.game_over_reason = reason

    def restart_game(self):
        self.score = 0
        self.level = 1
        self.state = self.STATE_PLAYING
        self.game_over_reason = ""


def get_intersection(ray_origin, ray_dir, mirror):
    x1, y1 = ray_origin.x, ray_origin.y
    x2, y2 = ray_origin.x + ray_dir.x, ray_origin.y + ray_dir.y
    x3, y3 = mirror.p1.x, mirror.p1.y
    x4, y4 = mirror.p2.x, mirror.p2.y

    denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
    if denominator == 0:
        return None

    t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator
    u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator

    if t > 0 and 0 <= u <= 1:
        return pygame.math.Vector2(x1 + t * (x2 - x1), y1 + t * (y2 - y1))
    return None


def calculate_reflection(incoming_vec, normal_vec):
    dot_product = incoming_vec.dot(normal_vec)
    reflection = incoming_vec - 2 * dot_product * normal_vec
    return reflection.normalize()


def point_to_segment_distance(point, seg_start, seg_end):
    segment = seg_end - seg_start
    seg_len_sq = segment.length_squared()
    if seg_len_sq == 0:
        return point.distance_to(seg_start)

    t = max(0.0, min(1.0, (point - seg_start).dot(segment) / seg_len_sq))
    closest = seg_start + segment * t
    return point.distance_to(closest)


def segment_hits_circle(seg_start, seg_end, center, radius):
    return point_to_segment_distance(center, seg_start, seg_end) <= radius


def can_place_emitter(position):
    for target in targets:
        if target.active and position.distance_to(target.pos) < game_state.target_safe_radius():
            return False
    return True


def generate_mirrors(count):
    new_mirrors = []
    for _ in range(count):
        x = random.randint(80, SIM_WIDTH - 180)
        y = random.randint(80, HEIGHT - 120)
        angle = random.uniform(0, math.pi * 2)
        length = random.randint(90, 130)
        end = (x + math.cos(angle) * length, y + math.sin(angle) * length)
        new_mirrors.append(Mirror((x, y), end))
    return new_mirrors


def generate_targets(count):
    items = []
    for _ in range(count):
        x = random.randint(120, SIM_WIDTH - 120)
        y = random.randint(110, HEIGHT - 110)
        items.append(Target(x, y))
    return items


def generate_enemies(count):
    items = []
    for _ in range(count):
        x = random.randint(150, SIM_WIDTH - 150)
        y = random.randint(120, HEIGHT - 120)
        vx = random.choice([-1, 1]) * random.uniform(1.4, 2.2)
        vy = random.choice([-1, 1]) * random.uniform(1.2, 2.0)
        items.append(Enemy(x, y, vx, vy))
    return items


def build_laser_path(origin, direction):
    segments = []
    reflections = []
    total_distance = 0.0
    cur_origin = pygame.math.Vector2(origin)
    cur_dir = pygame.math.Vector2(direction)

    for _ in range(MAX_BOUNCES):
        closest_pt = None
        hit_mirror = None
        min_distance = float("inf")

        for mirror in mirrors:
            point = get_intersection(cur_origin, cur_dir, mirror)
            if not point:
                continue

            distance = cur_origin.distance_to(point)
            if 0.1 < distance < min_distance:
                min_distance = distance
                closest_pt = point
                hit_mirror = mirror

        if closest_pt and hit_mirror:
            segments.append((pygame.math.Vector2(cur_origin), pygame.math.Vector2(closest_pt)))
            reflections.append(ReflectionData(min_distance))
            total_distance += min_distance

            next_dir = calculate_reflection(cur_dir, hit_mirror.normal)
            cur_origin = closest_pt + next_dir * 0.2
            cur_dir = next_dir
        else:
            end_point = cur_origin + cur_dir * 1200
            segments.append((pygame.math.Vector2(cur_origin), pygame.math.Vector2(end_point)))
            total_distance += cur_origin.distance_to(end_point)
            break

    return segments, reflections, total_distance


def reset_beam():
    global beam_progress, last_beam_signature
    beam_progress = 0.0
    last_beam_signature = None


def handle_segment_hits(seg_start, seg_end):
    if game_state.state != GameState.STATE_PLAYING:
        return

    for target in targets:
        if target.active and segment_hits_circle(seg_start, seg_end, target.pos, target.radius + 3):
            target.active = False
            game_state.record_target_hit(target.points)

    for enemy in enemies:
        if enemy.active and segment_hits_circle(seg_start, seg_end, enemy.pos, enemy.radius + 2):
            game_state.trigger_game_over("Drone Hit")
            return


def init_level():
    global mirrors, targets, enemies, mirror_count, laser_origin, layout_revision

    mirror_count = min(6, 3 + game_state.level)
    target_count = min(5, 2 + game_state.level)
    enemy_count = min(4, 1 + game_state.level // 2)

    mirrors = generate_mirrors(mirror_count)
    targets = generate_targets(target_count)
    enemies = generate_enemies(enemy_count)
    laser_origin = pygame.math.Vector2(100, HEIGHT // 2)
    game_state.start_level(target_count, enemy_count)
    layout_revision += 1
    reset_beam()


def draw_button(rect, label):
    hovered = rect.collidepoint(pygame.mouse.get_pos())
    fill = (46, 58, 72) if hovered else (36, 45, 57)
    pygame.draw.rect(screen, fill, rect, border_radius=6)
    pygame.draw.rect(screen, (80, 105, 128), rect, width=1, border_radius=6)
    text = ui_bold.render(label, True, (245, 245, 245))
    screen.blit(text, (rect.centerx - text.get_width() // 2, rect.centery - text.get_height() // 2))


game_state = GameState()
mirrors = []
targets = []
enemies = []
laser_origin = pygame.math.Vector2(100, HEIGHT // 2)
mirror_count = 4
layout_revision = 0
beam_progress = 0.0
last_beam_signature = None

btn_shuffle = pygame.Rect(SIM_WIDTH + 20, 270, UI_WIDTH - 40, 34)
btn_reset = pygame.Rect(SIM_WIDTH + 20, 314, UI_WIDTH - 40, 34)
btn_next_level = pygame.Rect(SIM_WIDTH + 80, 300, 220, 40)
btn_restart = pygame.Rect(SIM_WIDTH + 80, 352, 220, 40)

init_level()

running = True
while running:
    mouse_pos = pygame.math.Vector2(pygame.mouse.get_pos())
    dt = clock.tick(60) / 1000.0
    game_state.update(dt)

    if game_state.state == GameState.STATE_PLAYING:
        for enemy in enemies:
            enemy.update()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if game_state.state == GameState.STATE_PLAYING:
                if btn_shuffle.collidepoint(event.pos):
                    mirrors = generate_mirrors(mirror_count)
                    layout_revision += 1
                    reset_beam()
                elif btn_reset.collidepoint(event.pos):
                    init_level()
                elif event.pos[0] < SIM_WIDTH:
                    proposed_origin = pygame.math.Vector2(event.pos)
                    if can_place_emitter(proposed_origin):
                        laser_origin = proposed_origin
                        reset_beam()
            elif game_state.state == GameState.STATE_LEVEL_COMPLETE:
                if btn_next_level.collidepoint(event.pos):
                    game_state.level += 1
                    init_level()
                elif btn_restart.collidepoint(event.pos):
                    game_state.restart_game()
                    init_level()
            elif game_state.state == GameState.STATE_GAME_OVER:
                if btn_restart.collidepoint(event.pos):
                    game_state.restart_game()
                    init_level()

    if mouse_pos.x < SIM_WIDTH:
        laser_dir = mouse_pos - laser_origin
    else:
        laser_dir = pygame.math.Vector2(1, 0)

    if laser_dir.length() == 0:
        laser_dir = pygame.math.Vector2(1, 0)
    else:
        laser_dir = laser_dir.normalize()

    beam_signature = (
        round(laser_origin.x, 0),
        round(laser_origin.y, 0),
        round(laser_dir.x, 2),
        round(laser_dir.y, 2),
        layout_revision,
        game_state.level,
    )
    if beam_signature != last_beam_signature:
        beam_progress = 0.0
        last_beam_signature = beam_signature

    segments, reflections, total_path_distance = build_laser_path(laser_origin, laser_dir)
    if game_state.state == GameState.STATE_PLAYING:
        beam_progress = min(total_path_distance, beam_progress + game_state.beam_speed())

    screen.fill((6, 8, 12))
    pygame.draw.rect(screen, (10, 14, 20), (0, 0, SIM_WIDTH, HEIGHT))

    for mirror in mirrors:
        mirror.draw(screen)

    for target in targets:
        target.draw(screen)

    for enemy in enemies:
        enemy.draw(screen)

    pygame.draw.circle(screen, (255, 120, 120), laser_origin, 10, 2)
    pygame.draw.circle(screen, (255, 70, 70), laser_origin, 4)

    visible_remaining = beam_progress
    visible_reflections = []
    visible_distance = 0.0

    for seg_start, seg_end in segments:
        segment_vec = seg_end - seg_start
        segment_length = segment_vec.length()
        if segment_length == 0 or visible_remaining <= 0:
            break

        draw_length = min(segment_length, visible_remaining)
        draw_end = seg_start + segment_vec.normalize() * draw_length

        pygame.draw.line(screen, (255, 70, 70), seg_start, draw_end, 2)
        handle_segment_hits(seg_start, draw_end)

        visible_distance += draw_length
        if draw_length == segment_length:
            visible_reflections.append(ReflectionData(segment_length))
            pygame.draw.circle(screen, (255, 220, 220), seg_end, 3)

        visible_remaining -= draw_length
        if game_state.state == GameState.STATE_GAME_OVER:
            break

    game_state.check_level_complete()

    pygame.draw.rect(screen, (24, 28, 38), (SIM_WIDTH, 0, UI_WIDTH, HEIGHT))
    pygame.draw.line(screen, (68, 74, 88), (SIM_WIDTH, 0), (SIM_WIDTH, HEIGHT), 2)

    y = 24
    screen.blit(title_font.render("LASER PUZZLE", True, (120, 245, 210)), (SIM_WIDTH + 20, y))
    y += 34
    screen.blit(
        small_font.render("Clear green targets. Touching a red drone with the beam ends the round.", True, (170, 185, 200)),
        (SIM_WIDTH + 20, y),
    )
    y += 34

    ui_lines = [
        (f"Score: {game_state.score}", (255, 210, 90), ui_bold),
        (f"High Score: {game_state.high_score}", (180, 180, 130), small_font),
        (f"Level: {game_state.level}", (220, 220, 220), ui_font),
        (f"Targets Left: {game_state.remaining_targets()}", (120, 255, 170), ui_font),
        (f"Hazard Drones: {game_state.enemy_count}", (255, 120, 120), ui_font),
        (f"Safe Radius: {game_state.target_safe_radius()} px", (150, 230, 190), ui_font),
        (f"Time Left: {game_state.time_remaining:0.1f} s", (255, 205, 145), ui_font),
        (f"Beam Speed: {game_state.beam_speed():.1f} px/frame", (190, 210, 255), ui_font),
        (f"Visible Reflections: {len(visible_reflections)}", (200, 200, 200), ui_font),
        (f"Beam Distance: {int(visible_distance)} px", (200, 200, 200), ui_font),
    ]

    for text, color, font in ui_lines:
        screen.blit(font.render(text, True, color), (SIM_WIDTH + 20, y))
        y += 22

    y += 12
    draw_button(btn_shuffle, "Shuffle Mirrors")
    draw_button(btn_reset, "Reset Round")

    y = 388
    screen.blit(ui_bold.render("How To Play", True, (245, 245, 245)), (SIM_WIDTH + 20, y))
    y += 24

    help_lines = [
        "1. Move the mouse to aim the pulse.",
        "2. Click in the arena to move the emitter.",
        "3. Let the pulse travel across the mirrors.",
        "4. Safety circles grow every level.",
        "5. Beat the timer before it reaches zero.",
        "6. Hit every green target and avoid red drones.",
    ]

    for line in help_lines:
        screen.blit(small_font.render(line, True, (175, 185, 198)), (SIM_WIDTH + 20, y))
        y += 20

    y += 8
    screen.blit(ui_bold.render("Reflection Distances", True, (245, 245, 245)), (SIM_WIDTH + 20, y))
    y += 22

    if visible_reflections:
        for index, reflection in enumerate(visible_reflections[:5]):
            line = f"#{index + 1}: {int(reflection.distance)} px"
            screen.blit(small_font.render(line, True, (165, 170, 182)), (SIM_WIDTH + 20, y))
            y += 18
    else:
        screen.blit(small_font.render("No mirror hit yet.", True, (120, 128, 140)), (SIM_WIDTH + 20, y))

    if game_state.state == GameState.STATE_LEVEL_COMPLETE:
        overlay = pygame.Surface((SIM_WIDTH, HEIGHT))
        overlay.set_alpha(205)
        overlay.fill((8, 12, 18))
        screen.blit(overlay, (0, 0))

        pulse = 80 + int(50 * (0.5 + 0.5 * math.sin(game_state.level_flash)))
        clear_text = title_font.render("ROUND CLEAR", True, (pulse, 255, 140))
        screen.blit(clear_text, (SIM_WIDTH // 2 - clear_text.get_width() // 2, HEIGHT // 2 - 90))

        score_text = ui_font.render(f"Score: {game_state.score}", True, (255, 220, 120))
        screen.blit(score_text, (SIM_WIDTH // 2 - score_text.get_width() // 2, HEIGHT // 2 - 30))

        prompt_text = ui_font.render(f"Ready for Level {game_state.level + 1}?", True, (210, 210, 210))
        screen.blit(prompt_text, (SIM_WIDTH // 2 - prompt_text.get_width() // 2, HEIGHT // 2 + 8))

        draw_button(btn_next_level, "Next Level")
        draw_button(btn_restart, "Restart Game")

    elif game_state.state == GameState.STATE_GAME_OVER:
        overlay = pygame.Surface((SIM_WIDTH, HEIGHT))
        overlay.set_alpha(210)
        overlay.fill((22, 10, 10))
        screen.blit(overlay, (0, 0))

        flash = 140 + int(50 * (0.5 + 0.5 * math.sin(game_state.game_over_flash)))
        over_text = title_font.render(game_state.game_over_reason or "ROUND LOST", True, (255, flash, flash))
        screen.blit(over_text, (SIM_WIDTH // 2 - over_text.get_width() // 2, HEIGHT // 2 - 90))

        if game_state.game_over_reason == "Time Up":
            prompt_line = "The timer ran out before you cleared the targets."
        else:
            prompt_line = "The reflected beam touched a red drone."

        prompt = ui_font.render(prompt_line, True, (230, 210, 210))
        screen.blit(prompt, (SIM_WIDTH // 2 - prompt.get_width() // 2, HEIGHT // 2 - 24))

        score_text = ui_font.render(f"Score: {game_state.score}", True, (255, 220, 120))
        screen.blit(score_text, (SIM_WIDTH // 2 - score_text.get_width() // 2, HEIGHT // 2 + 10))

        draw_button(btn_restart, "Restart Game")

    pygame.display.flip()
pygame.quit()
