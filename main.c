#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "raylib.h"

const int screenWidth = 1024;
const int screenHeight = 1024;

Vector2 onScreen(Vector2 point2D, int width, int height) {
    point2D.x = point2D.x + width/2;
    point2D.y = height - (point2D.y + height/2);
    return point2D;
}

Vector2 projectPoint(Vector3 point3D) {
    Vector2 point;
    point.x = (point3D.x * 700 / point3D.z);
    point.y = (point3D.y * 700 / point3D.z);
    return point;
}

struct Figure {
    Vector3 *points;
    int numEdges;
    Vector2 *edges;
    int numPoints;
};

struct Figure createFigure(Vector3 *points, int numPoints, Vector2 *edges, int numEdges) {

    struct Figure figure;
    figure.points = points;
    figure.numPoints = numPoints;
    figure.edges = edges;
    figure.numEdges = numEdges;

    if (numPoints == 0 || points == NULL) {
        figure.points = NULL;
        return figure;
    }

    figure.points = (Vector3 *)malloc(numPoints * sizeof(Vector3));
    for (int i = 0; i < numPoints; i++) {
        figure.points[i] = points[i];
    }

    if (numEdges == 0 || edges == NULL) {
        figure.edges = NULL;
    }

    figure.edges = (Vector2 *)malloc(numEdges * sizeof(Vector2));
    for (int i = 0; i < numEdges; i++) {
        figure.edges[i] = edges[i];
    }
    return figure;
}

struct Figure createSphere(Vector3 center, int radius, int segments) {
    
    struct Figure sphere;
    sphere.numPoints = (segments + 1) * (segments + 1);
    sphere.points = (Vector3 *)malloc(sphere.numPoints * sizeof(Vector3));

    for (int i = 0; i <= segments; i++) {
        double angleX = i * 3.14159265358979323846 / segments;
        for (int j = 0; j <= segments; j++) {
            double angleY = j * 3.14159265358979323846 * 2 / segments;

            double x = center.x + radius * sin(angleX) * cos(angleY);
            double y = center.y + radius * sin(angleX) * sin(angleY);
            double z = center.z + radius * cos(angleX);

            sphere.points[i * (segments + 1) + j] = (Vector3){x, y, z};
        }
    }

    sphere.numEdges = (segments + 1) * (segments) * 2;
    sphere.edges = (Vector2 *)malloc(sphere.numEdges * sizeof(Vector2));

    for (int i = 0; i < segments; i++) {
        for (int j = 0; j <= segments; j++) {
            sphere.edges[(i * (segments + 1) + j) * 2] = (Vector2){(i * (segments + 1) + j), (i * (segments + 1) + j + 1)};
            sphere.edges[(i * (segments + 1) + j) * 2 + 1] = (Vector2){(i * (segments + 1) + j), ((i + 1) * (segments + 1) + j)};
        }
    }
    return sphere;
}

struct Figure createCube(Vector3 start, Vector3 size){

    struct Figure cube;
    cube.numPoints = 8;
    cube.points = (Vector3 *)malloc(cube.numPoints * sizeof(Vector3));
    cube.points[0] = start;
    cube.points[1] = (Vector3){start.x + size.x, start.y, start.z};
    cube.points[2] = (Vector3){start.x + size.x, start.y + size.y, start.z};
    cube.points[3] = (Vector3){start.x, start.y +size.y, start.z};
    cube.points[4] = (Vector3){start.x, start.y, start.z + size.z};
    cube.points[5] = (Vector3){start.x + size.x, start.y, start.z + size.z};
    cube.points[6] = (Vector3){start.x + size.x, start.y + size.y, start.z + size.z};
    cube.points[7] = (Vector3){start.x, start.y + size.y, start.z + size.z};

