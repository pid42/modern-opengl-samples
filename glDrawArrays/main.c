#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>

#define W 1024
#define H 768

enum STATE {
    STATE_INIT,
    STATE_RUNNING,
    STATE_EXIT
};

static
GLfloat geometry[] = {
     0.0f,  0.5f, 0.f,
    -0.5f, -0.5f, 0.f,
     0.5f, -0.5f, 0.f,
};

void load_content(const char *path, char **content)
{
    FILE *f = fopen(path, "r");
    if(!f)
    {
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }
    fseek(f, 0L, SEEK_END);
    long length = ftell(f);
    fseek(f, 0L, SEEK_SET);
    *content = calloc(length+1, sizeof(char));
    char *p  = *content;
    while(!feof(f))
    {
        size_t nread = fread(p, sizeof(char), length, f);
        p += nread;
    }
    fclose(f);
}

void compile_shader(GLuint shader, const char *path)
{
    char *content;
    load_content(path, &content);
    const char *sources[] = { content };

    glShaderSource(shader, 1, sources, NULL/*null terminated strings*/);
    free(content);
    glCompileShader(shader);
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE)
    {
         GLint logLength;
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
         GLchar *infoLog = calloc(logLength+1, sizeof(char));
         glGetShaderInfoLog(shader, logLength, NULL, infoLog);
         fprintf(stderr, "failed to compile %s: %s\n", path, infoLog);
         free(infoLog);
         exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{

    enum STATE state = STATE_INIT;

    /* initialize graphics */
    SDL_Init(SDL_INIT_EVERYTHING);

    /* prepare opengl core profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window *main_window = SDL_CreateWindow("glDrawArrays", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(main_window);
    glewExperimental=GL_TRUE;
    glewInit();

    printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* prepare default framebuffer */
    glClearColor(0,0,1,1);

    /* prepare shaders */
    GLuint vs_shader = glCreateShader(GL_VERTEX_SHADER);
    compile_shader(vs_shader, "glsl/pass.vert");
    GLuint fs_shader = glCreateShader(GL_FRAGMENT_SHADER);
    compile_shader(fs_shader, "glsl/pass.frag");

    GLuint program = glCreateProgram();
    glAttachShader(program, vs_shader);
    glAttachShader(program, fs_shader);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus );
    if ( linkStatus != GL_TRUE )
    {
        GLint logLength;
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength );
        GLchar *infoLog = calloc(logLength+1, sizeof(char));
        glGetProgramInfoLog( program, logLength, NULL, infoLog );
        fprintf(stderr, "Failed to link shader glProgram: %s\n", infoLog);
        free(infoLog);
        exit(EXIT_FAILURE);
    }
    glUseProgram(program);

    /* prepare geometry */
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);


    GLuint v_pos = glGetAttribLocation(program, "v_pos");
    glEnableVertexAttribArray(v_pos);
    glVertexAttribPointer(v_pos, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));

    GLuint color = glGetUniformLocation(program, "color");
    glUniform4f(color, 1.f, 0.f, 0.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* event loop */
    state = STATE_RUNNING;
    while(state == STATE_RUNNING)
    {
        /* process input */
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type)
            {
                case SDL_QUIT:
                    state = STATE_EXIT;
                    break;
            }
        }

        /* draw */
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3 /* one triangle */);
        SDL_GL_SwapWindow(main_window);
    }

    /* destroy */
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(main_window);

    return 0;
}
