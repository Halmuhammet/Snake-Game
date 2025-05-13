/*
 * Title: Snake Game with OpenGL
 * Description: A classic Snake game implementation using modern OpenGL
 * Author: Halmuhammet Muhamedorazov
 * Date: 12/02/2024
 * Version number: 1.0
 * Compiler versions: g++ 13.2.0, gcc 11.4.0
 * Libraries required: GLAD, GLFW 3.3+, GLM, stb_image
 * Controls: Use UP, DOWN, RIGHT, LEFT arrow keys to change the snake's direction
      SPACE to slow down the game
      LEFT CONTROL to speed up the game
 * Additional features: Textured graphics, variable game speed, big food spawning
*/

// Import necessary libraries
#include "gif.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>


// Variable to control game speed dynamically
float GAME_SPEED = 0.012f; // Time interval for snake movement which controls game speed

// Stores the initial game speed to allow resetting
float game_speed_controller = GAME_SPEED;

// Tracks the player's current score
int score = 0;

// Flags to manage food spawning and tracking
bool bigFoodOnScreen = false;
bool smallFoodOnScreen = true;
int smallFoodEaten = 0;
int bigFoodEaten = 0;

// Constants for game window and object dimensions
const float windowWIDTH = 800.0f;      // Width of the game window
const float windowHEIGHT = 600.0f;     // Height of the game window
const float SQUARE_SIZE = 20.0f;       // Size of game objects (snake segments, food)
const float WALL_THICKNESS = 60.0f;    // Thickness of game boundaries

// Timing and movement constants
const int SEGMENT_DELAY_MS = 50;       // Delay between snake segment movements
const float MOVE_STRIDE = 2.5; // Distance moved in a single step

// Timing variable to control rendering and movement speed
float lastMoveTime = 0.0f; // Used to allow for some time to pass before rendering to make the game playable

// Game state tracking
bool gameOver = false; // Tracks whether the game has ended, initially set to false

// Enum to represent possible movement directions of the snake
enum Direction { UP, DOWN, LEFT, RIGHT };

// Struct to represent snake segments and food items
struct Square {
    glm::vec2 position;           // Current position
    Direction direction;          // Movement direction

};

// Snake is represented as a vector of Square segments
std::vector<Square> snake;

// Food items
Square smallFood;
Square bigFood;

// Initial snake movement direction
Direction currentDirection = RIGHT; // Snake starts moving to the right
// Stores the next direction based on user input
Direction nextDirection = currentDirection;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void drawSquare(const Square& square, unsigned int shaderProgram, unsigned int VAO, bool useTexture, unsigned int textureID, glm::vec3 color);
void spawnFood(bool isBigFood);
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
unsigned int loadTexture(char const* path);
void useBackgroundTexture(unsigned int shaderProgram, GLuint backgroundVAO, unsigned int backgroundTextureID, glm::mat4 projection);
void setupBackgroundBuffers(GLuint& backgroundVAO, GLuint& backgroundVBO);
void setupSnakeBuffers(GLuint& squareVAO, GLuint& squareVBO, bool isBigFood);


// Vertex shader source code
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    uniform mat4 model;
    uniform mat4 projection;
    out vec2 TexCoord;
    void main() {
        // apply projection to map the coordinates to the window
        gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)glsl";

// Fragment shader souce code
const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D texture1;
    uniform vec4 color;
    uniform bool useTexture;
    void main() {
        if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
            if (texColor.a < 0.1) // Discard nearly transparent pixels
                discard;
                FragColor = texColor;
        } else {
            FragColor = color;
        }
    }
)glsl";

/*
* main method is the starting point of this program
*/

