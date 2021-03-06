//BEGIN HEAD
//BEGIN DESCRIPTION

/* Defined Progress/Goals:
 * 
 * RECENT: 
 * -sound
 * GLint sampler_channel[4] float
 * float iSampleRate	image/buffer/sound The sound sample rate (typically 44100)
 * https://www.shadertoy.com/view/Msl3Rr
 * Sound shaders:
 * the mainSound() function returns a vec2 containing the left and right (stereo)
 * sound channel wave data. 
 * 
 * LONG TERM:
 * -Soundcloud
 */
//END 	DESCRIPTION

//BEGIN INCLUDES
#ifdef _WIN32
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>     // Just for the icon - easy to strip out
#include <SDL2/SDL_mixer.h>
#endif
#include <GL/glew.h>		//No clue how to do Cross-Platform
				//If anyone tries to build

#include "default_shaders.h"
#include <string.h>
//END 	INCLUDES

#define FRAG	"assets/shader/frag/5.frag"
#define VERT	"assets/shader/vert/2.vert"

//BEGIN GLOBALS
int ww=500;
int wh=281;

Mix_Music *Music 	= NULL;

char   Running 		= 1;
GLuint switch_counter	= 0;	// switches shading programs
GLuint argument_provided= 0;

GLfloat vertices[] = {
	 -1.0f,   -1.0f,
	  1.0f,   -1.0f,
	 -1.0f,    1.0f,
	  1.0f,    1.0f,
};

//3 from header, one optionally loaded
GLuint shading_program[4];

GLint  attrib_position;		// Vertex Shader Attribute

//Used Uniforms so far
GLint  uniform_gtime;
GLint  uniform_itime;		// Don't need both of them

GLint  uniform_res;		// Resolution
GLint  uniform_mouse;		//image/buffer	xy = current pixel coords (if LMB is down). zw = click pixel

//Not used so far
GLint  sampler_channel[4];	//image/buffer Time for channel (if video or sound), in seconds
GLint  uniform_cres;
GLint  uniform_ctime;
GLint  uniform_date;
GLint  uniform_srate;
//END  GLOBALS

// loads custom Vertex- and Fragment Shader from File
GLuint custom_shaders		(const char *	, const char *);

// uses default Vertex-Shader and loads a ST-Shader
GLuint shadertoy_shader		(const char *);

// loads a shader from file and returns the compiled shader
GLuint GetShader		(GLenum 	, const char *);

// loads default Vertex- and Fragment Shader if no Arguments are provided
// or shaders to load failed (TBD)
GLuint default_shaders		(GLuint);

// Compiles def VS
GLuint default_vertex		(void);

GLuint compile_shader		(GLenum type, GLsizei , const char **);
GLuint program_check		(GLuint);

void   query_shadertoy_old_vars	(GLuint);
void   query_vars		(GLuint);

void 	shader_switch		(void);
float  fTime			(void);
void   init_glew		(void);
const char  *read_file		(const char *);
//END 	HEAD

