#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "raylib.h"

const int screenWidth = 1024;
const int screenHeight = 1024;

typedef struct Matrix4x4 {
    float m[4][4];
} Matrix4x4;

Matrix4x4 createMatrix4x4 (float m[4][4]) {
    Matrix4x4 matrix = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            matrix.m[i][j] = m[i][j];
        }
    }
    return matrix;
}

Vector3 multiply (Vector3 vector, Matrix4x4 matrix)
{
    Vector3 result;
    result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
    result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
    result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
    float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];
    if (w != 0.0f) {
        result.x /= w;
        result.y /= w;
        result.z /= w;
    }
    return result;
}

Vector2 onScreen(Vector3 point, int width, int height) {
    Vector2 point2D;
    point2D.x = (point.x + 1.0f) * width / 2.0f;
    point2D.y = (1.0f - point.y) * height / 2.0f;
    return point2D;
}

typedef struct LightPlane {
    Vector3 dir;
} LightPlane;

LightPlane createLightPlane(Vector3 dir) {
    LightPlane lightPlane;
    lightPlane.dir = dir;
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length != 0.0f) {
        lightPlane.dir.x /= length;
        lightPlane.dir.y /= length;
        lightPlane.dir.z /= length;
    }
    return lightPlane;
}

typedef struct Cam {
    Vector3 pos;
    float focalLength;
    float aspectRatio;
    float maxViewDistance;
    int fov;
    Vector2 dir;
} Cam;
/*
Vector2 projectFromCamera(Vector3 point, int fov, int vDist) {
    int z = vDist * point.z;
    
    if (z < 1) {
        return (Vector2) {0, 0};
    }

    double scale = fov / z;

    return (Vector3){point.x * scale, point.y * scale, point.z * scale};



}
*/

Vector3 projectPoint(Vector3 point3D, Matrix4x4 translationMatrix) {
    Vector3 transformedPoint = multiply(point3D, translationMatrix);
    return transformedPoint;
}


typedef struct Triangle {
    Vector3 points[3];
    Color color;
} Triangle;

Triangle createTriangle(Vector3 a, Vector3 b, Vector3 c) {
    Triangle triangle;
    triangle.points[0] = a;
    triangle.points[1] = b;
    triangle.points[2] = c;
    return triangle;
}

int compareTriangles(const void *a, const void *b) {
    const Triangle *triangleA = (const Triangle *)a;
    const Triangle *triangleB = (const Triangle *)b;

    float zA = (triangleA->points[0].z + triangleA->points[1].z + triangleA->points[2].z) / 3.0f;
    float zB = (triangleB->points[0].z + triangleB->points[1].z + triangleB->points[2].z) / 3.0f;

    if (zA < zB) return 1;
    if (zA > zB) return -1;
    return 0;
}

Vector3 createNormal(Triangle triangle) {
    Vector3 edge1 = (Vector3){triangle.points[1].x - triangle.points[0].x, triangle.points[1].y - triangle.points[0].y, triangle.points[1].z - triangle.points[0].z};
    Vector3 edge2 = (Vector3){triangle.points[2].x - triangle.points[0].x, triangle.points[2].y - triangle.points[0].y, triangle.points[2].z - triangle.points[0].z};

    Vector3 normal;
    normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
    normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
    normal.z = edge1.x * edge2.y - edge1.y * edge2.x;

    float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length != 0.0f) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }

    return normal;
}

float dotProduct(Triangle triangle, Vector3 vector) {
    Vector3 normal = createNormal(triangle);

    float dotProduct = normal.x * vector.x + normal.y * vector.y + normal.z * vector.z;

    return dotProduct;
}

bool isTriangleVisible(Triangle triangle, Cam camera) {
    Vector3 toCamera = (Vector3){triangle.points[0].x - camera.pos.x, triangle.points[0].y - camera.pos.y, triangle.points[0].z - camera.pos.z};   
    float dot = dotProduct(triangle, toCamera);
    return dot < 0;
}

typedef struct Figure {
    Triangle *faces;
    int numFaces;
} Figure;

