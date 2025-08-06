# doom.c

FPS engine in C, combining: 
 - **doom-style BSP collision and entity handling**
 - **Duke Nukem1996-style sector/portal rendering**
designed for non-Euclidean level layouts, efficient traversal, and classic software-rendered visuals

---

## 🧩 overview

- **Doom-style BSP** for spatial subdivision and collision detection. (log(n))
- **Portal/sector-based rendering** inspired by the build engine (Duke Nukem 3D).
- supports room-over-room layouts, recursive portal rendering, and advanced level connectivity.
- built-in C with minimal dependencies (SDL2).

---

## sreenshots

<p align="center">
  <img src="images/sc2.png" alt="minimap" width="32%">
</p>

---

## features

-  BSP-based collision and entity placement
- sector & portal rendering system
- support for room-over-room and recursive layouts
- software renderer using front-to-back portal traversal
- doom-like movement, physics, and entity management

---

## 🛠️ Build & Run

### Requirements

- SDL2
- C99-compatible compiler (GCC, Clang, MSVC)

### Steps

```bash
git clone https://github.com/youradrien/doom.c.git doom-nukem
cd doom-nukem