    cube.numEdges = 12;
    cube.edges = (Vector2 *)malloc(cube.numEdges * sizeof(Vector2));
    memcpy(cube.edges, (Vector2[]){{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}, 12 * sizeof(Vector2));
    return cube;
}

void printFigure(struct Figure *figure) {
    //for (int i = 0; i < figure->numPoints; i++) {
    //    DrawCircleV(onScreen(projectPoint(figure->points[i]), screenWidth, screenHeight), 5.0f, GREEN);
    //}
    for (int i = 0; i < figure->numEdges; i++) {
        int a = figure->edges[i].x;
        int b = figure->edges[i].y;
        DrawLineV(onScreen(projectPoint(figure->points[a]), screenWidth, screenHeight), onScreen(projectPoint(figure->points[b]), screenWidth, screenHeight), GREEN);
    }
}

void rotateFigure(struct Figure *figure, Vector3 rotationCenter, double angleX, double angleY, double angleZ) {
    double radX = angleX * 3.14159265358979323846 / 180.0;
    double radY = angleY * 3.14159265358979323846 / 180.0;
    double radZ = angleZ * 3.14159265358979323846 / 180.0;

    for (int i = 0; i < figure->numPoints; i++) {
        Vector3 p = figure->points[i];

        // Translate point to origin
        p.x -= rotationCenter.x;
        p.y -= rotationCenter.y;
        p.z -= rotationCenter.z;

        // Rotate around X-axis
        double y1 = p.y * cos(radX) - p.z * sin(radX);
        double z1 = p.y * sin(radX) + p.z * cos(radX);
        p.y = y1;
        p.z = z1;

        // Rotate around Y-axis
        double x2 = p.x * cos(radY) + p.z * sin(radY);
        double z2 = -p.x * sin(radY) + p.z * cos(radY);
        p.x = x2;
        p.z = z2;

        // Rotate around Z-axis
        double x3 = p.x * cos(radZ) - p.y * sin(radZ);
        double y3 = p.x * sin(radZ) + p.y * cos(radZ);
        p.x = x3;
        p.y = y3;

        p.x += rotationCenter.x;
        p.y += rotationCenter.y;
        p.z += rotationCenter.z;

        figure->points[i] = p;
    }
}

void moveFigure(struct Figure *figure, Vector3 dir) {
    for (int i = 0; i < figure->numPoints; i++) {
        figure->points[i].x+=dir.x;
        figure->points[i].y+=dir.y;
        figure->points[i].z+=dir.z;
    }
}

void freeFigure(struct Figure *figure) {
    if (figure->points != NULL) {
        free(figure->points);
        figure->points = NULL;
    }
    figure->numPoints = 0;
    if (figure->edges != NULL) {
        free(figure->edges);
        figure->edges = NULL;
    }
    figure->numEdges = 0;
}
/*
struct Camera {
    Vector3 pos;
    int focalLength;
    int fov;
    Vector2 dir;
};

Vector2 projectFromCamera(Vector3 point, int fov, int vDist) {
    int z = vDist * point.z;
    
    if (z < 1) {
        return (Vector2) {0, 0};
    }

    double scale = fov / z;

    return (Vector3){point.x * scale, point.y * scale, point.z * scale};



}
*/
//gcc main.c -o game.exe -I C:\msys64\mingw64\include -L C:\msys64\mingw64\lib -lraylib -lopengl32 -lgdi32 -lwinmm
int main(void) {
    InitWindow(screenWidth, screenHeight, "");

    SetTargetFPS(60);

    struct Figure s1 = createSphere((Vector3){150, 0, 300}, 100, 20);

    struct Figure c1 = createCube((Vector3){-50, -50, 250}, (Vector3){100, 100, 100});
    /*
    struct Camera camera;

    camera.pos = (Vector3){0, 0, 0};
    camera.dir = (Vector2){0, 0};
    float playerSpeed = 5.0f;
    float rotationSpeed = 5.0f;
    */
    // Главный цикл
    while (!WindowShouldClose()) {
        /*
        if (IsKeyDown(KEY_RIGHT))      camera.pos.x += playerSpeed;
        if (IsKeyDown(KEY_LEFT))       camera.pos.x -= playerSpeed;
        if (IsKeyDown(KEY_UP))         camera.pos.z += playerSpeed;
        if (IsKeyDown(KEY_DOWN))       camera.pos.z -= playerSpeed;
        if (IsKeyDown(KEY_SPACE))      camera.pos.y += playerSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT)) camera.pos.y -= playerSpeed;
        if (IsKeyDown(KEY_W)) camera.dir.y += rotationSpeed;
        if (IsKeyDown(KEY_S)) camera.dir.y -= rotationSpeed;
        if (IsKeyDown(KEY_A)) camera.dir.x += rotationSpeed;
        if (IsKeyDown(KEY_D)) camera.dir.x -= rotationSpeed;
        */


        //Vector2 mousePos = GetMousePosition();


        BeginDrawing();

            ClearBackground(RAYWHITE);
            rotateFigure(&c1, (Vector3){0, 0, 300}, 1, 1, 1);
            printFigure(&c1);
            rotateFigure(&s1, (Vector3){0, 0, 300}, 0, -1, 0);
            printFigure(&s1);

            //DrawText("Controls: Arrow keys to move", 10, 10, 20, DARKGRAY);
            DrawFPS(10, 40); // Показывает счетчик FPS в углу

        EndDrawing();
    }

    freeFigure(&c1);
    freeFigure(&s1);
    CloseWindow();
    return 0;
}