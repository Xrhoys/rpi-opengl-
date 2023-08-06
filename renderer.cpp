#include "renderer.h"

global render_group debugRenderGroup;
global render_group uiRenderGroup;

global font_engine g_fontEngine;

internal GLuint shaderProgram;
internal GLuint shaderProgramVideo;

internal GLuint VAO;
internal GLuint g_glBuffers[BUFFER_COUNT];

internal v4 clearBackground = RGBToFloat(LIGHTGRAY);

// TODO(Ecy): to remove this global
internal GLuint g_bgTexture[2]; 
internal GLuint g_emptyTexture;

vertex vertices[] = 
{
	Vertex(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f,  1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
};

u32 indices[] = 
{
	0, 1, 3,   // first triangle
	1, 2, 3,   // second triangle
};

internal void
LoadDataToTexture(GLuint textureId, u32 width, u32 height, void *data)
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
				 GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

internal void
LoadYUVToTexture(GLuint textureId, u32 width, u32 height, GLenum format, void *data)
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

internal void
InitRenderer()
{
    {
        const char *vertexShaderSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
            "layout (location = 0) in vec3 aPos;\n"
			"layout (location = 1) in vec2 aTexCoord;\n"
			"layout (location = 2) in vec4 aColor; \n"
			"out vec2 TexCoord;\n"
			"out vec4 Color;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos, 1.0);\n"
			"   TexCoord = aTexCoord;\n"
			"   Color = aColor;\n"
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
			"in vec4 Color;\n"
            "out vec4 FragColor;\n"
			"uniform sampler2D tex;\n"
            "void main()\n"
            "{\n"
			"    FragColor = texture(tex, TexCoord) * Color;\n"
			//"    FragColor = vec4(1.0f, 0.5f, 1.0f, 1.0f);\n"
            "}\0";
        
        const char *fragmentShaderVideoSource = 
			"#version 310 es\n"
			"precision mediump float;\n"
			"in vec2 TexCoord;\n"
			"in vec4 Color;\n"
            "out vec4 FragColor;\n"
			"uniform sampler2D texY;\n"
            "uniform sampler2D texUV;\n"
            "void main()\n"
            "{\n"
            "    float r, g, b, y, u, v;\n"
			"    y = texture(texY, TexCoord).r;\n"
			"    u = texture(texUV, TexCoord).r - 0.5;\n"
			"    v = texture(texUV, TexCoord).a - 0.5;\n"
			"    r = y + 1.13983*v;\n"
			"    g = y - 0.39465*u - 0.58060*v;\n"
			"    b = y + 2.03211*u;\n"
			"    FragColor = vec4(r, g, b, 1.0f);\n"
            "}\0";
        
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
		
        // Check ShaderCompile success
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
		GLuint fragmentShaderVideo;
        fragmentShaderVideo = glCreateShader(GL_FRAGMENT_SHADER);
		
        glShaderSource(fragmentShaderVideo, 1, &fragmentShaderVideoSource, NULL);
        glCompileShader(fragmentShaderVideo);
		
        // Check ShaderCompile success
		glGetShaderiv(fragmentShaderVideo, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(fragmentShaderVideo, 512, NULL, infoLog);
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
		
		shaderProgramVideo = glCreateProgram();
        
		glAttachShader(shaderProgramVideo, vertexShader);
        glAttachShader(shaderProgramVideo, fragmentShaderVideo);
        glLinkProgram(shaderProgramVideo);
		
        // Check ShaderCompile success
		glGetProgramiv(shaderProgramVideo, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(shaderProgramVideo, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		}
		
        //glDeleteShader(vertexShader);
        //glDeleteShader(fragmentShader);
		
        glGenVertexArrays(1, &VAO);
		glGenBuffers(BUFFER_COUNT, g_glBuffers);
		
        glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[BG_VERTEX_ARRAY]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 4, vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[BG_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, indices, GL_STATIC_DRAW);
		
		// TODO(Ecy): remove hard coded values later
		glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[FONT_VERTEX_ARRAY]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[FONT_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);
		
		glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[UI_VERTEX_ARRAY]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[UI_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);
		
		glGenTextures(2, g_bgTexture);
		glGenTextures(1, &g_emptyTexture);
		
		glBindTexture(GL_TEXTURE_2D, g_emptyTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		u8 generateTexture[30000];
		for(u32 index = 0;
			index < sizeof(generateTexture);
			++index)
		{
			generateTexture[index] = 255;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 100, 100, 0, GL_RGB, GL_UNSIGNED_BYTE, 
					 generateTexture);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		//glEnable(GL_DEPTH_TEST);
    }
}

internal void
UpdateBackgroundTexture()
{
}

void
UseVertexAttrib()
{
	glBindVertexArray(VAO);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texCoord));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, color));
}

internal void
Render()
{
	{
		//glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		
		glClearColor(clearBackground.r, clearBackground.g, clearBackground.b, clearBackground.a);
		glClear(GL_COLOR_BUFFER_BIT);

		{
			glUseProgram(shaderProgramVideo);
			glBindVertexArray(VAO);
			
			glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[BG_VERTEX_ARRAY]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[BG_INDEX_ARRAY]);
			
			UseVertexAttrib();
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_bgTexture[0]);
			glUniform1i(glGetUniformLocation(shaderProgramVideo, "texY"), 0);
			
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, g_bgTexture[1]);
			glUniform1i(glGetUniformLocation(shaderProgramVideo, "texUV"), 1);
			
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

#if 0
		// NOTE(Ecy): render tree-node UI structure
		// NOTE(Ecy): there is a specific API call order here:
		// 1. Bind data buffers and/or update buffer data
		// 2. Bind vertex array strcut
		// 3. Enable attributes
		// 4. Draw
		{
			glUseProgram(shaderProgram);
			
			glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[UI_VERTEX_ARRAY]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[UI_INDEX_ARRAY]);
			
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex) * uiRenderGroup.vertexCount, uiRenderGroup.vertices);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(u32) * uiRenderGroup.indexCount, uiRenderGroup.indices);
			
			UseVertexAttrib();
			
			glBindTexture(GL_TEXTURE_2D, g_emptyTexture);
			//glEnable(GL_BLEND);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
			
			glDrawElements(GL_TRIANGLES, uiRenderGroup.indexCount, GL_UNSIGNED_INT, 0);
		}
		
		// NOTE(Ecy): slow, experimental font engine debugging layer
		{
			glUseProgram(shaderProgram);
			
			glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[FONT_VERTEX_ARRAY]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[FONT_INDEX_ARRAY]);
			
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex) * debugRenderGroup.vertexCount, debugRenderGroup.vertices);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(u32) * debugRenderGroup.indexCount, debugRenderGroup.indices);
			
			UseVertexAttrib();
				
			glBindTexture(GL_TEXTURE_2D, g_fontEngine.textureId);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
			
			glDrawElements(GL_TRIANGLES, debugRenderGroup.indexCount, GL_UNSIGNED_INT, 0);
		}
#endif

	}
}
