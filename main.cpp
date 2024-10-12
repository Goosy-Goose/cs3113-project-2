/**
* Author: Selina Gong
* Assignment: Pong Clone
* Date due: 2024-10-12, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"  
#include "ShaderProgram.h"  
#include "stb_image.h"
#include "cmath"
#include "stdlib.h"
#include <ctime>
#include <vector>

enum AppStatus { RUNNING, TERMINATED };
enum FilterType { NEAREST, LINEAR };
enum PongCollisions { SIDEWALLS, BOUNCEWALLS, 
    PADDLECOLLISION, NOCOLLISION };

// window dimensions
constexpr float WINDOW_SIZE_MULT = 1.5f;
constexpr int WINDOW_WIDTH = 640 * WINDOW_SIZE_MULT,
WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

// background color components
constexpr float BG_RED = 0.7098f,
BG_BLUE = 0.95686f,
BG_GREEN = 1.0f,
BG_OPACITY = 1.0f;

// Our viewport
constexpr int   VIEWPORT_X = 0,
                VIEWPORT_Y = 0,
                VIEWPORT_WIDTH = WINDOW_WIDTH,
                VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Shader filepaths
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";


constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
bool ROUND_STARTED = false;


// ------ PADDLES VARS ------ //
constexpr char  PADDLE1_FILEPATH[] = "sprites/paddle-1.png",
                PADDLE2_FILEPATH[] = "sprites/paddle-2.png";

    /* initial scale and position */
constexpr glm::vec3 INIT_SCALE_PADDLES = glm::vec3(0.33f, 0.98125f, 0.0f), // 0.264f, 0.786f
                    INIT_POS_PADDLE1 = glm::vec3(3.2f, 0.0f, 0.0f),  // paddle 1 is on the right
                    INIT_POS_PADDLE2 = glm::vec3(-3.2f, 0.0f, 0.0f); // paddle 2 is on the left
    /* model matrixes */
glm::mat4 g_paddle1_matrix,
          g_paddle2_matrix;

GLuint g_paddle1_texture_id, 
       g_paddle2_texture_id;

constexpr float PADDLE_SPEED = 3.0f;
bool paddle2_is_npc = false;
    
    /* paddle 1 movement */ 
glm::vec3 g_paddle1_position = glm::vec3(0.0f, 0.0f, 0.0f); // accumulator
glm::vec3 g_paddle1_movement = glm::vec3(0.0f, 0.0f, 0.0f); // direction "unit" vector
    /* paddle 2 movement */
glm::vec3 g_paddle2_position = glm::vec3(0.0f, 0.0f, 0.0f); // accumulator
glm::vec3 g_paddle2_movement = glm::vec3(0.0f, 0.0f, 0.0f); // direction "unit" vector

// ------ PONGS (yarns) VARS ------ //
constexpr char  YARN1_FILEPATH[] = "sprites/yarn-1.png",
                YARN2_FILEPATH[] = "sprites/yarn-2.png",
                YARN3_FILEPATH[] = "sprites/yarn-3.png";

    /* initial scale and position */
constexpr glm::vec3 INIT_SCALE_PONGS = glm::vec3(0.5f, 0.5f, 0.0f),
                    INIT_POS_PONG1 = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_PONG2 = glm::vec3(0.0f, 1.5f, 0.0f),
                    INIT_POS_PONG3 = glm::vec3(0.0f, -1.5f, 0.0f);
    /* model matrixes */
glm::mat4 g_pong1_matrix,
          g_pong2_matrix,
          g_pong3_matrix;

GLuint g_pong1_texture_id,
       g_pong2_texture_id,
       g_pong3_texture_id;

constexpr float PONG_AVG_SPEED = 1.5f;
float pong1_speed,
      pong2_speed,
      pong3_speed;
int current_pong_count = 1;

    /* paddle 1 movement */ 
glm::vec3 g_pong1_position = glm::vec3(0.0f, 0.0f, 0.0f); // accumulator
glm::vec3 g_pong1_movement = glm::vec3(0.0f, 0.0f, 0.0f); // direction "unit" vector
    /* paddle 2 movement */