//BEGIN MAIN
int main(int argc, const char *argv[])
{
	//BEGIN INIT
// 	(void)argv; // not needed yet


	if (argc==2){
		argument_provided=1;
		SDL_Log("%s",argv[1]);
		//for now I will assume type shadertoy is provided
	} else {
		SDL_Log("No argument provided.");
	}
	//BEGIN INIT SDL2
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *Window = SDL_CreateWindow("0h - TESTING AUDIO",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		ww, wh,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |SDL_WINDOW_RESIZABLE);

		//BEGIN ICON
		SDL_Surface *icon;
		icon=IMG_Load("./assets/gfx/icon.png");
		SDL_SetWindowIcon(Window, icon);
		SDL_FreeSurface(icon);
		//END 	ICON

	Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
	Music=Mix_LoadMUS("assets/snd/The.madpix.project_-_Wish_You_Were_Here.mp3");
	Mix_PlayMusic(Music, -1);
	
	SDL_GLContext glContext = SDL_GL_CreateContext(Window);
	//END 	INIT SDL2

	init_glew();


	//BEGIN SHADER INIT

	SDL_Log("Trying to build default shaders");
	for (int i=0; i<3; i++){
		shading_program[i] = default_shaders(i);
		SDL_Log("i: %d", i);
	}
	if (shading_program[0] == 0){
		Running = 0;
		if (glGetError()!=0)
			SDL_Log("glError: %#08x\n", glGetError());
	} else {
		switch(argument_provided)
		{
			case 0:
				glUseProgram(shading_program[0]);
				query_vars(0);
				query_shadertoy_old_vars(0);
				SDL_Log("Using default shader");
				break;
			case 1:
				SDL_Log("Trying to build shadertoy shader");
				shading_program[3] = shadertoy_shader(argv[1]);
				glUseProgram(shading_program[3]);
				SDL_Log("Using shading program %d\n", shading_program[3]);
				query_vars(3);
				query_shadertoy_old_vars(3);
				switch_counter=4;
				break;
			case 2:
				shading_program[3] = custom_shaders(VERT, FRAG);

				SDL_Log("Trying to build custom shaders");
				break;
			default:
				//some statements to execute when default;
				break;
		}
	}

	glEnableVertexAttribArray	(attrib_position);
	glVertexAttribPointer		(attrib_position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glUniform3f			(uniform_res, ww, wh, 0.0f);
	//END 	SHADER INIT
	SDL_Point mouse;
	char MBL_CLICK=0;
	SDL_Event event;

	//END	INIT

	//BEGIN MAINLOOP
	
	while (Running){
		//BEGIN EVENTS
		while ( SDL_PollEvent(&event) ){
			SDL_GetMouseState(&mouse.x, &mouse.y);
			if (event.type == SDL_QUIT)
				Running = 0;
			if(event.type == SDL_WINDOWEVENT){
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
					ww=event.window.data1;
					wh=event.window.data2;
					glViewport (0, 0, ww, wh);
					glUniform3f(uniform_res, ww, wh, 0.0f);
				}
			}
			if(event.type == SDL_MOUSEMOTION){
				;
			}
			if(event.type == SDL_MOUSEBUTTONDOWN){
				if(event.button.button == SDL_BUTTON_RIGHT){
					;
				}
				if(event.button.button == SDL_BUTTON_MIDDLE){
					shader_switch();
				}
				if( event.button.button==SDL_BUTTON_LEFT){
					MBL_CLICK=1;
				}
			}
			if(event.type == SDL_MOUSEBUTTONUP){
				if( event.button.button==SDL_BUTTON_LEFT){
					MBL_CLICK=0;
				}	
			}
			if(event.type == SDL_KEYDOWN ){
				switch(event.key.keysym.sym ){
					case SDLK_ESCAPE:
						Running = 0;
						break;
						
					case SDLK_r:
					case SDLK_BACKSPACE:
						break;
						
					case SDLK_p:	
					case SDLK_SPACE:
						break;
						
					default:
						break;
				}
			}

		}
		//END 	EVENTS

		//BEGIN UPDATE
		if (MBL_CLICK==1)
			glUniform4f(uniform_mouse, mouse.x, mouse.y, 0, 0 );
		switch(switch_counter)
		{
			case 1:
				//some statements to execute when 1
				break;
			case 2:
				//some statements to execute when 2
				break;
			case 3:
				//some statements to execute when 3
				break;
			default:
				//some statements to execute when default;
				break;
		}
		glUniform1f(uniform_gtime, fTime());
// 		
		//END UPDATE
		//BEGIN RENDERING
		glClear(GL_COLOR_BUFFER_BIT);
// 		if (argument_provided<2)
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
// 		else
// 			glRectf(-1.0, -1.0, 1.0, 1.0);


		SDL_GL_SwapWindow(Window);
		//END 	RENDERING
	}
	//END 	MAINLOOP

	//BEGIN EXIT
	for (int i=0; i<3; i++){
		if (glIsProgram(shading_program[i]))
			glDeleteProgram(shading_program[i]);
	}
	Mix_FreeMusic(Music);
	Mix_CloseAudio();
	SDL_GL_DeleteContext(glContext);
	SDL_Quit();

	return EXIT_SUCCESS;
	//END 	EXIT
}
//END	MAIN

//BEGIN FUNCTIONS

