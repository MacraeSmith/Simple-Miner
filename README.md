# Simple Miner

<p align="center">
  <img src="Media/SimpleMiner_Cover.png" width="100%">
</p>

<p align="center">
  <strong>Custom C++ Engine | DirectX 11 | Procedural Voxel World</strong>
</p>

## Overview

Simple Miner is a Minecraft-inspired voxel sandbox built entirely within my custom C++ game engine.

The project focuses on procedural world generation, multithreaded chunk streaming, voxel lighting, fluid simulation, and large-scale terrain generation. The world is generated dynamically around the player using a chunk-based architecture that supports effectively endless exploration while maintaining predictable memory and CPU usage.

Terrain generation combines multiple layered noise functions to create varied landscapes, underground cave networks, climate-driven biomes, and procedural vegetation.

## Technical Deep Dive

For a detailed breakdown of the procedural generation systems, chunk streaming architecture, voxel lighting, fluid simulation, and world-building techniques used in this project, visit the full project page:

**https://www.macraesmith.com/projects/simple-miner**

---

## Technologies

* C++
* DirectX 11
* HLSL
* Custom Game Engine
* Multi-Threaded Job System
* Procedural Generation
* Voxel Rendering

---

## Key Features

### Infinite Voxel World

* Chunk-based world architecture
* Dynamic loading and unloading
* Configurable render distance
* Multithreaded chunk generation

### Advanced Procedural Generation

* Layered Perlin and Fractal noise
* Ridged and domain-warped noise
* Climate-based biome generation
* Procedural vegetation placement
* Large-scale cave systems

### Voxel Lighting

* Dynamic skylight propagation
* Emissive light sources
* Flood-fill light propagation
* Real-time lighting updates

### Liquid Simulation

* Dynamic water flow
* Lava simulation
* Cross-chunk liquid propagation
* Height-based flow visualization

### Terrain Destruction

* TNT explosions
* Dynamic terrain modification
* Lighting recalculation
* Physics interactions

### Player Systems

* First-person controls
* Block interaction
* Physics-based movement
* Real-time world editing

---

## Screenshots

<p align="center">
  <img src="Media/SimpleMiner_3DTerrain.png" width="90%">
</p>

<p align="center">
  <img src="Media/SimpleMiner_Lighting.png" width="90%">
</p>

---

## Running the Project

### Launch Prebuilt Executable

```text
Run/SimpleMiner_Release_x64.exe
```

### Build From Source

1. Open the solution in Visual Studio 2022
2. Build:

```text
Ctrl + Shift + B
```

3. Run:

```text
F5
```

Recommended build configurations:

* Release
* Fast_Break

---

## Major Systems

### Chunk Streaming

The world is partitioned into fixed-size chunks that load and unload around the player. Chunk generation runs entirely on worker threads and transitions back to the main thread only when integration into the active world is required.

### Terrain Generation

Terrain is generated using layered Perlin, Fractal, Ridged, and domain-warped noise. Separate noise groups control terrain shaping, caves, climate, and biome distribution, allowing each system to be tuned independently.

### Cave Generation

Large underground cave networks are created using volumetric density fields combined with cheese cave generation and domain-warped tunnel systems.

### Climate and Biomes

Temperature and humidity maps drive biome placement, vegetation spawning, and environmental variation across the world.

### Voxel Lighting

Flood-fill light propagation supports both skylight and emissive light sources while dynamically responding to terrain changes.

### Liquid Simulation

Water and lava use a voxel flood-fill system that supports gravity, directional flow, and seamless propagation across chunk boundaries.

### TNT and Destruction

Explosions modify terrain in real time, trigger lighting updates, and interact with the entity physics system to create dynamic chain reactions.

---