glm::vec3 g_pong2_position = glm::vec3(0.0f, 0.0f, 0.0f); // accumulator
glm::vec3 g_pong2_movement = glm::vec3(0.0f, 0.0f, 0.0f); // direction "unit" vector
    /* paddle 2 movement */
glm::vec3 g_pong3_position = glm::vec3(0.0f, 0.0f, 0.0f); // accumulator
glm::vec3 g_pong3_movement = glm::vec3(0.0f, 0.0f, 0.0f); // direction "unit" vector


// ------------ end pongs ------------------ //

GLuint g_font_texture_id;
constexpr char FONTSHEET_FILEPATH[] = "sprites/font1.png";
constexpr int FONTBANK_SIZE = 16;
std::string display_message = "";
std::string display_message_restart = "";

GLuint g_background_texture_id;
constexpr char BACKGROUND_FILEPATH[] = "sprites/torn_wallpaper.png";
glm::mat4 g_background_matrix;
constexpr glm::vec3 INIT_POS_BACKGROUND = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_SCALE_BACKGROUND = glm::vec3(10.0f, 7.5f, 0.0f);

// vars for generating and binding texture IDs
constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL = 0,
                TEXTURE_BORDER = 0;

float g_previous_ticks = 0.0f;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
          g_model_matrix_one,       // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
          g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.




void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

GLuint load_texture(const char* filepath, FilterType filterType)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

float random_pong_speed() {
    double rand_max = static_cast<double>(RAND_MAX);
    return (PONG_AVG_SPEED + (std::rand() / (rand_max) * 0.6));
}

void set_random_pong_direction(glm::vec3& g_pong_movement) {
    double rand_max = static_cast<double>(RAND_MAX);
    if ((std::rand() / rand_max) < 0.5) { g_pong_movement.x = -1; }
    else { g_pong_movement.x = 1; }
    if ((std::rand() / rand_max) < 0.5) { g_pong_movement.y = -1; }
    else { g_pong_movement.y = 1; }
}