//BEGIN GPU PROGRAM CREATION
//Build custom program from 2 files
GLuint custom_shaders(const char *vsPath, const char *fsPath)
{
	GLuint vertexShader;
	GLuint fragmentShader;

	vertexShader   = GetShader(GL_VERTEX_SHADER,   vsPath);
	fragmentShader = GetShader(GL_FRAGMENT_SHADER, fsPath);

	shading_program[3] = glCreateProgram();

	glAttachShader(shading_program[3], vertexShader);
	glAttachShader(shading_program[3], fragmentShader);

	glLinkProgram(shading_program[3]);

	//Error Checking
	GLuint status;
	status=program_check(shading_program[3]);
	if (status==GL_FALSE)
		return 0;
	return shading_program[3];

}

GLuint shadertoy_shader(const char *fsPath)
{
	GLuint vtx;
	vtx = default_vertex();
	if (vtx==0)
		return 0;

	GLuint frag;
	const char *sources[4];
	sources[0] = common_shader_header;
	sources[1] = fragment_shader_header;
	sources[2] = read_file(fsPath);
// 	const char *shaderSource=read_file(filename);
	sources[3] = fragment_shader_footer;
	frag = compile_shader(GL_FRAGMENT_SHADER, 4, sources);

// 	GLuint fragmentShader;
// 	fragmentShader = GetShader(GL_FRAGMENT_SHADER, fsPath);
	
	shading_program[3] = glCreateProgram();
	
	glAttachShader(shading_program[3], vtx);
	glAttachShader(shading_program[3], frag);
	
	glLinkProgram(shading_program[3]);
	
	//Error Checking
	GLuint status;
	status=program_check(shading_program[3]);
	if (status==GL_FALSE)
		return 0;
	return shading_program[3];
	
}

GLuint GetShader(GLenum eShaderType, const char *filename)
{

	const char *shaderSource=read_file(filename);
	GLuint shader = compile_shader(eShaderType, 1, &shaderSource);
	return shader;

}

GLuint compile_shader(GLenum type, GLsizei nsources, const char **sources)
{

	GLuint  shader;
	GLint   success, len;
	GLsizei i, srclens[nsources];
	
	for (i = 0; i < nsources; ++i)
		srclens[i] = (GLsizei)strlen(sources[i]);
	
	shader = glCreateShader(type);
	glShaderSource(shader, nsources, sources, srclens);
	glCompileShader(shader);
	
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		if (len > 1) {
			char *log;
			log = malloc(len);
			glGetShaderInfoLog(shader, len, NULL, log);
			fprintf(stderr, "%s\n\n", log);
			free(log);
		}
		SDL_Log("Error compiling shader.\n");
	}
	SDL_Log("shader: %u",shader);
	return shader;
}

//if no arguments provided use vertex and fragment shader from header
GLuint default_shaders(GLuint choice)
{
	SDL_Log("choice def: %d", choice);
	GLuint vtx;
	vtx = default_vertex();
	if (vtx==0)
		return 0;
	
	GLuint frag;
	const char *sources[4];
	sources[0] = common_shader_header;
	sources[1] = fragment_shader_header;
	switch(choice)
	{
		case 0:
			sources[2] = default_fragment_shader_0;
			break;
		case 1:
			sources[2] = default_fragment_shader_1;
			break;
		case 2:
			sources[2] = default_fragment_shader;
			break;
		default:
			//some statements to execute when default;
			break;
	}
	
	sources[3] = fragment_shader_footer;
	frag = compile_shader(GL_FRAGMENT_SHADER, 4, sources);

	shading_program[choice] = glCreateProgram();
	glAttachShader(shading_program[choice], vtx);
	glAttachShader(shading_program[choice], frag);
	glLinkProgram(shading_program[choice]);

	//Error Checking
	GLuint status;
	status=program_check(shading_program[choice]);
	if (status==GL_FALSE)
		return 0;

	return shading_program[choice];
}


GLuint default_vertex(void)
{
	GLuint vtx;
	const char *sources[2];
	sources[0] = common_shader_header;
	sources[1] = vertex_shader_body;
	vtx = compile_shader(GL_VERTEX_SHADER, 2, sources);
	
	return vtx;
	
}

