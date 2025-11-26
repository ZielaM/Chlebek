#version 410 core

// Dane wejściowe z C++ (pozycja punktu)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

// Macierz Kamery (Model-View-Projection)
uniform mat4 u_ViewProjection;

void main()
{
    // Przeliczamy pozycję 3D na pozycję na ekranie
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
    
    vColor = aColor;
    
    // Ustawiamy wielkość punktu (żeby było je widać)
    gl_PointSize = 4.0;
}