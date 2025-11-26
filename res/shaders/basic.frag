#version 410 core

in vec3 vColor;
out vec4 FragColor;

// Opcjonalnie: kolor przekazany z C++ (mnożnik)
uniform vec3 u_Color;

void main()
{
    // Ustawiamy kolor (np. pomarańczowy jak ciasto)
    FragColor = vec4(vColor * u_Color, 1.0); 
}