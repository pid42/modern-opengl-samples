#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <libpng/png.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define W 1024
#define H 768

enum STATE {
    STATE_INIT,
    STATE_RUNNING,
    STATE_EXIT
};

GLfloat geometry[] = {
    -0.5f,  0.5f, 0.f,
    -0.5f, -0.5f, 0.f,
     0.5f, -0.5f, 0.f,
     0.5f,  0.5f, 0.f
};

GLuint indexes[] = {
    0, 1, 2,
    2, 3, 0
};

GLfloat text_coords[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};

void load_png_file(const char *path, unsigned char **image_data, unsigned int *image_width, unsigned int *image_height, unsigned int *bpp)
{
    FILE *fp = fopen("sao_paulo.png", "rb");
    if(!fp) {
        printf("cannot open file %s\n", path);
        exit(EXIT_FAILURE);
    }

    // based on code from http://www.libpng.org/pub/png/libpng-1.0.3-manual.html
    png_structp png_ptr =
            png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL/*(png_voidp)user_error_ptr*/,
                                   NULL/*user_error_fn*/, NULL/*user_warning_fn*/);

    if (!png_ptr) exit(EXIT_FAILURE);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        exit(EXIT_FAILURE);
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_ptr->jmpbuf))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        exit(EXIT_FAILURE);;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);
    *image_width = info_ptr->width;
    *image_height = info_ptr->height;
    *bpp = info_ptr->channels;

    *image_data = malloc((*image_width) * (*image_height) * (*bpp));
    png_bytep row_pointer[*image_height];
    unsigned char *p;
    p = *image_data;
    for(int i = 0 ; i < *image_height ; i++)
    {
        row_pointer[i] = p;
        p += (*image_width) * (*bpp);
    }

    png_read_image(png_ptr, row_pointer);
    png_read_end(png_ptr, end_info);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    free(png_ptr);
    free(info_ptr);
    fclose(fp);
}

void load_content(const char *path, char **content)
{
    FILE *f = fopen(path, "r");
    if(!f)
    {
        fprintf(stderr, "failed to open file %s: %s\n", path, strerror(errno));
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

    SDL_Window *main_window = SDL_CreateWindow("glDrawElements", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_OPENGL);
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
    compile_shader(vs_shader, "glsl/texture.vert");
    GLuint fs_shader = glCreateShader(GL_FRAGMENT_SHADER);
    compile_shader(fs_shader, "glsl/texture.frag");

    GLuint program = glCreateProgram();
    glAttachShader(program, vs_shader);
    glAttachShader(program, fs_shader);
    glLinkProgram(program);
    GLint linkStatus = 0;
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

    /* here, besides the vbo for the vertices, there is an array of indexes that will
       define which vertices compose each geometry face */
    GLuint ibo = 0;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);

    /* now we bind the buffers to the shader attributes */
    GLuint v_pos = glGetAttribLocation(program, "v_pos");
    glEnableVertexAttribArray(v_pos);
    glVertexAttribPointer(v_pos, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* texture loading */
    unsigned char *image_data=0;
    unsigned int image_width=0, image_height=0, bpp=0;
    load_png_file("sao_paulo.png", &image_data, &image_width, &image_height, &bpp);
    /* here we have a buffer for the texture coordinates too.
     * im using a separate buffer here to show that we can use multiple buffers for the same bind point. the vao will keep
     * record of all of them. In real code, you should pack all vertex attributes in the same buffer, it will be faster
     */
    /* texture coords buffer */
    GLuint vbo_text_coords;
    glGenBuffers(1, &vbo_text_coords);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_text_coords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(text_coords), text_coords, GL_STATIC_DRAW);

    GLuint tex_coord = glGetAttribLocation(program, "tex_coord");
    glEnableVertexAttribArray(tex_coord);
    glVertexAttribPointer(tex_coord, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* texture configuration */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    free(image_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);


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
        /* instead of draw arrays, we use draw elements, and pass the index count for the faces */
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        SDL_GL_SwapWindow(main_window);
    }

    /* destroy */
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(main_window);

    return 0;
}

