### A simple graphics engine from scratch. 

It uses some librarys, so here are commands I use to compilate my programm

windows
- gcc main.c -o game.exe -I C:\msys64\mingw64\include -L C:\msys64\mingw64\lib -lraylib -lopengl32 -lgdi32 -lwinmm

linux
- gcc ...

---
## Explanation (by gemini)

### `Vector3` and `Matrix4x4`
The engine uses 3D floating-point vectors (`Vector3`) to represent vertex coordinates, normal vectors, and spatial directions. The `Matrix4x4` structure holds a 4x4 array of floats representing perspective transformation matrices in homogeneous coordinates.

### `Triangle`
The basic geometric primitive. It consists of three `Vector3` vertices (`points[3]`) and a `Color` field that determines the final shaded color of the surface.

### `Figure`
Represents a complete 3D object/mesh. It stores a dynamically allocated array of `Triangle` structures (`faces`) and an integer storing the total face count (`numFaces`).

### `Cam` and `LightPlane`
* **`Cam`**: Stores camera properties including position (`pos`), field of view (`fov`), aspect ratio, focal length, and maximum viewing distance.
* **`LightPlane`**: Stores a normalized direction vector (`dir`) representing directional light arriving from infinity.

---

## Key Functions and Mathematical Pipeline

### Matrix Multiplication and Projection: `multiply` & `projectPoint`

```c
Vector3 multiply(Vector3 vector, Matrix4x4 matrix, float *w);
Vector3 projectPoint(Vector3 point3D, Matrix4x4 translationMatrix);

```

#### Explanation

The `multiply` function transforms a 3D point using a 4x4 matrix. It treats the 3D vector as a 4D vector $[x, y, z, 1.0]$. The fourth output component, $w$, is stored separately through a pointer.

The `projectPoint` function performs the **perspective divide**. After multiplying the 3D point by the projection matrix, $w$ stores the depth scaling factor. If $w \neq 0$, all three transformed coordinates $(x, y, z)$ are divided by $w$. This division creates the foreshortening effect where objects farther from the camera appear smaller.

---

### Camera Control Functions: `moveCamera` & `rotateCamera`

```c
void moveCamera(Cam *camera, Vector3 movement);
void rotateCamera(Cam *camera, Vector2 rotation);
```
#### Explanation

* **`moveCamera`**: Updates the camera's world position based on relative movement inputs. It handles forward/backward motion along the current viewing direction, absolute vertical movement along the $Y$-axis, and sideways strafing using a calculated right vector.

* **`rotateCamera`**: Changes the camera's unit direction vector (dir) based on horizontal and vertical rotation angles. Vertical rotation updates the $XZ$ components, while horizontal rotation uses local right-axis calculations and Rodrigues' rotation formula to tilt the view without gimbal distortion.

### Matrix Transformations: worldTransform & lookAtCMatrix4x4

```c
worldTransform(Vector3 pos, Vector3 target, Vector3 up);
Matrix4x4 lookAt(Matrix4x4 m);
```

#### Explanation

* **`worldTransform`**: Constructs an orthonormal coordinate matrix (basis) from a camera position, target point, and up vector using Gram-Schmidt orthogonalization.

* **`lookAt`**: Computes the quick inverse of the camera's orientation/translation matrix, effectively converting world coordinates into view-space coordinates.

### Geometry Surface Normal: `createNormal`

```c
Vector3 createNormal(Triangle triangle);

```

#### Explanation

Calculates the unit normal vector perpendicular to the surface of a triangle.

1. Computes two edge vectors: $\vec{E_1} = P_1 - P_0$ and $\vec{E_2} = P_2 - P_0$.
2. Computes the cross product $\vec{N} = \vec{E_1} \times \vec{E_2}$.
3. Normalizes $\vec{N}$ to unit length.

The orientation of the normal vector depends on the winding order (vertex order) of the triangle vertices.

---

### Back-Face Culling: `isTriangleVisible`

```c
bool isTriangleVisible(Triangle triangle, Cam camera);

```

#### Explanation

Determines whether a triangle faces toward or away from the camera.

1. Computes a direction vector from the camera position to the first vertex of the triangle.
2. Computes the dot product between the surface normal and this view vector.
3. If the dot product is negative ($< 0$), the triangle faces the camera and is rendered. Otherwise, it is culled (skipped), reducing unnecessary rasterization overhead.

---

### Lighting and Shading: `dotProduct` with `LightPlane`

```c
float lightDot = dotProduct(face, lightPlane.dir);

```

#### Explanation

Implements basic Lambertian directional lighting. The dot product between the unit surface normal and the unit light direction vector calculates the cosine of the angle between them:

* $\text{Dot} = 1.0$: Surface directly faces the light source (maximum brightness).
* $\text{Dot} \le 0.0$: Surface faces away from the light source (dark/shadowed).

The resulting factor is scaled to RGB range $[0, 255]$ to shade the triangle dynamically.

---

### Depth Sorting: `compareTriangles`

```c
int compareTriangles(const void *a, const void *b);

```

#### Explanation

Provides a comparison callback for standard C `qsort` to implement the **Painter's Algorithm**.

1. Calculates the average $Z$-depth (centroid depth) of three vertices for each triangle.
2. Sorts triangles in descending order of their depth value.
3. Triangles farther from the camera are drawn first, allowing closer triangles to overwrite them and correctly resolve surface overlap without a hardware Z-buffer.

---

### Screen Coordinate Transformation: `onScreen`

```c
Vector2 onScreen(Vector3 point, int width, int height);

```

#### Explanation

Converts Normalized Device Coordinates (NDC) in the range $[-1.0, 1.0]$ to screen pixel coordinates:


$$\text{Pixel}_X = (X_{\text{ndc}} + 1.0) \times \frac{\text{width}}{2}$$

$$\text{Pixel}_Y = (1.0 - Y_{\text{ndc}}) \times \frac{\text{height}}{2}$$

The inversion of the $Y$ coordinate accounts for screen coordinate systems where the origin $(0,0)$ is located at the top-left corner.

---

### Object Operations: Transformation and Loading

* **`loadFromFile`**: Parses Wavefront `.obj` files, reading vertex definitions (`v`) and triangular faces (`f`).
* **`rotateFigure`**: Applies 3D rotation around arbitrary axes using Trigonometric Euler angles ($X$, $Y$, $Z$) relative to a specified center point.
* **`moveFigure`**: Applies translation by adding a displacement vector to all vertices.
* **`mergeFigures`**: Combines multiple `Figure` meshes into a single unified mesh using variadic arguments.

---

## Execution Flow inside `main`

1. **Initialization**:
* Initializes a window of size $1024 \times 1024$ using `InitWindow`.
* Configures camera properties, directional light, and builds the perspective projection matrix.


2. **Asset Loading & Setup**:
* Loads `axis.obj` and `UtahTeapot.obj`.
* Positions and rotates the meshes in world space.


3. **Render Loop** (`while (!WindowShouldClose())`):
* Rotates the teapot mesh incrementally around the $Y$-axis.
* Merges meshes into a single scene structure using `mergeFigures`.
* Clears the background to black.
* Calls `printFigure`, which filters visible faces, applies perspective projection, calculates lighting, sorts by depth, converts to pixel coordinates, and draws filled 2D triangles via raylib's `DrawTriangle`.
* Frees temporary allocated scene geometry and renders the frame.


4. **Shutdown**:
* Releases allocated mesh memory (`freeFigure`) and closes the raylib window context (`CloseWindow`).


