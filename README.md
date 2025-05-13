# 🐍 Snake Game - OpenGL C++
![snake game](https://github.com/user-attachments/assets/6fabcaa1-533b-43b2-99e1-0fda1dc75781)

Welcome to the Snake Game! This README provides essential instructions on how to play, control the snake, and understand the game’s mechanics.

---

## 🛠️ System Requirements
- **Operating System:** Windows, macOS, or Linux  
- **Graphics:** OpenGL 3.3 or higher  
- **Libraries Required:**
  - GLAD  
  - GLFW  
  - GLM  
  - stb_image

---

## ⚙️ Installation

1. **Install Dependencies**  
   Use a package manager like `apt`, `brew`, or `vcpkg` to install:
   - `GLFW`
   - `GLM`
   - `GLAD` (manual or use glad loader)

2. **Compile the Game**
   ```bash
   g++ main.cpp -o SnakeGame -lglfw -lGL -lGLU -lglad
   ```

3. **Run the Game**
   ```bash
   ./SnakeGame
   ```

---

## 🎮 Gameplay Instructions & Controls

Navigate the snake, collect food, and avoid collisions to keep playing and increase your score.

### Controls:
- `↑` – Move up  
- `↓` – Move down  
- `←` – Move left  
- `→` – Move right  
- `Space Bar` – Temporarily slow down  
- `Left Ctrl` – Temporarily speed up  

---

## 📐 Game Mechanics

- **Movement:** The snake moves automatically in its current direction. Change direction using arrow keys.
- **Food:**
  - Small food = +1 point
  - Every 3 small foods = spawns a big food (+2 points)
- **Game Over Conditions:**
  - Hitting the wall
  - Hitting the snake’s own body

### Restarting
To restart after a game over, simply close and reopen the game.

---

## 🧠 Understanding the Math

The game leverages several math concepts:

- **2D Coordinate System:** Each object (snake head, body, food) uses a 2D vector position.
- **Collision Detection:** Uses Euclidean distance:
  ```cpp
  if (glm::distance(head.position, food.position) < SQUARE_SIZE) {
      // Collision detected
  }
  ```
- **Transformations:** Snake movement and screen rendering use transformation matrices from GLM.

---

## 🧱 Code Structure Overview

- **Initialization:**
  - GLFW and GLAD setup
  - Shader creation and linking

- **Game Loop:**
  - Processes input  
  - Updates snake position and game state  
  - Renders each frame

- **Input Handling:**
  - Arrow keys change direction
  - Space/Ctrl modify game speed

- **Rendering Functions:**
  - `drawSquare()` and similar functions draw the snake, food, and game area

---

## 🧰 Troubleshooting
If the game won’t start, make sure all dependencies are installed.

