import os
import runpy


def main():
    target = os.path.join(os.path.dirname(__file__), "ant_sim.py")
    runpy.run_path(target, run_name="__main__")


if __name__ == "__main__":
    main()
