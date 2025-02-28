/**
* Author: Benjamin Tian
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <iostream>
#include <vector>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 2.0f;

constexpr int WINDOW_WIDTH  = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char BEAKER_SPRITE_FILEPATH[] = "messi.jpg",
               DROPS_SPRITE_FILEPATH[]  = "ronaldo.jpg",
               BALL_SPRITE_FILEPATH[]  = "ball.png",
                PLAYER1_SPRITE_FILEPATH[]  = "player1.png",
                PLAYER2_SPRITE_FILEPATH[]  = "player2.png";

//drops is left
//beaker is right paddle

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_DROPS  = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_DROPS    = glm::vec3(-4.0f, 0.0f, 0.0f),
                    INIT_SCALE_BEAKER = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_BEAKER   = glm::vec3(4.0f, 0.0f, 0.0f),
                    INIT_SCALE_BALL = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_BALL   = glm::vec3(0.0f, 0.0f, 0.0f);
                                        
// Maximum number of balls
constexpr int MAX_BALLS = 3;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_beaker_matrix, g_projection_matrix, g_drops_matrix, g_win_matrix;

// Array of ball matrices
glm::mat4 g_ball_matrices[MAX_BALLS];

float g_previous_ticks = 0.0f;

GLuint g_beaker_texture_id;
GLuint g_drops_texture_id;
GLuint g_ball_texture_id;

GLuint g_winner_texture_id;
GLuint g_win1_texture_id;
GLuint g_win2_texture_id;

bool game_over = false;  // Global flag to track game state
std::string g_winner_text = "";

// Current number of active balls (default: 1)
int g_active_balls = 1;

constexpr float DROPS_SPEED = 3.0f,
                BEAKER_SPEED = 3.0f,
                AI_SPEED = 25.0f; // speed of drop

glm::vec3 g_beaker_position   = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_beaker_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_drops_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_drops_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_drops_scale = glm::vec3(0.0f, 0.0f, 0.0f);  // scale trigger vector
glm::vec3 g_drops_size  = glm::vec3(1.0f, 1.0f, 0.0f);  // scale accumulator vector

const float BALL_SPEED = 2.0f;
const float SCREEN_TOP = 3.0f;
const float SCREEN_BOTTOM = -3.0f;
const float SCREEN_LEFT = -4.5f;
const float SCREEN_RIGHT = 4.5f;

const float PADDLE_WIDTH = 0.2f;
const float PADDLE_HEIGHT = 1.0f;

// Ball properties arrays
glm::vec3 g_ball_positions[MAX_BALLS];
glm::vec3 g_ball_velocities[MAX_BALLS];
bool g_ball_active[MAX_BALLS] = {true, false, false}; // Only first ball active by default

SDL_Renderer* renderer;

void initialise();
void process_input();
void update();
void render();
void shutdown();
void reset_ball(int ball_index);

void render_texture(GLuint textureID, float x, float y, float width, float height);
void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id);


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

// Function to reset a ball to center with random velocity
void reset_ball(int ball_index) {
    // Reset position to center
    g_ball_positions[ball_index] = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Set random direction for the ball
    float angle = (float)(rand() % 360) * 3.14159f / 180.0f; // Random angle in radians
    g_ball_velocities[ball_index].x = BALL_SPEED * cos(angle);
    g_ball_velocities[ball_index].y = BALL_SPEED * sin(angle);
    
    // Make sure x velocity is not too small
    if (fabs(g_ball_velocities[ball_index].x) < BALL_SPEED * 0.5f) {
        g_ball_velocities[ball_index].x = (g_ball_velocities[ball_index].x > 0) ?
                                         BALL_SPEED * 0.5f : -BALL_SPEED * 0.5f;
    }
}

void initialise()
{
    // Seed random number generator
    srand(static_cast<unsigned int>(time(NULL)));
    
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("User-Input and Collisions Exercise",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_beaker_matrix = glm::mat4(1.0f);
    g_drops_matrix = glm::mat4(1.0f);
    g_win_matrix = glm::mat4(1.0f);
    
    // Initialize all ball matrices
    for (int i = 0; i < MAX_BALLS; i++) {
        g_ball_matrices[i] = glm::mat4(1.0f);
        reset_ball(i);
    }
    
    g_win_matrix = glm::translate(g_win_matrix, INIT_POS_BALL);

    g_drops_matrix = glm::translate(g_drops_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_drops_position += g_drops_movement;

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_beaker_texture_id = load_texture(BEAKER_SPRITE_FILEPATH);
    g_drops_texture_id = load_texture(DROPS_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    
    g_win1_texture_id = load_texture(PLAYER1_SPRITE_FILEPATH);
    g_win2_texture_id = load_texture(PLAYER2_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool g_ai_mode = false; // Track AI mode
float g_ai_direction = 1.0f; // Direction of AI movement

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                    case SDLK_t:
                        g_ai_mode = !g_ai_mode; // Toggle AI mode
                        break;
                    // Handle number keys 1-3 to change number of balls
                    case SDLK_1:
                        g_active_balls = 1;
                        // Set active flags
                        g_ball_active[0] = true;
                        g_ball_active[1] = false;
                        g_ball_active[2] = false;
                        break;
                    case SDLK_2:
                        g_active_balls = 2;
                        // Set active flags
                        g_ball_active[0] = true;
                        g_ball_active[1] = true;
                        g_ball_active[2] = false;
                        // Reset second ball position if it was inactive
                        reset_ball(1);
                        break;
                    case SDLK_3:
                        g_active_balls = 3;
                        // Set active flags
                        g_ball_active[0] = true;
                        g_ball_active[1] = true;
                        g_ball_active[2] = true;
                        // Reset inactive balls
                        if (!g_ball_active[1]) reset_ball(1);
                        if (!g_ball_active[2]) reset_ball(2);
                        break;
                    case SDLK_r: // Reset game
                        game_over = false;
                        for (int i = 0; i < MAX_BALLS; i++) {
                            reset_ball(i);
                        }
                        break;
                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Reset movement before checking input
    g_drops_movement.y = 0.0f;
    g_beaker_movement.y = 0.0f;

    if (!g_ai_mode) // Only allow player control if AI mode is off
    {
        if (key_state[SDL_SCANCODE_W] && g_drops_position.y < 3.0f)
        {
            g_drops_movement.y = 1.0f;
        }
        if (key_state[SDL_SCANCODE_S] && g_drops_position.y > -3.0f)
        {
            g_drops_movement.y = -1.0f;
        }
    }

    if (key_state[SDL_SCANCODE_UP] && g_beaker_position.y < 3.0f)
    {
        g_beaker_movement.y = 1.0f;
    }
    if (key_state[SDL_SCANCODE_DOWN] && g_beaker_position.y > -3.0f)
    {
        g_beaker_movement.y = -1.0f;
    }
}

void UpdateBall(int ball_index, float delta_time) {
    // Skip if ball is not active
    if (!g_ball_active[ball_index] || game_over) {
        return;
    }
    
    // Move the ball
    g_ball_positions[ball_index] += g_ball_velocities[ball_index] * delta_time;

    // Bounce off top and bottom walls
    if ((g_ball_positions[ball_index].y >= SCREEN_TOP && g_ball_velocities[ball_index].y > 0) ||
        (g_ball_positions[ball_index].y <= SCREEN_BOTTOM && g_ball_velocities[ball_index].y < 0)) {
        g_ball_velocities[ball_index].y *= -1.0f; // Reverse Y direction
    }

    // Collision detection with left paddle (g_drops_position)
    float x_distance_left = fabs(g_ball_positions[ball_index].x - INIT_POS_DROPS.x) -
                            ((INIT_SCALE_BALL.x + INIT_SCALE_DROPS.x) / 2.0f);
    float y_distance_left = fabs(g_ball_positions[ball_index].y - g_drops_position.y) -
                            ((INIT_SCALE_BALL.y + INIT_SCALE_DROPS.y) / 2.0f);

    if (x_distance_left < 0 && y_distance_left < 0 && g_ball_velocities[ball_index].x < 0) {
        g_ball_velocities[ball_index].x *= -1.0f; // Reverse X direction
    }

    // Collision detection with right paddle (g_beaker_position)
    float x_distance_right = fabs(g_ball_positions[ball_index].x - INIT_POS_BEAKER.x) -
                             ((INIT_SCALE_BALL.x + INIT_SCALE_BEAKER.x) / 2.0f);
    float y_distance_right = fabs(g_ball_positions[ball_index].y - g_beaker_position.y) -
                             ((INIT_SCALE_BALL.y + INIT_SCALE_BEAKER.y) / 2.0f);

    if (x_distance_right < 0 && y_distance_right < 0 && g_ball_velocities[ball_index].x > 0) {
        g_ball_velocities[ball_index].x *= -1.0f; // Reverse X direction
    }

    // Check for game over (ball passes left or right screen boundary)
    if (g_ball_positions[ball_index].x <= SCREEN_LEFT) {
        // Player 2 (right) scores
        g_winner_texture_id = g_win2_texture_id;
        g_winner_text = "Player 2 Wins!";
        game_over = true;
    }
    else if (g_ball_positions[ball_index].x >= SCREEN_RIGHT) {
        // Player 1 (left) scores
        g_winner_texture_id = g_win1_texture_id;
        g_winner_text = "Player 1 Wins!";
        game_over = true;
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    // AI logic: Simple up and down movement
    if (g_ai_mode)
    {
        // Reverse direction when hitting screen bounds
        if (g_drops_position.y >= 3.0f && g_ai_direction > 0.0f) {
            g_ai_direction = -1.0f; // Reverse direction
        }
        else if (g_drops_position.y <= -3.0f && g_ai_direction < 0.0f) {
            g_ai_direction = 1.0f; // Reverse direction
        }
        g_drops_movement.y = g_ai_direction * AI_SPEED * delta_time;
    }

    // Update all balls
    for (int i = 0; i < MAX_BALLS; i++) {
        UpdateBall(i, delta_time);
    }

    // Update positions
    g_drops_position += g_drops_movement * DROPS_SPEED * delta_time;
    g_beaker_position += g_beaker_movement * BEAKER_SPEED * delta_time;

    // Apply transformations
    g_beaker_matrix = glm::mat4(1.0f);
    g_beaker_matrix = glm::translate(g_beaker_matrix, INIT_POS_BEAKER);
    g_beaker_matrix = glm::translate(g_beaker_matrix, g_beaker_position);
    g_beaker_matrix = glm::scale(g_beaker_matrix, INIT_SCALE_BEAKER);

    g_drops_matrix = glm::mat4(1.0f);
    g_drops_matrix = glm::translate(g_drops_matrix, INIT_POS_DROPS);
    g_drops_matrix = glm::translate(g_drops_matrix, g_drops_position);
    g_drops_matrix = glm::scale(g_drops_matrix, INIT_SCALE_DROPS);
    g_drops_matrix = glm::scale(g_drops_matrix, g_drops_size);
    
    // Update all ball matrices
    for (int i = 0; i < MAX_BALLS; i++) {
        g_ball_matrices[i] = glm::mat4(1.0f);
        g_ball_matrices[i] = glm::translate(g_ball_matrices[i], INIT_POS_BALL);
        g_ball_matrices[i] = glm::translate(g_ball_matrices[i], g_ball_positions[i]);
        g_ball_matrices[i] = glm::scale(g_ball_matrices[i], INIT_SCALE_BALL);
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 9); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f , //triangle 2
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f // triangle 3
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, //triangle 2
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,// triangle 3
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    if (game_over) {
        draw_object(g_win_matrix, g_winner_texture_id); // Render winner image
    } else {
        // Draw paddles
        draw_object(g_beaker_matrix, g_beaker_texture_id);
        draw_object(g_drops_matrix, g_drops_texture_id);
        
        // Draw only active balls
        for (int i = 0; i < MAX_BALLS; i++) {
            if (g_ball_active[i]) {
                draw_object(g_ball_matrices[i], g_ball_texture_id);
            }
        }
    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