int main() {
    // GLFW initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWIDTH, windowHEIGHT, "Snake Game", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD initialization
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set up shaders
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Create and bind VBO and VAO for body segments
    unsigned int squareVBO, squareVAO;
    setupSnakeBuffers(squareVAO, squareVBO, false);
    unsigned int bodyTexture = loadTexture("textures/body3.png");

    // Create separte VAO and VBO for head so that we can apply different texture to it

    unsigned int headVAO, headVBO;
    setupSnakeBuffers(headVAO, headVBO, false);
    // load the snake head texture
    unsigned int headTexture = loadTexture("textures/head1.png");
   

    // Now, we need VAO and VBO for the small food
    unsigned int smallFoodVAO, smallFoodVBO;
    setupSnakeBuffers(smallFoodVAO, smallFoodVBO, false);
    
    // We also need VAO and VBO for big food
    unsigned int bigFoodVAO, bigFoodVBO;
    setupSnakeBuffers(bigFoodVAO, bigFoodVBO, true);
    
    // load the snake head texture
    unsigned int foodTexture = loadTexture("textures/food.png");
  

    // Load the background texture
    unsigned int backgroundTextureID = loadTexture("textures/snakeBackground.png");

    //Setup VAO and VBO buffers for the background texture
    GLuint backgroundVAO, backgroundVBO;
    setupBackgroundBuffers(backgroundVAO, backgroundVBO);
  


    // Initialize game state
    // start the snake with one square in the middle of the screen going to the right
    snake.push_back({ glm::vec2(windowWIDTH / 2.0, windowHEIGHT / 2.0), RIGHT });
    // spawn the food in random place
    spawnFood(false);

    // Use orthgraphic projection matrix to convert the window coordinates 
    // to normalized device coordinate (NDC) which goes from -1 to 1
    glm::mat4 projection = glm::ortho(0.0f, windowWIDTH, 0.0f, windowHEIGHT);


    // rendering loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        float currentTime = static_cast<float>(glfwGetTime()); // get the current time
        float deltaTime = currentTime - lastMoveTime; // get the time interval (change in time) since last time snake moved

        // In each frame check whether enough time has passed (and game is not over)
        // to update the position of snake so that the game is playable and not too fast.

        if (deltaTime >= GAME_SPEED && !gameOver) {

            currentDirection = nextDirection; // update current direction
            lastMoveTime = currentTime;       // update last move time to current time

            // Move each segment of the snake by updating its position and direction
            // Start from the last segment and update its position and direction by
            // assigning the position and direction of the segment next to the last and so on.
            for (int i = snake.size() - 1; i > 0; i--) {
   
                snake[i].position = snake[i - 1].position;
                snake[i].direction = snake[i - 1].direction;
            }
    



            // Move the head depending on direction player chooses
            // Reference to the first element (head) of the snake vector
            Square& head = snake[0];
            // Set the head's direction to the current game direction
            head.direction = currentDirection;
            // Move the snake head based on the current direction
            switch (head.direction) {
            case UP:
                // Move snake head upward by increasing y-coordinate
                head.position.y += MOVE_STRIDE;
                break;
            case DOWN:
                // Move snake head downward by decreasing y-coordinate
                head.position.y -= MOVE_STRIDE;
                break;
            case LEFT:
                // Move snake head to the left by decreasing x-coordinate
                head.position.x -= MOVE_STRIDE;
                break;
            case RIGHT:
                // Move snake head to the right by increasing x-coordinate
                head.position.x += MOVE_STRIDE;
                break;
            }

            // Check collision with walls; if head collides with wall, game over.
            if (head.position.x < 0 + WALL_THICKNESS ||             //left wall
                head.position.x >= windowWIDTH - WALL_THICKNESS ||   //right wall
                head.position.y < 0 + WALL_THICKNESS - 17.0 ||       //bottom wall
                head.position.y >= windowHEIGHT - WALL_THICKNESS)   //top wall
            {
                gameOver = true;
            }

            // Check collision with snake's own body:
            // Iterate over each segment of the body (except head)
            // and check the distance between the head and selected body segment
            // If the distance between them is less than the square size (20.0f),
            // then it means that head and body segment collided each other
            // In that case, set the game over variable to true to end the game
            for (size_t i = 1; i < snake.size(); ++i) {
                // Use GLM distance function to check the distance between head and body segment
                if (glm::distance(head.position, snake[i].position) < MOVE_STRIDE) {
                    gameOver = true;
                    break;
                }
            }
            
           // Check for small food collision
            if (smallFoodOnScreen && glm::distance(head.position, smallFood.position) < SQUARE_SIZE) {
                // Add new segment to snake
                Square newSegment = snake.back();
                for (int i = 0; i < 25; i++) {
                    snake.push_back(newSegment);
                }
                // Increase score and small food counter
                score++;
                smallFoodEaten++;

                // Check if it's time for big food
                if (smallFoodEaten == 3) {
                    // Time for big food
                    spawnFood(true);
                    bigFoodOnScreen = true;
                    smallFoodOnScreen = false;
                    smallFoodEaten = 0;  // Reset the counter
                }
                else {
                    // Spawn new small food
                    spawnFood(false);
                    smallFoodOnScreen = true;
                }
            }

            // Check for big food collision
            if (bigFoodOnScreen && glm::distance(head.position, bigFood.position) < SQUARE_SIZE * 2) {
                // Add two new segments to snake
                Square newSegment = snake.back();
                for (int i = 0; i < 75; i++) {
                    snake.push_back(newSegment);
                }
                // Increase score
                score += 2;

                // Spawn small food and remove big food
                spawnFood(false);
                smallFoodOnScreen = true;
                bigFoodOnScreen = false;
            }
        }

        // set the background color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw background texture first
        useBackgroundTexture(shaderProgram, backgroundVAO, backgroundTextureID, projection);

        // Use shader program to render
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        // Bind the VAO
        glBindVertexArray(squareVAO);
        
        // Draw each segment of snake
        for (int i = 0; i < snake.size(); i++) {
            if (i == 0) {//this is for the head segment
                drawSquare(snake[i], shaderProgram, headVAO, true, headTexture, glm::vec3(0.0, 1.0, 0.0));
            }
            else {// body segments
                drawSquare(snake[i], shaderProgram, squareVAO, true, bodyTexture, glm::vec3(0.0, 1.0, 0.0));
            }
        }
        // Draw the food depending on which one needs to be rendered
        if (bigFoodOnScreen == true) {// render big food
            drawSquare(bigFood, shaderProgram, bigFoodVAO, true, foodTexture, glm::vec3(0.0, 1.0, 0.0));
            
        }
        else {// otherwise, render small food
            drawSquare(smallFood, shaderProgram, smallFoodVAO, true, foodTexture, glm::vec3(0.0, 1.0, 0.0));
        }
      
        // If game over, display "Game Over" message and the score to the console
        if (gameOver) {
            std::cerr << "Game Over" << std::endl;
            std::cerr << "Your Score: " <<score<<std::endl;

            break;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Cleanup
    glDeleteVertexArrays(1, &squareVAO);
    glDeleteBuffers(1, &squareVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}


/*
* This function is used for processing user input to control the movement of snake
* @param window: the current rendering window
*/

void processInput(GLFWwindow* window) {
    // Static flags to track key press states and prevent multiple triggers
    static bool spacePressed = false;
    static bool ctrlPressed = false;

    // Space bar functionality - slow down game speed
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        // Mark space as pressed to prevent repeated triggers
        spacePressed = true;
        // Reduce game speed, ensuring it doesn't go below 0.05
        if (GAME_SPEED > 0.003) {
            GAME_SPEED -= 0.005;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        // Reset space press state
        spacePressed = false;
        // Restore default game speed if Ctrl is not pressed
        if (!ctrlPressed) {
            GAME_SPEED = game_speed_controller;
        }
    }

    // Left Control functionality - speed up game
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && !ctrlPressed) {
        // Mark Ctrl as pressed to prevent repeated triggers
        ctrlPressed = true;
        // Set game speed to a fixed accelerated value
        GAME_SPEED = 0.5;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE) {
        // Reset Ctrl press state
        ctrlPressed = false;
        // Restore default game speed if space is not pressed
        if (!spacePressed) {
            GAME_SPEED = game_speed_controller;
        }
    }

    // Directional controls for game movement
    // Up arrow - change direction to UP if not currently moving DOWN
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && currentDirection != DOWN) {
        nextDirection = UP;
    }
    // Down arrow - change direction to DOWN if not currently moving UP
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && currentDirection != UP) {
        nextDirection = DOWN;
    }
    // Left arrow - change direction to LEFT if not currently moving RIGHT
    else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && currentDirection != RIGHT) {
        nextDirection = LEFT;
    }
    // Right arrow - change direction to RIGHT if not currently moving LEFT
    else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && currentDirection != LEFT) {
        nextDirection = RIGHT;
    }
}

