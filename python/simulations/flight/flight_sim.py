import math
import random
import time
from collections import deque

import pygame


pygame.init()
pygame.font.init()

SIM_WIDTH = 900
UI_WIDTH = 320
HEIGHT = 640

screen = pygame.display.set_mode((SIM_WIDTH + UI_WIDTH, HEIGHT))
pygame.display.set_caption("Interactive Flight Simulation with Diagnostics")
clock = pygame.time.Clock()

ui_font = pygame.font.SysFont("Consolas", 14)
title_font = pygame.font.SysFont("Consolas", 18, bold=True)


class FlightSim:
    def __init__(self):
        self.pos = pygame.math.Vector2(SIM_WIDTH * 0.5, HEIGHT * 0.6)
        self.vel = pygame.math.Vector2(0, 0)
        self.heading = -90.0
        self.throttle = 0.25
        self.max_speed = 380.0
        self.accel = 240.0
        self.drag = 0.14
        self.turn_speed = 120.0

        self.autopilot = False
        self.target_pos = pygame.math.Vector2(SIM_WIDTH * 0.7, HEIGHT * 0.3)

        self.distance_traveled = 0.0
        self.max_speed_seen = 0.0

    def forward_vec(self):
        rad = math.radians(self.heading)
        return pygame.math.Vector2(math.cos(rad), math.sin(rad))

    def update(self, dt, held_keys):
        diagnostics = {
            "waypoint_reached_now": False,
            "thrusting_now": False,
            "turning_now": False,
            "braking_now": False,
        }

        turn_input = 0.0
        throttle_delta = 0.0
        braking = False

        if held_keys[pygame.K_a] or held_keys[pygame.K_LEFT]:
            turn_input -= 1.0
        if held_keys[pygame.K_d] or held_keys[pygame.K_RIGHT]:
            turn_input += 1.0
        if held_keys[pygame.K_w] or held_keys[pygame.K_UP]:
            throttle_delta += 0.60
            diagnostics["thrusting_now"] = True
        if held_keys[pygame.K_s] or held_keys[pygame.K_DOWN]:
            throttle_delta -= 0.60
            diagnostics["braking_now"] = True
            braking = True

        if self.autopilot:
            to_target = self.target_pos - self.pos
            if to_target.length() > 0:
                desired_heading = math.degrees(math.atan2(to_target.y, to_target.x))
                delta = (desired_heading - self.heading + 180.0) % 360.0 - 180.0
                if abs(delta) > 1.2:
                    turn_input += 1.0 if delta > 0 else -1.0
                self.throttle = min(max(self.throttle, 0.35), 0.75)
            if to_target.length() < 20.0:
                diagnostics["waypoint_reached_now"] = True

        if abs(turn_input) > 0.01:
            self.heading += turn_input * self.turn_speed * dt
            diagnostics["turning_now"] = True

        self.heading = (self.heading + 360.0) % 360.0
        self.throttle = min(max(self.throttle + throttle_delta * dt, 0.0), 1.0)

        thrust = self.forward_vec() * (self.accel * self.throttle)
        self.vel += thrust * dt

        if braking:
            self.vel *= 0.96

        self.vel *= (1.0 - self.drag * dt)

        speed = self.vel.length()
        if speed > self.max_speed:
            self.vel.scale_to_length(self.max_speed)
            speed = self.max_speed

        old_pos = pygame.math.Vector2(self.pos)
        self.pos += self.vel * dt

        # Bounce from boundaries to keep the simulation visible.
        if self.pos.x < 0:
            self.pos.x = 0
            self.vel.x *= -0.5
        elif self.pos.x > SIM_WIDTH:
            self.pos.x = SIM_WIDTH
            self.vel.x *= -0.5
        if self.pos.y < 0:
            self.pos.y = 0
            self.vel.y *= -0.5
        elif self.pos.y > HEIGHT:
            self.pos.y = HEIGHT
            self.vel.y *= -0.5

        self.distance_traveled += self.pos.distance_to(old_pos)
        self.max_speed_seen = max(self.max_speed_seen, speed)

        return diagnostics

    def draw(self, surface):
        # Waypoint
        pygame.draw.circle(surface, (240, 210, 80), (int(self.target_pos.x), int(self.target_pos.y)), 8, 2)
        pygame.draw.circle(surface, (240, 210, 80), (int(self.target_pos.x), int(self.target_pos.y)), 2)

        # Heading line
        nose = self.pos + self.forward_vec() * 18
        pygame.draw.line(surface, (255, 255, 255), self.pos, nose, 2)

        # Plane body (triangle)
        right = pygame.math.Vector2(-self.forward_vec().y, self.forward_vec().x)
        p1 = nose
        p2 = self.pos - self.forward_vec() * 10 + right * 8
        p3 = self.pos - self.forward_vec() * 10 - right * 8
        pygame.draw.polygon(surface, (120, 220, 255), [p1, p2, p3])


