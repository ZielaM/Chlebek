#version 410 core

out vec4 FragColor;

// Opcjonalnie: kolor przekazany z C++
uniform vec3 u_Color;

void main()
{
    // Ustawiamy kolor (np. pomarańczowy jak ciasto)
    FragColor = vec4(u_Color, 1.0); 
}