Figure createFigure(Triangle *faces, int numFaces) {

    Figure figure;
    figure.faces = faces;
    figure.numFaces = numFaces;

    if (numFaces == 0 || faces == NULL) {
        figure.faces = NULL;
        return figure;
    }

    figure.faces = (Triangle *)malloc(numFaces * sizeof(Triangle));
    for (int i = 0; i < numFaces; i++) {
        figure.faces[i] = faces[i];
    }

    return figure;
}

Figure loadFromFile(const char *filename) {
    Figure figure = {0};
    FILE *file = fopen(filename, "r");
    if (!file) {
        return figure;
    }

    Vector3 *tempVertices = NULL;
    int vertexCount = 0;
    int vertexCapacity = 0;

    Triangle *tempFaces = NULL;
    int faceCount = 0;
    int faceCapacity = 0;

    char line[256];

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            Vector3 v;
            sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z);
            if (vertexCount >= vertexCapacity) {
                vertexCapacity = (vertexCapacity == 0) ? 128 : vertexCapacity * 2;
                Vector3 *newVerts = realloc(tempVertices, vertexCapacity * sizeof(Vector3));
                if (!newVerts) break;
                tempVertices = newVerts;
            }
            tempVertices[vertexCount++] = v;
            
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int v1, v2, v3;
            sscanf(line + 2, "%d %d %d", &v1, &v2, &v3);
            
            v1--; v2--; v3--;
            if (v1 >= 0 && v1 < vertexCount &&
                v2 >= 0 && v2 < vertexCount &&
                v3 >= 0 && v3 < vertexCount) 
            {
                if (faceCount >= faceCapacity) {
                    faceCapacity = (faceCapacity == 0) ? 128 : faceCapacity * 2;
                    Triangle *newFaces = realloc(tempFaces, faceCapacity * sizeof(Triangle));
                    if (!newFaces) break;
                    tempFaces = newFaces;
                }

                Triangle tri;
                tri.points[0] = tempVertices[v1];
                tri.points[1] = tempVertices[v2];
                tri.points[2] = tempVertices[v3];
                tri.color = WHITE;

                tempFaces[faceCount++] = tri;
            }
        
        }
    }

    fclose(file);
    free(tempVertices);
    figure.faces = tempFaces;
    figure.numFaces = faceCount;
    
    return figure;
}

void freeFigure(Figure *figure) {
    if (figure->faces != NULL) {
        free(figure->faces);
        figure->faces = NULL;
    }
    figure->numFaces = 0;
}

Figure createSphere(Vector3 center, int radius, int segments)
{
    Figure sphere = {0};

    if (segments < 3 || radius <= 0) {
        return sphere;
    }

    int ringCount = segments - 1;
    sphere.numFaces = 2 * segments * segments;
    sphere.faces = malloc((size_t)sphere.numFaces * sizeof(Triangle));

    if (sphere.faces == NULL) {
        sphere.numFaces = 0;
        return sphere;
    }

    Vector3 *points = malloc(
        (size_t)(segments + 1) * (segments + 1) * sizeof(Vector3)
    );

    if (points == NULL) {
        freeFigure(&sphere);
        return sphere;
    }

    const double pi = 3.14159265358979323846;

    for (int i = 0; i <= segments; i++) {
        double angelX = pi * i / segments;

        for (int j = 0; j <= segments; j++) {
            double angelY = 2.0 * pi * j / segments;

            points[i * (segments + 1) + j] = (Vector3){
                center.x + radius * sin(angelX) * cos(angelY),
                center.y + radius * sin(angelX) * sin(angelY),
                center.z + radius * cos(angelX)
            };
        }
    }

    int faceIndex = 0;

    // Top cap and middle bands.
    for (int i = 0; i < segments; i++) {
        for (int j = 0; j < segments; j++) {
            int a = i * (segments + 1) + j;
            int b = i * (segments + 1) + j + 1;
            int c = (i + 1) * (segments + 1) + j;
            int d = (i + 1) * (segments + 1) + j + 1;

            // Clockwise winding viewed from outside.
            sphere.faces[faceIndex++] =
                createTriangle(points[a], points[c], points[b]);

            sphere.faces[faceIndex++] =
                createTriangle(points[b], points[c], points[d]);
        }
    }

    free(points);
    return sphere;
}

