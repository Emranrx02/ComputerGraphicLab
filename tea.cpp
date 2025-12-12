#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <cstdlib>
#include <ctime>
// --- Global Variables for Animation, Camera, and Game State ---
float angle = 0.0f;       // Angle for shopkeeper's arm
float steamY = 0.0f;      // Height of steam particles
float cameraAngle = 0.0f; // Camera rotation angle
float camX = 0.0f, camZ = 15.0f;
float cameraDist = 20.0f; // Camera distance (zoom)
float leafAngle = 0.0f;   // global angle for tree leaf sway

bool showLoading = true;  // Start with loading screen
bool isPaused = false;    // Animation pause state
bool isNight = false;     // Day/night toggle

// --- Colors ---
void setColor(float r, float g, float b, float a = 1.0) {
    glColor4f(r, g, b, a);
}

// --- Text Rendering Helper ---
void drawBitmapText(const char *string, float x, float y, void *font) {
    glRasterPos2f(x, y);
    for (const char *c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// On-screen overlay showing controls
void drawOverlay() {
    // Render 2D HUD: disable depth test and use ortho
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Background translucent box for readability
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glBegin(GL_QUADS);
        glVertex2f(10, 560);
        glVertex2f(260, 560);
        glVertex2f(260, 510);
        glVertex2f(10, 510);
    glEnd();
    glDisable(GL_BLEND);

    // Draw controls text in white
    setColor(1.0, 1.0, 1.0);
    drawBitmapText("Controls:", 20, 540, GLUT_BITMAP_HELVETICA_12);
    drawBitmapText("ENTER - Start", 20, 525, GLUT_BITMAP_HELVETICA_12);
    drawBitmapText("Space / P - Pause", 20, 510, GLUT_BITMAP_HELVETICA_12);
    drawBitmapText("Left / Right - Rotate Camera", 130, 525, GLUT_BITMAP_HELVETICA_12);
    drawBitmapText("N - Toggle Day/Night", 130, 510, GLUT_BITMAP_HELVETICA_12);

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// Draw shop name above the stall in world coordinates
void drawShopName() {
    const char *name = "DHAKAI CHA DOKAN";
    // Place text above the stall roof (matches stall transform in drawStall)
    // choose color depending on day/night
    if (!isNight) setColor(0.0f, 0.0f, 0.0f);
    else setColor(1.0f, 0.95f, 0.6f);
    // Raster position in world coords - adjust height/z to fit roof
    glRasterPos3f(-2.8f, 5.8f, 0.6f);
    for (const char *c = name; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// Draw a simple moon in the sky at night
void drawMoon() {
    if (!isNight) return;
    glPushMatrix();
    glTranslatef(-12.0f, 12.0f, -10.0f);
    setColor(0.95f, 0.95f, 0.85f);
    glutSolidSphere(1.2, 20, 20);
    glPopMatrix();
}

// --- Pedestrian system ---
struct Pedestrian {
    float x;
    float z;
    float speed; // movement along X
};

std::vector<Pedestrian> pedestrians;

void initPedestrians() {
    pedestrians.clear();
    const int N = 6; // number of walkers
    for (int i = 0; i < N; ++i) {
        Pedestrian p;
        // start spread along road X axis
        p.x = -25.0f + (float)rand() / RAND_MAX * 50.0f; // [-25,25]
        // road region in the scene is around z = 12 (8..18 in drawGroundAndRoad)
        p.z = 11.5f + (float)rand() / RAND_MAX * 3.0f; // [11.5,14.5]
        // random speed and direction
        float s = 0.02f + (float)rand() / RAND_MAX * 0.06f; // [0.02,0.08]
        if (rand() % 2 == 0) s = -s;
        p.speed = s;
        pedestrians.push_back(p);
    }
}

// --- Loading Screen ---
void drawLoadingScreen() {
    // Switch to 2D Orthographic projection for text
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Background
    glClearColor(0.1, 0.1, 0.2, 1.0); // Dark Blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setColor(1.0, 1.0, 1.0); // White text

    // Title
    drawBitmapText("Project Name: A Tea Stall", 300, 550, GLUT_BITMAP_HELVETICA_18);
    drawBitmapText("Submitted By:", 350, 520, GLUT_BITMAP_HELVETICA_18);

    // Table Header
    float tableTop = 480;
    float rowHeight = 30;
    float col1 = 150, col2 = 250, col3 = 550;

    setColor(0.8, 0.8, 0.8);
    drawBitmapText("No.", col1, tableTop, GLUT_BITMAP_HELVETICA_18);
    drawBitmapText("Name", col2, tableTop, GLUT_BITMAP_HELVETICA_18);
    drawBitmapText("ID", col3, tableTop, GLUT_BITMAP_HELVETICA_18);

    // Horizontal Line under header
    glBegin(GL_LINES);
    glVertex2f(140, tableTop - 5); glVertex2f(650, tableTop - 5);
    glEnd();

    // Student Data
    const char* students[][3] = {
        {"1", "Md Zahid Hasan Patwary", "221-15-4996"},
        {"2", "Md Amran Haque", "221-15-5662"},
        {"3", "Md Sanaullah", "221-15-4995"},
        {"4", "Md Shakibul Islam", "221-15-5551"},
        {"5", "Md Tahsinul Hoque", "221-15-4661"}
    };

    for(int i = 0; i < 5; i++) {
        float y = tableTop - ((i + 1) * rowHeight) - 10;
        drawBitmapText(students[i][0], col1, y, GLUT_BITMAP_HELVETICA_18);
        drawBitmapText(students[i][1], col2, y, GLUT_BITMAP_HELVETICA_18);
        drawBitmapText(students[i][2], col3, y, GLUT_BITMAP_HELVETICA_18);
    }

    // Footer Info
    drawBitmapText("Section: 61_N1", 350, 200, GLUT_BITMAP_HELVETICA_18);
    drawBitmapText("Department of CSE", 330, 170, GLUT_BITMAP_HELVETICA_18);
    drawBitmapText("Daffodil International University", 290, 140, GLUT_BITMAP_TIMES_ROMAN_24);

    // Instructions
    setColor(1.0, 1.0, 0.0); // Yellow
    drawBitmapText("Press ENTER to Start", 320, 80, GLUT_BITMAP_HELVETICA_18);
}

// --- Helper Functions to Draw Basic Shapes ---

// Draw a simple cube with specific dimensions
void drawCube(float width, float height, float depth) {
    glPushMatrix();
    glScalef(width, height, depth);
    glutSolidCube(1.0);
    glPopMatrix();
}

// Draw a cylinder (used for bamboo poles, cups, logs)
void drawCylinder(float radius, float height, float r, float g, float b) {
    setColor(r, g, b);
    GLUquadricObj *quadratic;
    quadratic = gluNewQuadric();
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // Rotate to stand upright
    gluCylinder(quadratic, radius, radius, height, 32, 32);
    glPopMatrix();
}

// --- Scene Objects ---

void drawGroundAndRoad() {
    // Grass color (darker at night)
    if (!isNight) setColor(0.13, 0.55, 0.13); // Forest Green
    else setColor(0.03, 0.12, 0.03); // very dark green for night
    glBegin(GL_QUADS);
        glVertex3f(-30.0, -2.0, -30.0);
        glVertex3f(30.0, -2.0, -30.0);
        glVertex3f(30.0, -2.0, 30.0);
        glVertex3f(-30.0, -2.0, 30.0);
    glEnd();

    // Gray Road with markings
    if(!isNight) setColor(0.3, 0.3, 0.3); // Dark Gray
    else setColor(0.08, 0.08, 0.08); // darker road at night
    glBegin(GL_QUADS);
        glVertex3f(-30.0, -1.95, 8.0);
        glVertex3f(30.0, -1.95, 8.0);
        glVertex3f(30.0, -1.95, 18.0);
        glVertex3f(-30.0, -1.95, 18.0);
    glEnd();

    // White Road Lines
    if(!isNight) setColor(1.0, 1.0, 1.0);
    else setColor(0.8, 0.75, 0.6); // dim yellowish markings at night
    for(float i=-25; i<30; i+=10) {
        glBegin(GL_QUADS);
            glVertex3f(i, -1.90, 12.8);
            glVertex3f(i+5, -1.90, 12.8);
            glVertex3f(i+5, -1.90, 13.2);
            glVertex3f(i, -1.90, 13.2);
        glEnd();
    }
}

void drawTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -2.0, z);

    // Trunk
    drawCylinder(0.8, 6.0, 0.4, 0.25, 0.1); // Brown

    // Leaves (Multiple spheres for bushy look)
        setColor(0.0, 0.4, 0.0); // Dark Green
        glTranslatef(0.0, 6.0, 0.0);

        // Compute a small sway amount per-tree so trees don't all move in sync
        float phase = (x + z) * 0.3f;
        float sway = sinf(leafAngle + phase) * 10.0f; // degrees

        // Central big sphere (slight sway)
        glPushMatrix();
            glRotatef(sway * 0.5f, 0.0f, 0.0f, 1.0f);
            glutSolidSphere(3.0, 15, 15);
        glPopMatrix();

        // Right/top sphere
        glPushMatrix();
            glTranslatef(1.5, 1.0, 0.0);
            glRotatef(sway, 0.0f, 0.0f, 1.0f);
            glutSolidSphere(2.5, 15, 15);
        glPopMatrix();

        // Left/front sphere
        glPushMatrix();
            glTranslatef(-1.5, 0.5, 1.0);
            glRotatef(-sway * 0.7f, 0.0f, 0.0f, 1.0f);
            glutSolidSphere(2.5, 15, 15);
        glPopMatrix();
    glPopMatrix();
}

void drawLampPost(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -2.0, z);

    // Pole
    drawCylinder(0.2, 10.0, 0.2, 0.2, 0.2); // Dark Gray

    // Lamp Head
    glTranslatef(0.0, 10.0, 0.0);
    setColor(1.0, 1.0, 0.8); // Light Yellow Bulb
    glutSolidSphere(0.8, 15, 15);

    // If night, draw glowing halo to simulate light
    if (isNight) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        setColor(1.0, 0.95, 0.7, 0.35);
        glPushMatrix();
        // slightly larger translucent sphere for glow
        glutSolidSphere(1.6, 10, 10);
        glPopMatrix();
        glDisable(GL_BLEND);
    }

    // Cap
    glTranslatef(0.0, 0.5, 0.0);
    setColor(0.1, 0.1, 0.1);
    glRotatef(-90, 1,0,0);
    glutSolidCone(1.0, 0.5, 15, 15);
    glPopMatrix();
}

void drawHangingSnacks() {
    // String
    setColor(0.1, 0.1, 0.1);
    glBegin(GL_LINES);
    glVertex3f(-3.5, 3.8, 3.5);
    glVertex3f(3.5, 3.8, 3.5);
    glEnd();

    // Chips Packets
    float positions[] = {-2.5, -1.0, 1.0, 2.5};
    for(int i=0; i<4; i++) {
        glPushMatrix();
        glTranslatef(positions[i], 3.4, 3.5);
        if(i % 2 == 0) setColor(1.0, 0.0, 0.0); // Red chips
        else setColor(1.0, 0.8, 0.0); // Yellow chips

        // Simple packet shape
        glScalef(0.6, 0.8, 0.1);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

void drawBiscuitJar() {
    glPushMatrix();
    glTranslatef(2.5, 0.6, 0.5); // Right side of table

    // Transparent Jar Body
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setColor(0.8, 0.9, 1.0, 0.4); // Light Blue transparent
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    GLUquadricObj *q = gluNewQuadric();
    gluCylinder(q, 0.6, 0.6, 1.2, 20, 20);
    glPopMatrix();

    glDisable(GL_BLEND);

    // Red Lid
    glTranslatef(0.0, 1.2, 0.0);
    setColor(0.8, 0.1, 0.1);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    GLUquadricObj *lid = gluNewQuadric();
    gluDisk(lid, 0.0, 0.6, 20, 1);
    glPopMatrix();

    // Biscuits inside (simple cylinders)
    glTranslatef(0.0, -0.8, 0.0);
    setColor(0.8, 0.6, 0.3);
    glutSolidSphere(0.4, 10, 10);

    glPopMatrix();
}

void drawStall() {
    // 1. Bamboo Pillars (Brown)
    float bambooR = 0.15f, bambooH = 6.0f;
    float r = 0.55, g = 0.35, b = 0.15;

    glPushMatrix(); glTranslatef(-4.0, -2.0, -2.0); drawCylinder(bambooR, bambooH, r, g, b); glPopMatrix();
    glPushMatrix(); glTranslatef(4.0, -2.0, -2.0); drawCylinder(bambooR, bambooH, r, g, b); glPopMatrix();
    glPushMatrix(); glTranslatef(-4.0, -2.0, 3.0); drawCylinder(bambooR, bambooH-1.0, r, g, b); glPopMatrix();
    glPushMatrix(); glTranslatef(4.0, -2.0, 3.0); drawCylinder(bambooR, bambooH-1.0, r, g, b); glPopMatrix();

    // 2. Tin Roof (Silver/Gray with Corrugated effect)
    glPushMatrix();
    glTranslatef(0.0, 4.0, 0.5);
    glRotatef(15.0, 1.0, 0.0, 0.0);

    setColor(0.7, 0.75, 0.8); // Base Tin color
    drawCube(9.0, 0.1, 7.0);

    // Corrugated lines on roof
    setColor(0.5, 0.55, 0.6); // Darker lines
    for(float i = -4.2; i <= 4.2; i+=0.3) {
        glBegin(GL_LINES);
            glVertex3f(i, 0.06, -3.4);
            glVertex3f(i, 0.06, 3.4);
        glEnd();
    }
    glPopMatrix();

    // Hanging Snacks
    drawHangingSnacks();

    // 3. Table (Wooden plank)
    setColor(0.55, 0.27, 0.07);
    glPushMatrix();
    glTranslatef(0.0, 0.0, 0.0);
    drawCube(7.0, 0.2, 2.0);
    glPopMatrix();

    // Table legs
    glPushMatrix(); glTranslatef(-3.0, -1.0, 0.0); drawCube(0.2, 2.0, 1.5); glPopMatrix();
    glPushMatrix(); glTranslatef(3.0, -1.0, 0.0); drawCube(0.2, 2.0, 1.5); glPopMatrix();

    // Add Biscuit Jar
    drawBiscuitJar();
}

void drawBench(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -1.0, z);
    setColor(0.4, 0.2, 0.0);
    drawCube(3.0, 0.2, 1.0);

    // Legs
    glPushMatrix(); glTranslatef(-1.2, -0.5, 0.0); drawCube(0.2, 1.0, 0.8); glPopMatrix();
    glPushMatrix(); glTranslatef(1.2, -0.5, 0.0); drawCube(0.2, 1.0, 0.8); glPopMatrix();
    glPopMatrix();
}

void drawKettleAndCups() {
    // Kettle on table
    glPushMatrix();
    glTranslatef(-1.5, 0.5, 0.0);
    setColor(0.8, 0.8, 0.8);
    glutSolidSphere(0.4, 20, 20);
    // Spout
    glPushMatrix();
    glTranslatef(0.3, 0.1, 0.0);
    glRotatef(45, 0.0, 0.0, 1.0);
    drawCylinder(0.1, 0.5, 0.7, 0.7, 0.7);
    glPopMatrix();
    // Handle
    setColor(0.1, 0.1, 0.1);
    glPushMatrix();
    glTranslatef(0.0, 0.5, 0.0);
    glutSolidTorus(0.05, 0.3, 10, 10);
    glPopMatrix();
    glPopMatrix();

    // Cups on table
    float cupX[] = {0.0, 1.0};
    for(int i=0; i<2; i++) {
        glPushMatrix();
        glTranslatef(cupX[i], 0.2, 0.2);
        glRotatef(-90, 1.0, 0.0, 0.0);
        setColor(1.0, 1.0, 1.0);
        glutSolidCone(0.15, 0.3, 10, 10);

        // Steam Animation
        setColor(0.9, 0.9, 0.9);
        glBegin(GL_LINES);
            glVertex3f(0.0, 0.0, 0.3 + steamY);
            glVertex3f(0.05, 0.05, 0.6 + steamY);
        glEnd();
        glPopMatrix();
    }
}

void drawHuman(float x, float y, float z, bool isShopkeeper) {
    glPushMatrix();
    glTranslatef(x, y, z);

    // Head
    setColor(0.87, 0.72, 0.53); // Skin color
    glPushMatrix();
    glTranslatef(0.0, 1.6, 0.0);
    glutSolidSphere(0.3, 20, 20);

    // Eyes
    setColor(0.0, 0.0, 0.0);
    glPushMatrix(); glTranslatef(0.1, 0.05, 0.25); glutSolidSphere(0.03, 10, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1, 0.05, 0.25); glutSolidSphere(0.03, 10, 10); glPopMatrix();

    glPopMatrix();

    // Body (Shirt)
    if(isShopkeeper) setColor(0.2, 0.2, 0.8); // Blue shirt
    else setColor(0.8, 0.2, 0.2); // Red shirt

    glPushMatrix();
    glTranslatef(0.0, 0.8, 0.0);
    drawCube(0.8, 1.2, 0.5);
    glPopMatrix();

    // Legs (Lungi/Pants)
    setColor(0.9, 0.9, 0.9);
    glPushMatrix();
    glTranslatef(-0.2, 0.0, 0.0);
    drawCube(0.25, 1.0, 0.3);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.2, 0.0, 0.0);
    drawCube(0.25, 1.0, 0.3);
    glPopMatrix();

    // Arms
    // Right Arm (Animated if Shopkeeper)
    setColor(0.87, 0.72, 0.53);
    glPushMatrix();
    glTranslatef(0.5, 1.2, 0.0);
    if(isShopkeeper) {
         glRotatef(angle, 0.0, 0.0, 1.0);
    }
    glRotatef(-15, 0.0, 0.0, 1.0);
    drawCube(0.15, 0.8, 0.15);
    glPopMatrix();

    // Left Arm
    glPushMatrix();
    glTranslatef(-0.5, 1.2, 0.0);
    glRotatef(15, 0.0, 0.0, 1.0);
    drawCube(0.15, 0.8, 0.15);
    glPopMatrix();

    glPopMatrix();
}

// --- Main Display Function ---
void display() {
    if(showLoading) {
        drawLoadingScreen();
    } else {
        // Switch back to 3D Perspective for the scene
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, 1.0, 1.0, 100.0);
        glMatrixMode(GL_MODELVIEW);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

    // Sky Color (change for night)
    if (!isNight) glClearColor(0.53, 0.81, 0.92, 1.0);
    else glClearColor(0.02, 0.03, 0.08, 1.0);

    // Camera controls (use cameraDist for zoom)
    float camX = cameraDist * sin(cameraAngle);
    float camZ = cameraDist * cos(cameraAngle);
        gluLookAt(camX, 6.0, camZ,  // Elevated eye position
                  0.0, 2.0, 0.0,   // Look slightly up
                  0.0, 1.0, 0.0);

        drawGroundAndRoad();
        drawTree(-10.0, -5.0);
        drawLampPost(10.0, 12.0);

    drawStall();
    drawShopName();
    drawMoon();
        drawKettleAndCups();

        // Shopkeeper inside
        drawHuman(0.0, -0.8, -1.0, true);

        // Pedestrians walking along the road
        for (size_t i = 0; i < pedestrians.size(); ++i) {
            drawHuman(pedestrians[i].x, -0.8f, pedestrians[i].z, false);
        }

        // Customers sitting on bench
        drawBench(0.0, 5.0);
        glPushMatrix();
        glRotatef(180, 0.0, 1.0, 0.0);
        drawHuman(-0.8, -0.3, -5.0, false);
        drawHuman(0.8, -0.3, -5.0, false);
        glPopMatrix();

        // Show "Paused" text if paused
        if(isPaused) {
            // Overlay 2D Text on 3D scene (disable depth test temporarily)
            glDisable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            gluOrtho2D(0, 800, 0, 600);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            setColor(1.0, 0.0, 0.0);
            drawBitmapText("PAUSED", 350, 550, GLUT_BITMAP_TIMES_ROMAN_24);

            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glEnable(GL_DEPTH_TEST);
        }
    }
    glutSwapBuffers();
}