def draw_background(surface, stars):
    surface.fill((16, 20, 28))
    for sx, sy, sr in stars:
        pygame.draw.circle(surface, (180, 190, 210), (sx, sy), sr)


def main():
    sim = FlightSim()
    stars = [(random.randint(0, SIM_WIDTH - 1), random.randint(0, HEIGHT - 1), random.randint(1, 2)) for _ in range(120)]

    events_total = 0
    quit_events = 0
    keydown_events = 0
    keyup_events = 0
    mousebutton_events = 0
    mousemotion_events = 0

    frame_ms_history = deque(maxlen=120)
    update_ms_history = deque(maxlen=120)
    render_ms_history = deque(maxlen=120)

    frame_idx = 0

    running = True
    sim_start = time.perf_counter()

    try:
        while running:
            frame_start = time.perf_counter()
            frame_idx += 1

            for event in pygame.event.get():
                events_total += 1
                if event.type == pygame.QUIT:
                    quit_events += 1
                    running = False
                elif event.type == pygame.KEYDOWN:
                    keydown_events += 1
                    if event.key == pygame.K_ESCAPE:
                        running = False
                    elif event.key == pygame.K_p:
                        sim.autopilot = not sim.autopilot
                    elif event.key == pygame.K_SPACE:
                        sim.vel *= 0.0
                elif event.type == pygame.KEYUP:
                    keyup_events += 1
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    mousebutton_events += 1
                    mx, my = pygame.mouse.get_pos()
                    if mx < SIM_WIDTH:
                        if event.button == 1:
                            sim.target_pos.update(mx, my)
                        elif event.button == 3:
                            sim.autopilot = not sim.autopilot
                elif event.type == pygame.MOUSEMOTION:
                    mousemotion_events += 1

            dt = clock.get_time() / 1000.0
            if dt <= 0.0:
                dt = 1.0 / 60.0

            update_start = time.perf_counter()
            keys = pygame.key.get_pressed()
            sim.update(dt, keys)
            update_end = time.perf_counter()

            render_start = time.perf_counter()
            draw_background(screen, stars)
            sim.draw(screen)

            ui_rect = pygame.Rect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT)
            pygame.draw.rect(screen, (12, 12, 16), ui_rect)
            pygame.draw.line(screen, (90, 90, 100), (SIM_WIDTH, 0), (SIM_WIDTH, HEIGHT), 2)

            elapsed = time.perf_counter() - sim_start
            speed = sim.vel.length()

            avg_frame_ms = (sum(frame_ms_history) / len(frame_ms_history)) if frame_ms_history else 0.0
            avg_update_ms = (sum(update_ms_history) / len(update_ms_history)) if update_ms_history else 0.0
            avg_render_ms = (sum(render_ms_history) / len(render_ms_history)) if render_ms_history else 0.0

            y = 16
            screen.blit(title_font.render("FLIGHT DIAGNOSTICS", True, (255, 255, 255)), (SIM_WIDTH + 14, y))
            y += 32

            lines = [
                f"Elapsed: {elapsed:7.2f} s",
                f"Frame: {frame_idx}",
                f"FPS: {clock.get_fps():6.2f}",
                f"Frame ms(avg): {avg_frame_ms:7.3f}",
                f"Update ms(avg): {avg_update_ms:7.3f}",
                f"Render ms(avg): {avg_render_ms:7.3f}",
                f"Pos: ({sim.pos.x:7.1f}, {sim.pos.y:7.1f})",
                f"Speed: {speed:7.2f} px/s",
                f"Heading: {sim.heading:7.2f} deg",
                f"Throttle: {sim.throttle:5.2f}",
                f"Autopilot: {sim.autopilot}",
                f"Distance total: {sim.distance_traveled:8.1f}",
                f"Max speed: {sim.max_speed_seen:7.2f}",
                "",
                f"Events total: {events_total}",
                f"quit/keyD/keyU: {quit_events}/{keydown_events}/{keyup_events}",
                f"mouseDown/motion: {mousebutton_events}/{mousemotion_events}",
                "",
                "Controls:",
                "W/S or Up/Down: throttle",
                "A/D or Left/Right: turn",
                "LMB: set waypoint",
                "RMB or P: toggle autopilot",
                "SPACE: zero velocity",
                "ESC: quit",
            ]

            for line in lines:
                color = (210, 210, 210) if line else (120, 120, 120)
                screen.blit(ui_font.render(line, True, color), (SIM_WIDTH + 14, y))
                y += 18

            pygame.display.flip()
            render_end = time.perf_counter()

            clock.tick(60)

            frame_end = time.perf_counter()
            frame_ms = (frame_end - frame_start) * 1000.0
            update_ms = (update_end - update_start) * 1000.0
            render_ms = (render_end - render_start) * 1000.0

            frame_ms_history.append(frame_ms)
            update_ms_history.append(update_ms)
            render_ms_history.append(render_ms)

    finally:
        pygame.quit()


if __name__ == "__main__":
    main()
