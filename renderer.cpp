#include "renderer.h"

global render_group debugRenderGroup;
global render_group uiRenderGroup;

global font_engine g_fontEngine;

internal GLuint shaderProgram;
internal GLuint VAO;
internal GLuint g_glBuffers[BUFFER_COUNT];

internal v4 clearBackground = RGBToFloat(LIGHTGRAY);

// TODO(Xrhoys): to remove this global
internal GLuint g_bgTexture, g_emptyTexture;

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
	1, 2, 3    // second triangle
};

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

		/*
			R = Y + 1.140V
   			G = Y - 0.395U - 0.581V
			B = Y + 2.032U
		*/
		
        // const char *fragmentShaderSource = 
		// 	"#version 310 es\n"
		// 	"#extension GL_OES_EGL_image_external_essl3 : enable\n"
		// 	// "#extension GL_EXT_YUV_target : require\n"
		// 	"precision mediump float;\n"
		// 	"in vec2 TexCoord;\n"
		// 	"in vec4 Color;\n"
        //     "out vec4 FragColor;\n"
		// 	"uniform samplerExternalOES texY;\n"
		// 	"uniform samplerExternalOES texUV;\n"
        //     "void main()\n"
        //     "{\n"
		// 	"    vec4 yuv;\n"
		// 	"    float r, g, b;\n"
		// 	"    yuv.x = texture(texY, TexCoord).r;\n"
		// 	// "    TexCoord.x += 2.0f/3.0f;\n"
		// 	"    yuv.yz = texture(texUV, TexCoord).ra;\n"
		// 	"    r = yuv.x + 1.140f * yuv.y;\n"
		// 	"    g = yuv.x - 0.395f * yuv.z - 0.581f * yuv.y;\n"
		// 	"    b = yuv.x + 2.032f * yuv.z;\n"
		// 	// "    r = dot(yuv, vec4(1.0f, 1.0f, 1.0f, 1.0f));\n"
		// 	// "    g = dot(yuv, vec4(1.0f, 1.0f, 1.0f, 1.0f));\n"
		// 	// "    b = dot(yuv, vec4(1.0f, 1.0f, 1.0f, 1.0f));\n"
        //     "    FragColor = texture(texY, TexCoord);\n"
		// 	// "    FragColor = vec4(r, g, b, 1.0f);\n"
        //     "}\0";

		const char *fragmentShaderSource = 
			"#version 310 es\n"
			"#extension GL_OES_EGL_image_external : require\n"
			"#extension GL_OES_EGL_image_external_essl3 : enable\n"
			"precision mediump float;\n"
			"in vec2 TexCoord;\n"
			"in vec4 Color;\n"
			"out vec4 FragColor;\n"
			"uniform samplerExternalOES tex;\n"
			"void main()\n"
			"{\n"
			"    FragColor = texture2D(tex, TexCoord);\n"
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
			fprintf(stdout, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
		}
		
        shaderProgram = glCreateProgram();
		
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
		
        // Check ShaderCompile success
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			fprintf(stdout, "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n%s", infoLog);
		}
		
        //glDeleteShader(vertexShader);
        //glDeleteShader(fragmentShader);
		
        glGenVertexArrays(1, &VAO);
		glGenBuffers(BUFFER_COUNT, g_glBuffers);
		
        glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[BG_VERTEX_ARRAY]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 4, vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[BG_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, indices, GL_STATIC_DRAW);
		
		// TODO(Xrhoys): remove hard coded values later
		glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[FONT_VERTEX_ARRAY]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[FONT_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);
		
		glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[UI_VERTEX_ARRAY]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 50000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[UI_INDEX_ARRAY]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 50000 * 6 / 4, NULL, GL_DYNAMIC_DRAW);
		
		glGenTextures(1, &g_bgTexture);
		
		glBindTexture(GL_TEXTURE_2D, g_bgTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
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
		
		
		glUseProgram(shaderProgram);

		{
			glBindVertexArray(VAO);
			
			glBindBuffer(GL_ARRAY_BUFFER, g_glBuffers[BG_VERTEX_ARRAY]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_glBuffers[BG_INDEX_ARRAY]);
			
			UseVertexAttrib();
			
			glBindTexture(GL_TEXTURE_2D, g_bgTexture);
			
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		// NOTE(Xrhoys): render tree-node UI structure
		// NOTE(Xrhoys): there is a specific API call order here:
		// 1. Bind data buffers and/or update buffer data
		// 2. Bind vertex array strcut
		// 3. Enable attributes
		// 4. Draw
		{
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
		
		// NOTE(Xrhoys): slow, experimental font engine debugging layer
		{
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

	}
}
