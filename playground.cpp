#include <stdio.h>
#include <vector>

#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
using namespace glm;

#include "common/shader.hpp"
#include "common/texture.hpp"
#include "common/controls.hpp"
#include "common/objloader.hpp"
#include "common/vboindexer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/string_cast.hpp>
GLFWwindow* window = nullptr;


int main(){

    // Init GLFW
    glewExperimental = true;
    if (!glfwInit()){
        fprintf(stderr, "Faild to init GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

    int width = 1024;
    int height = 768;

    window = glfwCreateWindow(width, height, "Tut 03", NULL, NULL);

    if (window == NULL){
        fprintf(stderr, "Failed to open GLFW ...");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental=true;
    if (glewInit() != GLEW_OK){
        fprintf(stderr, "Failer to init GLEW");
        return -1;
    }

    glfwSetScrollCallback(window, scroll_callback);

    // To stop window from autoclosing - lol
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // Accept closer fragments
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile Shaders
    GLuint programID = LoadShaders( "shaders/vertexShader.glsl", "shaders/fragmentShader.glsl" );


    mat4 ModelMatrix = translate(mat4(1.0f), vec3(-1.0f, 0.0f, 0.0f)); // At the origin w/ identity
 
    // Hand over to shaders
    GLuint MatrixID = glGetUniformLocation(programID, "MVP"); // Init full cam transform
    GLuint ViewID = glGetUniformLocation(programID, "V"); // Init view transform 
    GLuint ModelID = glGetUniformLocation(programID, "M"); // Init model transform 
    GLuint ModelViewID = glGetUniformLocation(programID, "MV"); // Init model transform 
    GLuint LightPosID = glGetUniformLocation(programID, "LightPosition_worldspace"); // Init model transform 

    // Implemented the texture loader in /common/texture.cpp/hpp
    // GLuint Texture = loadBMP_custom("uvtemplate.bmp");
    GLuint Texture = loadDDS("assets/tutorial9/uvmap.DDS");

	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

    // Init VAO
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("assets/tutorial9/suzanne.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);


    // Variables for movin cube
    float x_pos = 0.0f;
    float x_delta = 0.01f;
    float x_pos_sign = 1.0f;

    // Define light position
    vec3 lightPos = vec3(4.0f, 4.0f, 4.0f);
    do{
        // Clear scene to avoid flickering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        computeMatricesFromInputs();
        mat4 ProjectionMatrix = getProjectionMatrix();
        mat4 ViewMatrix = getViewMatrix();

        if (ModelMatrix[3][0] > 1.0f){
            x_pos_sign = -1.0f;
        }
        else if (ModelMatrix[3][0] < -1.0f)
        {
            x_pos_sign = 1.0f;
        }
        
        x_pos = x_pos_sign * x_delta;
        ModelMatrix = translate(ModelMatrix, vec3(x_pos, 0.0f, 0.0f));;

        mat4 mvp = ProjectionMatrix * ViewMatrix * ModelMatrix;
        mat4 mv = ViewMatrix * ModelMatrix;
        
        glUseProgram(programID);
        // send transform to the shader, doesnt mater where its at so long as it is instantiated before the end of the loop
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(ViewID, 1, GL_FALSE, &ViewMatrix[0][0]);
        glUniformMatrix4fv(ModelID, 1, GL_FALSE, &ModelMatrix[0][0]);
        glUniformMatrix4fv(ModelViewID, 1, GL_FALSE, &mv[0][0]);
        glUniform3fv(LightPosID, 1, &lightPos[0]);
        
        glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

        // Configuration for the vertex positions/buffer
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // Configuration for the UV buffer
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
            1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
        glVertexAttribPointer(
            2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

  
        // Draw the triangle !
		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
    }
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) ==0);

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;

}