GLuint program_check(GLuint program)
{
	//Error Checking
	GLint status;
	glValidateProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status){
		GLint len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		if (len > 1){
			char *log;
			log = malloc(len);
			glGetProgramInfoLog(program, len, &len, log);
			SDL_LogError(SDL_LOG_CATEGORY_ERROR,"%s\n\n", log);
// 			fprintf(stderr, "%s\n\n", log);
			free(log);
		}
		SDL_Log("Error linking shader default program.\n");
		return GL_FALSE;
	}
	return GL_TRUE;
}
//END 	GPU PROGRAM CREATION

void query_shadertoy_old_vars(GLuint choice)
{
	
	
// vec3	 iResolution	image/buffer		The viewport resolution (z is pixel aspect ratio, usually 1.0)
// float iTime		image/sound/buffer	Current time in seconds
// float iTimeDelta	image/buffer		Time it takes to render a frame, in seconds
// int	 iFrame		image/buffer		Current frame
// float iFrameRate	image/buffer		Number of frames rendered per second
// float iChannelTime[4]image/buffer		Time for channel (if video or sound), in seconds
// vec3	 iChannelResolution[4]	image/buffer/sound	Input texture resolution for each channel
// vec4	 iMouse		image/buffer		xy = current pixel coords (if LMB is down). zw = click pixel
// sampler2D		iChannel{i}		image/buffer/sound	Sampler for input textures i
// vec4	 iDate		image/buffer/sound	Year, month, day, time in seconds in .xyzw
// float iSampleRate	image/buffer/sound	The sound sample rate (typically 44100)

	SDL_Log("choice old vars: %d", choice);
	sampler_channel[0] = glGetUniformLocation(shading_program[choice], "iChannel0");
	sampler_channel[1] = glGetUniformLocation(shading_program[choice], "iChannel1");
	sampler_channel[2] = glGetUniformLocation(shading_program[choice], "iChannel2");
	sampler_channel[3] = glGetUniformLocation(shading_program[choice], "iChannel3");
	
	uniform_cres  = glGetUniformLocation(shading_program[choice], "iChannelResolution");
	uniform_ctime = glGetUniformLocation(shading_program[choice], "iChannelTime");
// 	doing this on demand in function query_vars now
// 	uniform_date  = glGetUniformLocation(shading_program, "iDate");
// 	uniform_gtime = glGetUniformLocation(shading_program, "iGlobalTime");
	uniform_itime = glGetUniformLocation(shading_program[choice], "iTime");
// 	uniform_mouse = glGetUniformLocation(shading_program[choice], "iMouse");
	uniform_res   = glGetUniformLocation(shading_program[choice], "iResolution");
	uniform_srate = glGetUniformLocation(shading_program[choice], "iSampleRate");
}