/*
 * This function sets up the vertex buffer objects and vertex array object for the snake segments and food
 * @param squareVAO: reference to the Vertex Array Object for the square
 * @param squareVBO: reference to the Vertex Buffer Object for the square
 * @param isBigFood: boolean flag indicating whether to set up buffers for big food (true) or regular square (false)
*/

void setupSnakeBuffers(GLuint& squareVAO, GLuint& squareVBO, bool isBigFood) {
    // Determine the size of the square based on whether it's big food or not
    float halfSquareSize;
    if (isBigFood == true) {
        halfSquareSize = SQUARE_SIZE;
    
    } 
    else{
        halfSquareSize = SQUARE_SIZE / 2.0;
    }
    // Define vertex data for the square, including positions and texture coordinates
    float vertices[] = {
        // positions                         // texture coords
         halfSquareSize,  halfSquareSize, 0.0f,  1.0f, 1.0f,  // top right
         halfSquareSize, -halfSquareSize, 0.0f,  1.0f, 0.0f,  // bottom right
        -halfSquareSize, -halfSquareSize, 0.0f,  0.0f, 0.0f,  // bottom left
        -halfSquareSize,  halfSquareSize, 0.0f,  0.0f, 1.0f   // top left
    };
    // Define indices for drawing the square using two triangles
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    // Generate and bind Vertex Array Object (VAO) and a buffer
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindVertexArray(squareVAO);
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // bind the VBE
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VBO and VAO to prevent accidental modifications
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/*
* This function renders a square to the screen after applying transformation
* @param square: snake segment or food of type Square
* @param shaderProgram: the integer id of shader program to render the square
* @param VAO: Vertex Array Object that contains information about the square
*/

void drawSquare(const Square& square, unsigned int shaderProgram, unsigned int VAO, bool useTexture, unsigned int textureID, glm::vec3 color) {
    // Initialize identity matrix
    glm::mat4 identity = glm::mat4(1.0f);
    // Pass the identity matrix to the model matrix to do the translation operation
    glm::mat4 model = glm::translate(identity, glm::vec3(square.position, 0.0f));

    if (square.direction == UP) {
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    else if (square.direction == DOWN) {
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    else if (square.direction == RIGHT) {
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    else { // direction LEFT
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));



    // Send the model matrix to the vertex shader 
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    // Get the location of "useTexture" variable in the fragment shader and set the useTexture field to either true or false
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture);
    // Get the location of "color" variable in the fragment shader and give it a color
    glUniform4f(glGetUniformLocation(shaderProgram, "color"), color.x, color.y, color.z, 1.0f);

    // If a food or snake has a texture, then use that texture in the fragment shader and bypass color attribute
    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    }

    glBindVertexArray(VAO);
    // Draw the square as two triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
}

/*
 * This function spawns food (either big or small) at a random position on the screen
 * @param isBigFood: boolean flag indicating whether to spawn big food (true) or small food (false)
 */

void spawnFood(bool isBigFood) {
    // Get the max multiplicative factor of window to allow the food spawn within the window

    glm::vec2 newPosition;
    bool validPosition;
    do {
        //Get the new random position of food.
        //"% xMax" and "% yMax" allows the new coordinate to stay within window size
        int newX = int(WALL_THICKNESS + 40.0) + (rand() % (int(windowWIDTH) - 2 * int(WALL_THICKNESS + 2 * SQUARE_SIZE) + 1));
        int newY = int(WALL_THICKNESS + 40.0) + (rand() % (int(windowHEIGHT) - 2 * int(WALL_THICKNESS + 2 * SQUARE_SIZE) + 1));
        // Update the new postion of foood
        newPosition = glm::vec2(newX, newY);

        validPosition = true; // initialize the valid position to true

        // Don't allow the food to spawn on top of the snake
        for (const auto& segment : snake) {
            // Check the distance of food with each segment of snake. 
            // If the distance is less than the segment size (20.0f)
            // Then, the new position is on top of the snake. In that case, set the validPosition to false,
            // so that new food position can be re-calculated
            if (glm::distance(segment.position, newPosition) < SQUARE_SIZE) {
                validPosition = false;
                break;
            }
        }
    } while (!validPosition); // if the food wants spawn on top of snake, re-calculate a new position for it

    // Update the food's new position

    if (isBigFood == true) {
        bigFood.position = newPosition;
        std::cout << "Big Food spawned at: (" << newPosition.x << ", " << newPosition.y << ")" << std::endl;
    }
    else {
        smallFood.position = newPosition;
        std::cout << "Small Food spawned at: (" << newPosition.x << ", " << newPosition.y << ")" << std::endl;
    }
}

/*
 * This function sets up the vertex buffer objects and vertex array object for the background texture
 * @param backgroundVAO: reference to the Vertex Array Object for the background
 * @param backgroundVBO: reference to the Vertex Buffer Object for the background
*/

void setupBackgroundBuffers(GLuint& backgroundVAO, GLuint& backgroundVBO) {
    // Wall vertices (outline of the game window)
    /*
        Texture coordinate for background that covers the entire screen
        It forms two triangles with 3 vertices, each with its texture coordinates
    */

    float backgroundVertices[] = {
    0.0f, 0.0f, 0.0f, 0.0f,        // Bottom-left
    windowWIDTH, 0.0f, 1.0f, 0.0f, // Bottom-right
    windowWIDTH, windowHEIGHT, 1.0f, 1.0f, // Top-right
    0.0f, windowHEIGHT, 0.0f, 1.0f // Top-left
    };

    // Generate the VAO and VBO
    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);

    // Bind the VAO
    glBindVertexArray(backgroundVAO);

    // Bind the VBO and set the buffer data
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);

    // Set up vertex attribute pointers
    // first 2 elements in the background vertex is for position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // second 2 elements in the background vertex is for texture
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VBO and VAO to avoid accidental modifications
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/*
 * This function renders the background texture to the screen
 * @param shaderProgram: the integer id of shader program to render the background
 * @param backgroundVAO: Vertex Array Object that contains information about the background quad
 * @param backgroundTextureID: the ID of the background texture to be rendered
 * @param projection: the projection matrix for transforming coordinates
*/