// --- Animation Logic ---
void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS

    if(!showLoading && !isPaused) {
        // Animate Steam
        steamY += 0.005f;
        if(steamY > 0.2f) steamY = 0.0f;

        // Animate Shopkeeper Hand (Pouring motion)
        static bool armUp = true;
        if(armUp) {
            angle += 1.0f;
            if(angle > 45.0f) armUp = false;
        } else {
            angle -= 1.0f;
            if(angle < 0.0f) armUp = true;
        }

        // Update pedestrians
        for (size_t i = 0; i < pedestrians.size(); ++i) {
            pedestrians[i].x += pedestrians[i].speed;
            // wrap around screen edges
            if (pedestrians[i].x > 35.0f) pedestrians[i].x = -35.0f;
            if (pedestrians[i].x < -35.0f) pedestrians[i].x = 35.0f;
        }

        // Update leaf sway angle
        leafAngle += 0.03f; // small increment for smooth sway
        if (leafAngle > 10000.0f) leafAngle = 0.0f; // avoid overflow
    }
}

// --- Keyboard Input ---
void handleKeypress(unsigned char key, int x, int y) {
    if (key == 13) { // ENTER key
        showLoading = false;
    }
    if (key == ' ' || key == 'p' || key == 'P') { // Space or P for Pause
        isPaused = !isPaused;
    }
    if (key == 'n' || key == 'N') { // Toggle day/night
        isNight = !isNight;
    }
    if (key == 'd' || key == 'D') { // force day
        isNight = false;
    }
    // Zoom controls: '+' or '=' to zoom in, '-' to zoom out
    if (key == '+' || key == '=') {
        cameraDist -= 1.5f;
        if (cameraDist < 5.0f) cameraDist = 5.0f;
    }
    if (key == '-') {
        cameraDist += 1.5f;
        if (cameraDist > 80.0f) cameraDist = 80.0f;
    }
}

void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT)
        cameraAngle += 0.05f;
    else if (key == GLUT_KEY_LEFT)
        cameraAngle -= 0.05f;
    glutPostRedisplay();
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // initialize random seed and pedestrians
    srand((unsigned int)time(NULL));
    initPedestrians();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Bangladeshi Tea Stall Animation");

    init();

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutKeyboardFunc(handleKeypress); // Register standard keys
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}