void reset_level() {
    display_message = "Press space to start";
    display_message_restart = "";

    g_paddle1_matrix = glm::mat4(1.0f);
    g_paddle1_matrix = glm::translate(g_paddle1_matrix, INIT_POS_PADDLE1);
    g_paddle1_position = glm::vec3(0.0f, 0.0f, 0.0f);

    g_paddle2_matrix = glm::mat4(1.0f);
    g_paddle2_matrix = glm::translate(g_paddle2_matrix, INIT_POS_PADDLE2);
    g_paddle2_position = glm::vec3(0.0f, 0.0f, 0.0f);

    g_pong1_matrix = glm::mat4(1.0f);
    g_pong1_matrix = glm::translate(g_pong1_matrix, INIT_POS_PONG1);
    g_pong1_position = glm::vec3(0.0f, 0.0f, 0.0f);

    g_pong2_matrix = glm::mat4(1.0f);
    g_pong2_matrix = glm::translate(g_pong2_matrix, INIT_POS_PONG2);
    g_pong2_position = glm::vec3(0.0f, 0.0f, 0.0f);

    g_pong3_matrix = glm::mat4(1.0f);
    g_pong3_matrix = glm::translate(g_pong3_matrix, INIT_POS_PONG3);
    g_pong3_position = glm::vec3(0.0f, 0.0f, 0.0f);
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;

        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Load shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    // Initialise view and projection matrices
    g_view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    // Set shaders
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    //-------- text ------------- //
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH, NEAREST);
    display_message = "Press space to start.";
    display_message_restart = "Press 1,2,3 for yarns";
    //-------- background ------------- //
    g_background_texture_id = load_texture(BACKGROUND_FILEPATH, NEAREST);
    g_background_matrix = glm::mat4(1.0f);
    g_background_matrix = glm::translate(g_background_matrix, INIT_POS_BACKGROUND);
    g_background_matrix = glm::scale(g_background_matrix, INIT_SCALE_BACKGROUND);

    //----------PADDLES----------//
    g_paddle1_texture_id = load_texture(PADDLE1_FILEPATH, NEAREST);
    g_paddle2_texture_id = load_texture(PADDLE2_FILEPATH, NEAREST);

   /* paddle 1 position init */
    g_paddle1_matrix = glm::mat4(1.0f);
    g_paddle1_matrix = glm::translate(g_paddle1_matrix, glm::vec3(1.0f, 1.0f, 0.0f));

    /* paddle 2 position init */
    g_paddle2_matrix = glm::mat4(1.0f);
    g_paddle2_matrix = glm::translate(g_paddle2_matrix, glm::vec3(1.0f, 1.0f, 0.0f));


    //--------PONGS (balls)-----------//
    g_pong1_texture_id = load_texture(YARN1_FILEPATH, NEAREST);
    g_pong2_texture_id = load_texture(YARN2_FILEPATH, NEAREST);
    g_pong3_texture_id = load_texture(YARN3_FILEPATH, NEAREST);

    /* pong 1 position init */
    g_pong1_matrix = glm::mat4(1.0f);
    g_pong1_matrix = glm::translate(g_pong1_matrix, glm::vec3(1.0f, 1.0f, 0.0f));

    /* pong 2 position init */
    g_pong2_matrix = glm::mat4(1.0f);
    g_pong2_matrix = glm::translate(g_pong2_matrix, glm::vec3(1.0f, 1.0f, 0.0f));

    /* pong 3 position init */
    g_pong3_matrix = glm::mat4(1.0f);
    g_pong3_matrix = glm::translate(g_pong3_matrix, glm::vec3(1.0f, 1.0f, 0.0f));

    /*set random pong speeds and directions*/
    pong1_speed = random_pong_speed();
    set_random_pong_direction(g_pong1_movement);
    pong2_speed = random_pong_speed();
    set_random_pong_direction(g_pong2_movement);
    pong3_speed = random_pong_speed();
    set_random_pong_direction(g_pong3_movement);
}

void process_input()
{
    g_paddle1_movement = glm::vec3(0.0f);
    /* only stops movement when no key input if player 2 is controlled */
    if (!paddle2_is_npc) {
        g_paddle2_movement = glm::vec3(0.0f);
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
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
                /* switch between npc and pc */
                paddle2_is_npc = !paddle2_is_npc;
                if (g_paddle2_movement.y == 0) {
                    g_paddle2_movement.y = 1.0f;
                }
                break;
            case SDLK_r:
                reset_level();
                break;
            case SDLK_SPACE:
                if (!ROUND_STARTED) {
                    ROUND_STARTED = true;
                    display_message = "";
                    display_message_restart = "";
                }
                break;
            case SDLK_1:
                current_pong_count = 1;
                break;
            case SDLK_2:
                current_pong_count = 2;
                break;
            case SDLK_3:
                current_pong_count = 4;
                break;
            default:
                break;
            }

        default:
            break;
        }
    }// end of while sdl poll event

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_UP]) {
        if (g_paddle1_position.y < 3.5f) {
            g_paddle1_movement.y = 1.0f;
        }
    }
    else if (key_state[SDL_SCANCODE_DOWN]) {
        if (g_paddle1_position.y > -3.5f) {
            g_paddle1_movement.y = -1.0f;

        }
    }

    if (!paddle2_is_npc) {
        if (key_state[SDL_SCANCODE_W]) {
            if (g_paddle2_position.y < 3.5f) {
                g_paddle2_movement.y = 1.0f;
            }
        }
        else if (key_state[SDL_SCANCODE_S]) {
            if (g_paddle2_position.y > -3.5f) {
                g_paddle2_movement.y = -1.0f;

            }
        }
    }

}