void useBackgroundTexture(unsigned int shaderProgram, GLuint backgroundVAO, unsigned int backgroundTextureID, glm::mat4 projection) {

    glm::mat4 backgroundModel = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(backgroundModel));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(backgroundVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundTextureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_TRUE);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

/*
* This function helps with creating shader program
* @param vertexSource: vertex shader source code
* @param fragmentSource: fragment shader source code
* @return program: integer id of shader program
*/

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Compile the shader source codes
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    unsigned int program = glCreateProgram();
    // Attach each shader to the program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    // Link the vertex and fragment shaders to the program
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    // If linking wasn't successful, print an error message
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    //After the linking, we can delete the now useless compiled shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // Return the id (int) of program
    return program;
}

/*
* This function is used inside createShaderProgram to compile the shader source codes
* @param type: type of shader: fragment or vertex
* @return id: return the id of compiled shader
*/

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    // Compile the shader course code and assign it an int value
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    // If the compilation is not successful, print an error message
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Return the id (integer) of compiled shader
    return id;
}

/*
* This function loads a texture from a user provided file path and generates an int texture ID
* @param path: path to the texture to be loaded as string
* @return textureID: texture ID of loaded texture
*/

unsigned int loadTexture(char const* path)
{
    // variable to hold the textureID
    unsigned int textureID;
    stbi_set_flip_vertically_on_load(true);
    // Generate a texture object with its ID as well
    glGenTextures(1, &textureID);

    // Variables to hold texture dimensions and component count
    int width, height, nrComponents;
    // Load the texture data
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) // Check if the texture data was loaded witout any issues
    {
        // Determine the format based on the number of components
        GLenum format = GL_RGB; //rgb is the default
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        // Set the texture image data
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // create a mipmaps for the texture
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set the texture wrapping and filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Free the loaded image data
        stbi_image_free(data);
    }
    else // Handle loading failure
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    //return the texture ID
    return textureID;
}

/*
* This function is called whenever the window is resized
*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Adjust the OpenGL viewport to match the new window dimensions
    glViewport(0, 0, width, height);
}