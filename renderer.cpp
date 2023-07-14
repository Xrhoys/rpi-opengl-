#include "renderer.h"

global render_group uiRenderGroup;

GLuint shaderProgram;

GLuint VA0, VB0, EB0, FontV0, FontB0;
GLuint texture;

float vertices[] = 
{
	-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom left
	1.0f, -1.0f, 0.0f,  1.0f, 1.0f,// bottom right
	1.0f,  1.0f, 0.0f,  1.0f, 0.0f,// top right
	-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, // top left 
};
unsigned int indices[] = 
{
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
};

inline char* 
getErrorStr(EGLint code)
{
	switch(code)
	{
		case EGL_SUCCESS: return "No error";
		case EGL_NOT_INITIALIZED: return "EGL not initialized or failed to initialize";
		case EGL_BAD_ACCESS: return "Resource inaccessible";
		case EGL_BAD_ALLOC: return "Cannot allocate resources";
		case EGL_BAD_ATTRIBUTE: return "Unrecognized attribute or attribute value";
		case EGL_BAD_CONTEXT: return "Invalid EGL context";
		case EGL_BAD_CONFIG: return "Invalid EGL frame buffer configuration";
		case EGL_BAD_CURRENT_SURFACE: return "Current surface is no longer valid";
		case EGL_BAD_DISPLAY: return "Invalid EGL display";
		case EGL_BAD_SURFACE: return "Invalid surface";
		case EGL_BAD_MATCH: return "Inconsistent arguments";
		case EGL_BAD_PARAMETER: return "Invalid argument";
		case EGL_BAD_NATIVE_PIXMAP: return "Invalid native pixmap";
		case EGL_BAD_NATIVE_WINDOW: return "Invalid native window";
		case EGL_CONTEXT_LOST: return "Context lost";
		default: return "";
	}
}

 internal void
InitRenderer(app_state *appContext)
{
    {
        const char *vertexShaderSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
            "layout (location = 0) in vec3 aPos;\n"
			"layout (location = 1) in vec2 aTexCoord;\n"
			"out vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos, 1.0);\n"
			"   TexCoord = aTexCoord;\n"
            "}\0";
		
        GLuint vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
		
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
		
        // Check ShaderCompile success
		int  success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        const char *fragmentShaderSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
			"in vec2 TexCoord;\n"
            "out vec4 FragColor;\n"
			"uniform sampler2D tex;\n"
            "void main()\n"
            "{\n"
            "    FragColor = texture(tex, TexCoord);\n"
			//"    FragColor = vec4(1.0f, 0.5f, 1.0f, 1.0f);\n"
            "}\0";
        
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
		
        // Check ShaderCompile success
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        shaderProgram = glCreateProgram();
		
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
		
        // Check ShaderCompile success
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        //glDeleteShader(vertexShader);
        //glDeleteShader(fragmentShader);
		
        glGenVertexArrays(1, &VA0);
		glGenBuffers(1, &VB0);
		glGenBuffers(1, &EB0);
		glGenBuffers(1, &FontB0);
		glGenBuffers(1, &FontV0);
		
        glBindBuffer(GL_ARRAY_BUFFER, VB0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		// TODO(Ecy): remove hard coded values later
		glBindBuffer(GL_ARRAY_BUFFER, FontV0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FontB0);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);
		
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		//glEnable(GL_DEPTH_TEST);
    }
	
	uiRenderGroup.vertices = (vertex*)malloc(10 * 1024 * 1024);
	uiRenderGroup.indices  = (u32*)malloc(10 * 1024 * 1024);
}

internal void
Render()
{
	// NOTE(Ecy): Replace with suggested frame timing
	//if(Win32GetLastElapsed() > 1.0f / REFRESH_RATE)
	{
		//glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(shaderProgram);
#if 0
		{
			glBindVertexArray(VA0);
			
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),  (void*)(3 * sizeof(float)));
			
			glBindTexture(GL_TEXTURE_2D, texture);
			glBindBuffer(GL_ARRAY_BUFFER, VB0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EB0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
#endif
		
		// NOTE(Ecy): slow, experimental font engine debugging layer
		{
			glBindVertexArray(VA0);
			
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),  (void*)(3 * sizeof(float)));
			
			glBindTexture(GL_TEXTURE_2D, 2);
			
			glBindBuffer(GL_ARRAY_BUFFER, FontV0);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex) * uiRenderGroup.vertexCount, uiRenderGroup.vertices);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FontB0);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(u32) * uiRenderGroup.indexCount, uiRenderGroup.indices);
			
			glDrawElements(GL_TRIANGLES, uiRenderGroup.indexCount, GL_UNSIGNED_INT, 0);
		}
	}
}