Figure createCube(Vector3 start, Vector3 size){

    Figure cube;
    cube.numFaces = 12;
    cube.faces = (Triangle *)malloc(cube.numFaces * sizeof(Triangle));

    Vector3 p000 = (Vector3){start.x, start.y, start.z};
    Vector3 p100 = (Vector3){start.x + size.x, start.y, start.z};
    Vector3 p110 = (Vector3){start.x + size.x, start.y + size.y, start.z};
    Vector3 p010 = (Vector3){start.x, start.y + size.y, start.z};
    Vector3 p001 = (Vector3){start.x, start.y, start.z + size.z};
    Vector3 p101 = (Vector3){start.x + size.x, start.y, start.z + size.z};
    Vector3 p111 = (Vector3){start.x + size.x, start.y + size.y, start.z + size.z};
    Vector3 p011 = (Vector3){start.x, start.y + size.y, start.z + size.z};

    cube.faces[0] = createTriangle(p000, p010, p110);
    cube.faces[1] = createTriangle(p000, p110, p100);
    cube.faces[2] = createTriangle(p001, p101, p111);
    cube.faces[3] = createTriangle(p001, p111, p011);
    cube.faces[4] = createTriangle(p000, p100, p101);
    cube.faces[5] = createTriangle(p000, p101, p001);
    cube.faces[6] = createTriangle(p010, p011, p111);
    cube.faces[7] = createTriangle(p010, p111, p110);
    cube.faces[8] = createTriangle(p000, p001, p011);
    cube.faces[9] = createTriangle(p000, p011, p010);
    cube.faces[10] = createTriangle(p100, p110, p111);
    cube.faces[11] = createTriangle(p100, p111, p101);

    return cube;
}


void printFigure(Figure *figure, Matrix4x4 translationMatrix, Cam camera, LightPlane lightPlane) {

    Triangle *trianglesToDraw = NULL;
    int triangleCount = 0;
    for (int i = 0; i < figure->numFaces; i++) {
        Triangle face = figure->faces[i];
        if (!isTriangleVisible(face, camera)) {
            continue;
        }
        triangleCount++;
        Triangle *newTrianglesToDraw = realloc(trianglesToDraw, (triangleCount) * sizeof(Triangle));
        if (newTrianglesToDraw == NULL) {
            free(trianglesToDraw);
            return;
        }
        Vector3 p0 = projectPoint(face.points[0], translationMatrix);
        Vector3 p1 = projectPoint(face.points[1], translationMatrix);
        Vector3 p2 = projectPoint(face.points[2], translationMatrix);
        float lightDot = dotProduct(face, lightPlane.dir);
        Color color = (Color){255 * lightDot, 255 * lightDot, 255 * lightDot, 255};
        Triangle face2D = {p0, p1, p2, color};
        trianglesToDraw = newTrianglesToDraw;
        trianglesToDraw[triangleCount - 1] = face2D;
    }

    qsort(trianglesToDraw, triangleCount, sizeof(Triangle), compareTriangles);


    for (int i = 0; i < triangleCount; i++) {
        Triangle face = trianglesToDraw[i];
        Vector2 p0 = onScreen(face.points[0], screenWidth, screenHeight);
        Vector2 p1 = onScreen(face.points[1], screenWidth, screenHeight);
        Vector2 p2 = onScreen(face.points[2], screenWidth, screenHeight);
        DrawTriangle(p2, p1, p0, face.color);
        //DrawLineV(face.points[0], face.points[1], GREEN);
        //DrawLineV(face.points[1], face.points[2], GREEN);
        //DrawLineV(face.points[2], face.points[0], GREEN);
    }
    free(trianglesToDraw);
}

void rotateFigure(Figure *figure, Vector3 rotationCenter, double angleX, double angleY, double angleZ) {
    double radX = angleX * 3.14159265358979323846 / 180.0;
    double radY = angleY * 3.14159265358979323846 / 180.0;
    double radZ = angleZ * 3.14159265358979323846 / 180.0;

    for (int i = 0; i < figure->numFaces; i++) {
        for (int j = 0; j < 3; j++) {
            Vector3 p = figure->faces[i].points[j];

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

            // Translate point back
            p.x += rotationCenter.x;
            p.y += rotationCenter.y;
            p.z += rotationCenter.z;

            figure->faces[i].points[j] = p;
        }
    }
}