void query_vars(GLuint choice)
{
// 	SDL_Log("choice query vars: %d", choice);
	// query gpu-program for attributes and uniforms
	// https://www.khronos.org/opengl/wiki/Program_Introspection
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniform.xhtml
	//glGetActiveUniform(	GLuint program,
	// 			GLuint index,
	// 			GLsizei bufSize,
	// 			GLsizei *length,
	// 			GLint *size,
	// 			GLenum *type,
	// 			GLchar *name);
// 	GLsizei count;
// 	glGetActiveUniformsiv( shading_program​, GLsizei uniformCount​, const GLuint *uniformIndices​, GLenum pname​, GLint *params​ );
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniform.xhtml
	
	GLint i;
	GLint count;
	
	GLint size; // size of the variable
	GLenum type; // type of the variable (float, vec3 or mat4, etc)
	
	const GLsizei bufSize = 16; // maximum name length
	GLchar name[bufSize]; // variable name in GLSL
	GLsizei length; // name length
	
	//Attributes
	glGetProgramiv(shading_program[choice], GL_ACTIVE_ATTRIBUTES, &count);
// 	printf("Active Attributes: %d\n", count);
	
	for (i = 0; i < count; i++){
		glGetActiveAttrib(shading_program[choice], (GLuint)i, bufSize, &length, &size, &type, name);
		if (!strcmp("iPosition",name)){
			attrib_position = i;
// 			SDL_Log("setting new atrribute position");
		}
// 		SDL_Log("Attribute #%d Type: %u Name: %s\n", i, type, name);
	}
	
	//Uniforms
	glGetProgramiv(shading_program[choice], GL_ACTIVE_UNIFORMS, &count);
// 	printf("Active Uniforms: %d\n", count);
	
	//shadertoy old api and api 2018
	// fricking time iTime and iGlobalTime;
	
	// 11/07/2017 - Release 0.9.3
	// - Layout/style fixes
	// - Shadertoy competition 2017 is here
	// - Can use iTime instead of iGlobalTime, shorter shaders! iGlobalTime will be obsoleted
	// - Added global char counter so it is easy to see how long is your shader with all passes
	
	char global_time1[]="iTime";
	char global_time2[]="iGlobalTime";
	for (i = 0; i < count; i++){
		glGetActiveUniform(shading_program[choice], (GLuint)i, bufSize, &length, &size, &type, name);
		
		SDL_Log("Uniform #%d Type: %u Name: %s\n", i, type, name);
		if (!strcmp(global_time1,name)||!strcmp(global_time2,name)){
// 			SDL_Log("we got a hit");
			uniform_gtime = i;
		}
		if (!strcmp("iMouse",name))
			uniform_mouse = i;
		
		if (!strcmp("iChannel0",name)){
			sampler_channel[0] = i;
			SDL_Log("found sampler_channel[0]");
		}
	}
	//https://stackoverflow.com/questions/440144/in-opengl-is-there-a-way-to-get-a-list-of-all-uniforms-attribs-used-by-a-shade
	
	//void glUniform1f(GLint location​, GLfloat v0​);
}
//BEGIN MISC
float fTime(void)
{

	static Uint64 start 	 = 0;
	static Uint64 frequency  = 0;
	
	if (start==0){
		start		=	SDL_GetPerformanceCounter();
		frequency	=	SDL_GetPerformanceFrequency();
		return 0.0f;
	}
	
	Uint64 counter    = 0;
	       counter    = SDL_GetPerformanceCounter();
	Uint64 accumulate = counter - start;
	return   (float)accumulate / (float)frequency;

}

void init_glew(void)
{
	GLenum status;
	status = glewInit();

	if (status != GLEW_OK){
		SDL_Log("glewInit error: %s\n", glewGetErrorString (status));
		Running = 0;
	}

	SDL_Log("\nGL_VERSION   : %s\nGL_VENDOR    : %s\nGL_RENDERER  : %s\n"
		 "GLEW_VERSION : %s\nGLSL VERSION : %s\n",
	  glGetString (GL_VERSION), glGetString (GL_VENDOR),
		 glGetString (GL_RENDERER), glewGetString (GLEW_VERSION),
		 glGetString (GL_SHADING_LANGUAGE_VERSION));

	int maj;
	int min;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,   &maj);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,   &min);
	SDL_Log("Using OpenGL %d.%d", maj, min);
	
	if (!GLEW_VERSION_2_0){
		SDL_Log("At least OpenGL 2.0 with GLSL 1.10 required.");
		Running = 0;
	}
}

const char * read_file(const char *filename)
{
	long length = 0;
	char *result = NULL;
	FILE *file = fopen(filename, "r");
	if(file) {
		int status = fseek(file, 0, SEEK_END);
		if(status != 0) {
			fclose(file);
			return NULL;
		}
		length = ftell(file);
		status = fseek(file, 0, SEEK_SET);
		if(status != 0) {
			fclose(file);
			return NULL;
		}
		result = malloc((length+1) * sizeof(char));
		if(result) {
			size_t actual_length = fread(result, sizeof(char), length , file);
			result[actual_length++] = '\0';
		} 
		fclose(file);
		return result;
	}
	SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Couldn't read %s", filename);
	return NULL;
}

void shader_switch(void)
{
	switch_counter++;
	if (switch_counter>(2+argument_provided))
		switch_counter=0;
	SDL_Log("switch_counter: %d", switch_counter);
	glUseProgram(shading_program[switch_counter]);
	query_vars(switch_counter);
	query_shadertoy_old_vars(switch_counter);
	glEnableVertexAttribArray	(attrib_position);
	glVertexAttribPointer		(attrib_position, 2, GL_FLOAT, GL_FALSE, 0, vertices);

	glViewport (0, 0, ww, wh);
	glUniform3f(uniform_res, ww, wh, 0.0f);
}
//END 	MISC

//END FUNCTIONS