const PongCollisions check_pong_collision(glm::vec3& g_pong_position, const int pong_num) {
    /* set the init pos for each pong */
    float init_pos_pong_x, init_pos_pong_y;
    if (pong_num == 1) {
        init_pos_pong_x = INIT_POS_PONG1.x;
        init_pos_pong_y = INIT_POS_PONG1.y;
    }
    else if (pong_num == 2) {
        init_pos_pong_x = INIT_POS_PONG2.x;
        init_pos_pong_y = INIT_POS_PONG2.y;
    }
    else {
        init_pos_pong_x = INIT_POS_PONG3.x;
        init_pos_pong_y = INIT_POS_PONG3.y;
    }
    
    if (abs(g_pong_position.x) > 5) {
        return SIDEWALLS; // should end game
    }
    /* top and bottom walls */
    if (abs(g_pong_position.y + init_pos_pong_y) > 3.5) {
        return BOUNCEWALLS;
    }

    /* paddle 1 collision check */
    float x_distance =  fabs(g_pong_position.x + init_pos_pong_x - 
                            (g_paddle1_position.x + INIT_POS_PADDLE1.x)) -
                        ((INIT_SCALE_PONGS.x + INIT_SCALE_PADDLES.x) / 2.0f);
    float y_distance =  fabs(g_pong_position.y + init_pos_pong_y - 
                            (g_paddle1_position.y + INIT_POS_PADDLE1.y)) -
                        ((INIT_SCALE_PONGS.y + INIT_SCALE_PADDLES.y) / 2.0f);
    if (x_distance < 0.0f && y_distance < 0.0f) {
        return PADDLECOLLISION;
    }

    /* paddle 2 collision check */
    x_distance = fabs(g_pong_position.x + init_pos_pong_x - 
                      (g_paddle2_position.x + INIT_POS_PADDLE2.x)) -
                  ((INIT_SCALE_PONGS.x + INIT_SCALE_PADDLES.x) / 2.0f);
    y_distance =  fabs(g_pong_position.y + init_pos_pong_y - 
                      (g_paddle2_position.y + INIT_POS_PADDLE2.y)) -
                  ((INIT_SCALE_PONGS.y + INIT_SCALE_PADDLES.y) / 2.0f);
    if (x_distance < 0.0f && y_distance < 0.0f) {
        return PADDLECOLLISION;
    }

    return NOCOLLISION;
}

void do_pong_collision(PongCollisions pong_collision, glm::vec3& pong_mvmt, glm::vec3& pong_pos) {
    if (pong_collision == SIDEWALLS) {
        if (pong_pos.x < 0) { display_message = "Orange cat wins!"; }
        else { display_message = "Black cat wins!"; }
        display_message_restart = "Press [r] to reset";
        ROUND_STARTED = false;
        return;
    }
    if (pong_collision == BOUNCEWALLS) { 
        pong_mvmt.y = -pong_mvmt.y;
        pong_pos.y = pong_pos.y + (pong_mvmt.y * 0.1);
        return; 
    }
    if (pong_collision == PADDLECOLLISION) {
        pong_mvmt.x = -pong_mvmt.x;
        pong_pos.x = pong_pos.x + (pong_mvmt.x * 0.1);
        return;
    }
}