void moveFigure(Figure *figure, Vector3 dir) {
    for (int i = 0; i < figure->numFaces; i++) {
        for (int j = 0; j < 3; j++) {
            figure->faces[i].points[j].x += dir.x;
            figure->faces[i].points[j].y += dir.y;
            figure->faces[i].points[j].z += dir.z;
        }
    }
}

Figure mergeFigures(int numFigures, ...) {
    va_list args;
    va_start(args, numFigures);

    Figure mergedFigure = {0};

    int totalFaces = 0;
    Triangle *allFaces = NULL;

    for (int i = 0; i < numFigures; i++) {
        Figure figure = va_arg(args, Figure);
        totalFaces += figure.numFaces;
        for (int j = 0; j < figure.numFaces; j++) {
            Triangle *newAllFaces = realloc(allFaces, totalFaces * sizeof(Triangle));
            if (newAllFaces == NULL) {
                free(allFaces);
                va_end(args);
                return mergedFigure;
            }
            allFaces = newAllFaces;
            allFaces[totalFaces - figure.numFaces + j] = figure.faces[j];
        }
    }

    va_end(args);
    mergedFigure.faces = allFaces;
    mergedFigure.numFaces = totalFaces;
    return mergedFigure;
}

//gcc main.c -o game.exe -I C:\msys64\mingw64\include -L C:\msys64\mingw64\lib -lraylib -lopengl32 -lgdi32 -lwinmm
int main(void) {
    InitWindow(screenWidth, screenHeight, "");

    SetTargetFPS(60);

    LightPlane lightPlane = createLightPlane((Vector3){0, 0, -1});
    Cam camera;
    camera.pos = (Vector3){0, 0, 0};
    camera.dir = (Vector2){0, 0};
    camera.focalLength = 0.1;
    camera.aspectRatio = (float)screenHeight / (float)screenWidth;
    camera.maxViewDistance = 1000.0f;
    camera.fov = 90;

    int fov = 90;
    Matrix4x4 translationMatrix = createMatrix4x4((float[4][4]){
        {camera.aspectRatio * 1.0f / tanf(camera.fov * 0.5f / 180.0f * 3.14159f), 0, 0, 0},
        {0, 1.0f / tanf(camera.fov * 0.5f / 180.0f * 3.14159f), 0, 0},
        {0, 0, camera.maxViewDistance/(camera.maxViewDistance-camera.focalLength), 1},
        {0, 0, -camera.maxViewDistance*camera.focalLength/(camera.maxViewDistance-camera.focalLength), 0}
    });

    //Figure s1 = createSphere((Vector3){1.5, 0, 3}, 1, 20);

    //Figure c1 = createCube((Vector3){-0.5, -0.5, -0.5}, (Vector3){1, 1, 1});
    //moveFigure(&c1, (Vector3){0, 0, 3});
    Figure teapot = loadFromFile("UtahTeapot.obj");
    moveFigure(&teapot, (Vector3){0, 0-2, 8});
    rotateFigure(&teapot, (Vector3){0, -2, 8}, -90, 0, 0);
    Figure scene;
    /*
    Cam camera;

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
        //rotateFigure(&c1, (Vector3){-0.3, 0.2, 3}, 0.7, 1, 1);
        //rotateFigure(&s1, (Vector3){0, 0, 3}, 0, -1, 0);
        rotateFigure(&teapot, (Vector3){0, 0, 8}, 0, 1, 0);
        scene = mergeFigures(1, teapot);

        //Vector2 mousePos = GetMousePosition();

        BeginDrawing();

            ClearBackground(BLACK);
            printFigure(&scene, translationMatrix, camera, lightPlane);



            //DrawText("Controls: Arrow keys to move", 10, 10, 20, DARKGRAY);
            DrawFPS(10, 40); // Показывает счетчик FPS в углу

        EndDrawing();
    }

    //freeFigure(&c1);
    //freeFigure(&s1);
    freeFigure(&teapot);
    freeFigure(&scene);
    CloseWindow();
    return 0;
}