void update() {
    // --- DELTA TIME CALCULATIONS --- //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

// PADDLE update() LOGIC
    // --- ACCUMULATOR LOGIC --- //
    g_paddle1_position += g_paddle1_movement * PADDLE_SPEED * delta_time;
    if (paddle2_is_npc) {
        if (abs(g_paddle2_position.y) >= 3.5f) {
            g_paddle2_movement.y = -g_paddle2_movement.y;
        }
    }
    g_paddle2_position += g_paddle2_movement * PADDLE_SPEED * delta_time;
    

    // --- TRANSLATION --- //
    g_paddle1_matrix = glm::mat4(1.0f);
    g_paddle1_matrix = glm::translate(g_paddle1_matrix, INIT_POS_PADDLE1);
    g_paddle1_matrix = glm::translate(g_paddle1_matrix, g_paddle1_position);

    g_paddle2_matrix = glm::mat4(1.0f);
    g_paddle2_matrix = glm::translate(g_paddle2_matrix, INIT_POS_PADDLE2);
    g_paddle2_matrix = glm::translate(g_paddle2_matrix, g_paddle2_position);


    // --- SCALING --- //
    g_paddle1_matrix = glm::scale(g_paddle1_matrix, INIT_SCALE_PADDLES);
    g_paddle2_matrix = glm::scale(g_paddle2_matrix, INIT_SCALE_PADDLES);


// pong1 update() LOGIC
    if (current_pong_count >= 1) {
        // --- ACCUMULATOR LOGIC --- //
        PongCollisions p1_collision = check_pong_collision(g_pong1_position, 1);
        do_pong_collision(p1_collision, g_pong1_movement, g_pong1_position);

        if (ROUND_STARTED) { /*only move when round starts*/
            g_pong1_position += g_pong1_movement * pong1_speed * delta_time;
        }
        // --- TRANSLATION --- //
        g_pong1_matrix = glm::mat4(1.0f);
        g_pong1_matrix = glm::translate(g_pong1_matrix, INIT_POS_PONG1);
        g_pong1_matrix = glm::translate(g_pong1_matrix, g_pong1_position);

        // --- SCALING --- //
        g_pong1_matrix = glm::scale(g_pong1_matrix, INIT_SCALE_PONGS);
    }
// pong2 update() LOGIC
    if (current_pong_count >= 2) {
        // --- ACCUMULATOR LOGIC --- //
        PongCollisions p2_collision = check_pong_collision(g_pong2_position, 2);
        do_pong_collision(p2_collision, g_pong2_movement, g_pong2_position);

        if (ROUND_STARTED) { /*only move when round starts*/
            g_pong2_position += g_pong2_movement * pong2_speed * delta_time;
        }

        // --- TRANSLATION --- //
        g_pong2_matrix = glm::mat4(1.0f);
        g_pong2_matrix = glm::translate(g_pong2_matrix, INIT_POS_PONG2);
        g_pong2_matrix = glm::translate(g_pong2_matrix, g_pong2_position);

        // --- SCALING --- //
        g_pong2_matrix = glm::scale(g_pong2_matrix, INIT_SCALE_PONGS);
    }

// pong3 update() LOGIC
    if (current_pong_count >= 3) {
        // --- ACCUMULATOR LOGIC --- //
        PongCollisions p3_collision = check_pong_collision(g_pong3_position, 3);
        do_pong_collision(p3_collision, g_pong3_movement, g_pong3_position);

        if (ROUND_STARTED) { /*only move when round starts*/
            g_pong3_position += g_pong3_movement * pong3_speed * delta_time;
        }

        // --- TRANSLATION --- //
        g_pong3_matrix = glm::mat4(1.0f);
        g_pong3_matrix = glm::translate(g_pong3_matrix, INIT_POS_PONG3);
        g_pong3_matrix = glm::translate(g_pong3_matrix, g_pong3_position);

        // --- SCALING --- //
        g_pong3_matrix = glm::scale(g_pong3_matrix, INIT_SCALE_PONGS);
    }
}


void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // clear background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };


    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_background_matrix, g_background_texture_id);
    draw_object(g_paddle1_matrix, g_paddle1_texture_id);
    draw_object(g_paddle2_matrix, g_paddle2_texture_id);
    if (current_pong_count >= 1) { draw_object(g_pong1_matrix, g_pong1_texture_id); }
    if (current_pong_count >= 2) { draw_object(g_pong2_matrix, g_pong2_texture_id); }
    if (current_pong_count >= 3) { draw_object(g_pong3_matrix, g_pong3_texture_id); }

    draw_text(&g_shader_program, g_font_texture_id, display_message, 0.3f, 0.05f,
        glm::vec3(-3.5f, 2.0f, 0.0f));
    draw_text(&g_shader_program, g_font_texture_id, display_message_restart, 0.3f, 0.05f,
        glm::vec3(-3.5f, 1.0f, 0.0f));

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { 
    SDL_Quit(); 
}


int main(int argc, char* argv[])
{
    srand(std::time(NULL));
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }

    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}