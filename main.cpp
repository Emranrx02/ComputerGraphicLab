#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>


int winW = 1024;
int winH = 600;

float walker1_x = -150.0f;
float walker2_x = 1200.0f;
float cart_x = 1400.0f;
float sun_angle = 0.0f;
float cloud1_x = -300.0f;
float cloud2_x = 200.0f;
float bike_x = -200.0f;
float bird1_x = -200;
float bird2_x = -600;
float birdFlap = 0.0f;   // wing animation

// speeds / config
const float WALKER_SPEED = 0.7f;
const float CART_SPEED = 1.2f;
const float CLOUD_SPEED = 0.3f;
const float SUN_SPEED = 0.02f;
const float BIKE_SPEED = 2.1f; // initial bike speed (used to init bike_v)

// Bike control globals
float bike_v = BIKE_SPEED;            // current bike horizontal velocity
bool bike_left_pressed = false;
bool bike_right_pressed = false;

const float BIKE_CONTROL_SPEED = 3.5f;   // speed when user presses arrow
const float BIKE_MAX_SPEED = 4.0f;       // clamp for random speed
const float BIKE_RANDOM_ACCEL = 0.12f;   // randomness intensity
const float BIKE_FRICTION = 0.997f;      // friction factor when drifting

// --- forward declarations ---
void drawCart(float x, float y);
void drawBike(float x, float y, float scale);
void drawSeller(float x, float y, float scale);
void drawPerson(float x, float y, float scale, bool facingLeft);
void drawGoods(const char* type, float x, float y);
void drawShopsRow();
void reshape(int w, int h);

// NEW opposite-side shops forward declarations
void drawOppositeShop(float x, float y, float w, float h, float r, float g, float b);
void drawOppositeShopsRow();

// special key handlers
void specialDown(int key, int x, int y);
void specialUp(int key, int x, int y);

// ------------------------------------------------------------------
// Utility drawing helpers
// ------------------------------------------------------------------
void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
      glVertex2f(x, y);
      glVertex2f(x + w, y);
      glVertex2f(x + w, y + h);
      glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segments = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}

// draw bitmap text centered at (cx,cy)
void drawBitmapTextCentered(void* font, float cx, float cy, const char* text) {
    if (!text || text[0] == '\0') return;
    int totalW = 0;
    for (const char* p = text; *p; ++p) {
        totalW += glutBitmapWidth(font, (int)(unsigned char)(*p));
    }
    float startX = cx - totalW / 2.0f;
    float startY = cy - 6.0f;
    glRasterPos2f(startX, startY);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(font, (int)(unsigned char)(*p));
    }
}
void drawBird(float x, float y, float flap, float scale = 1.0f) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    // Body
    glColor3f(0, 0, 0);
    glBegin(GL_POLYGON);
        glVertex2f(0, 0);
        glVertex2f(18, 4);
        glVertex2f(26, 0);
        glVertex2f(18, -4);
    glEnd();

    // Beak
    glColor3f(1.0f, 0.8f, 0.3f);
    glBegin(GL_TRIANGLES);
        glVertex2f(26, 0);
        glVertex2f(32, 2);
        glVertex2f(32, -2);
    glEnd();

    // Wings (beautiful flapping motion)
    float wingUp = 16.0f + sinf(flap) * 10.0f;   // animated
    float wingDn = -16.0f - sinf(flap) * 10.0f;  // opposite side

    glColor3f(0, 0, 0);

    glBegin(GL_TRIANGLES);
        // upper wing
        glVertex2f(10, 0);
        glVertex2f(-4, wingUp);
        glVertex2f(6, 0);

        // lower wing
        glVertex2f(10, 0);
        glVertex2f(-4, wingDn);
        glVertex2f(6, 0);
    glEnd();

    glPopMatrix();
}


// ------------------------------------------------------------------
// Characters / Sellers / Goods
// ------------------------------------------------------------------
void drawPerson(float x, float y, float scale, bool facingLeft) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    // ------------------
    // HEAD
    // ------------------
    glColor3f(1.0f, 0.88f, 0.65f);
    drawCircle(0, 32, 10);

    // ------------------
    // BODY (shirt)
    // ------------------
    glColor3f(0.2f, 0.45f, 0.85f);
    drawRect(-6, 6, 12, 18);

    // ------------------
    // LEGS
    // ------------------
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_LINES);
      glVertex2f(-3, 6);
      glVertex2f(-3, -10);

      glVertex2f(3, 6);
      glVertex2f(3, -10);
    glEnd();

    // ------------------
    // HANDS
    // ------------------
    glColor3f(1.0f, 0.88f, 0.65f);
    glBegin(GL_LINES);
      // left hand
      glVertex2f(-6, 18);
      glVertex2f(-14, 10);

      // right hand
      glVertex2f(6, 18);
      glVertex2f(14, 10);
    glEnd();

    // ------------------
    // SHOPPING BAG (auto left/right depending on facingLeft)
    // ------------------
    if (!facingLeft) {
        // Face right → bag in right hand
        glColor3f(0.95f, 0.85f, 0.35f);   // bag color
        drawRect(14, 10 - 12, 12, 16);    // bag body

        glColor3f(0.55f, 0.34f, 0.05f);   // handle
        glBegin(GL_LINES);
          glVertex2f(14 + 6, 20);
          glVertex2f(14 + 6, 10 - 12);
        glEnd();
    }
    else {
        // Face left → bag in left hand
        glColor3f(0.95f, 0.85f, 0.35f);
        drawRect(-26, 10 - 12, 12, 16);

        glColor3f(0.55f, 0.34f, 0.05f);
        glBegin(GL_LINES);
          glVertex2f(-26 + 6, 20);
          glVertex2f(-26 + 6, 10 - 12);
        glEnd();
    }

    glPopMatrix();
}


void drawGoods(const char* type, float x, float y) {
    if (!type) return;
    if (strcmp(type, "veg") == 0) {
        glColor3f(0.1f, 0.6f, 0.2f);     // vegetable crate
        drawRect(x, y, 44, 18);
        glColor3f(0.85f, 0.55f, 0.15f);  // a pumpkin
        drawCircle(x + 56, y + 10, 8);
    }
    else if (strcmp(type, "fruit") == 0) {
        glColor3f(1.0f, 0.3f, 0.3f);     // apples
        drawCircle(x + 6, y + 10, 7);
        glColor3f(1.0f, 0.8f, 0.1f);     // mango
        drawCircle(x + 26, y + 10, 7);
        glColor3f(1.0f, 0.55f, 0.15f);   // another fruit box
        drawRect(x + 44, y, 24, 18);
    }
    else if (strcmp(type, "cloth") == 0) {
        glColor3f(0.2f, 0.4f, 0.8f);
        drawRect(x, y, 44, 20);          // folded cloth
        glColor3f(0.8f, 0.2f, 0.4f);
        drawRect(x + 48, y, 44, 20);
    }
    else if (strcmp(type, "bakery") == 0) {
        glColor3f(0.85f, 0.65f, 0.35f);  // bread
        drawRect(x, y, 44, 16);
        glColor3f(0.95f, 0.85f, 0.55f);  // cake
        drawRect(x + 48, y, 44, 16);
    }
    else if (strcmp(type, "milk") == 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawRect(x, y, 18, 28);          // milk bottles
        drawRect(x + 22, y, 18, 28);
    }
    else {
        glColor3f(0.8f, 0.55f, 0.35f);
        drawRect(x, y, 40, 18);
    }
}

// ------------------------------------------------------------------
// Shop drawing (with nicer awning + sign text)
// ------------------------------------------------------------------
void drawShop(float x, float y, float w, float h, float r, float g, float b, const char* sign = nullptr) {
    // Main building body
    glColor3f(r, g, b);
    drawRect(x, y, w, h);

    // Top roof rim (dark strip)
    glColor3f(0.08f, 0.08f, 0.08f);
    drawRect(x - 4, y + h, w + 8, h * 0.12f);

    // Roof edge highlight
    glColor3f(0.22f, 0.22f, 0.22f);
    drawRect(x - 4, y + h + h*0.12f - 6, w + 8, 4);

    // Awning (striped)
    float awH = h * 0.22f;
    float awY = y + h - 6.0f;
    int stripes = 6;
    float stripeW = w / float(stripes);
    for (int i = 0; i < stripes; ++i) {
        if (i % 2 == 0) glColor3f(0.85f, 0.15f, 0.15f);
        else glColor3f(1.0f, 0.55f, 0.35f);
        drawRect(x + i * stripeW, awY, stripeW + 1, awH);
    }

    // Awning front scallop
    float scallopY = awY - 6.0f;
    for (int i = 0; i < stripes; ++i) {
        float cx = x + i * stripeW + stripeW * 0.5f;
        float radius = stripeW * 0.38f;
        if (i % 2 == 0) glColor3f(0.7f, 0.12f, 0.12f);
        else glColor3f(0.95f, 0.48f, 0.30f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, scallopY);
        for (int s = 0; s <= 18; ++s) {
            float theta = 3.1415926f * float(s) / 18.0f;
            float sx = radius * cosf(theta);
            float sy = -radius * sinf(theta);
            glVertex2f(cx + sx, scallopY + sy);
        }
        glEnd();
    }

    // Awning shadow
    glColor3f(0.12f, 0.12f, 0.12f);
    drawRect(x + 2, awY - 4, w - 4, 6);

    // Door
    glColor3f(0.28f, 0.14f, 0.04f);
    float doorW = w * 0.26f;
    float doorH = h * 0.52f;
    drawRect(x + w * 0.68f - 6, y, doorW, doorH);
    glColor3f(0.95f, 0.85f, 0.3f);
    drawCircle(x + w * 0.68f + doorW - 12, y + doorH * 0.45f, 3);

    // Windows
    float winWw = w * 0.22f;
    float winHh = h * 0.28f;
    glColor3f(0.92f, 0.97f, 1.0f);
    drawRect(x + 8, y + h*0.55f, winWw, winHh);
    drawRect(x + w*0.28f, y + h*0.55f, winWw, winHh);
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_LINE_LOOP);
      glVertex2f(x + 8, y + h*0.55f);
      glVertex2f(x + 8 + winWw, y + h*0.55f);
      glVertex2f(x + 8 + winWw, y + h*0.55f + winHh);
      glVertex2f(x + 8, y + h*0.55f + winHh);
    glEnd();
    glBegin(GL_LINE_LOOP);
      glVertex2f(x + w*0.28f, y + h*0.55f);
      glVertex2f(x + w*0.28f + winWw, y + h*0.55f);
      glVertex2f(x + w*0.28f + winWw, y + h*0.55f + winHh);
      glVertex2f(x + w*0.28f, y + h*0.55f + winHh);
    glEnd();

    // Signboard
    float signH = 18;
    float signX = x + 6;
    float signY = y + h + 2;
    glColor3f(0.06f, 0.06f, 0.06f);
    drawRect(signX, signY, w - 12, signH);
    glColor3f(0.95f, 0.95f, 0.96f);
    drawRect(signX + 6, signY + 4, w - 24, signH - 8);

    // Text on sign
    if (sign && sign[0] != '\0') {
        glColor3f(0.07f, 0.07f, 0.07f);
        void* font = GLUT_BITMAP_HELVETICA_12;
        float centerX = signX + (w - 12) * 0.5f;
        float centerY = signY + signH * 0.5f + 2;
        drawBitmapTextCentered(font, centerX, centerY, sign);
    }
}


// ------------------------------------------------------------------
// Opposite-side shops (new) - simple back-side view shops
// ------------------------------------------------------------------
void drawOppositeShop(float x, float y, float w, float h, float r, float g, float b) {
    // building body (slightly desaturated so it looks a bit distant)
    glColor3f(r * 0.92f, g * 0.92f, b * 0.92f);
    drawRect(x, y, w, h);

    // roof rim
    glColor3f(0.08f, 0.08f, 0.08f);
    drawRect(x - 3.0f, y + h, w + 6.0f, h * 0.12f);

    // two small horizontal windows on the back
    glColor3f(0.9f, 0.96f, 1.0f);
    float winW = w * 0.30f;
    float winH = h * 0.18f;
    drawRect(x + w*0.10f, y + h*0.56f, winW, winH);
    drawRect(x + w*0.60f, y + h*0.56f, winW, winH);

    // subtle horizontal lines for texture
    glColor3f(0.12f, 0.12f, 0.12f);
    int lines = 3;
    for (int i = 1; i <= lines; ++i) {
        float ly = y + h * (0.25f + 0.12f * i);
        glBegin(GL_LINES);
          glVertex2f(x + 6.0f, ly);
          glVertex2f(x + w - 6.0f, ly);
        glEnd();
    }
}

void drawOppositeShopsRow() {
    int shopCount = 7;
    float w = 160.0f;   // keep same width to match front shops
    float h = 110.0f;   // same height if you want identical size (or reduce)
    float gap = 18.0f;

    float totalWidth = shopCount * w + (shopCount - 1) * gap;
    float startX = (winW - totalWidth) / 2.0f;

    // place them BELOW the road: estimate road bottom = winH*0.15f + 20
    // baseYOpp chosen so shops sit on the sandy foreground
    float baseYOpp = winH * 0.15f - h - 6.0f; // tweak -6 if needed

    if (baseYOpp < 6.0f) baseYOpp = 6.0f; // keep visible on small heights

    // colors matching your front shops (reuse same palette)
    float cols[7][3] = {
        {0.9f, 0.4f, 0.3f},
        {0.15f, 0.6f, 0.35f},
        {0.2f, 0.5f, 0.8f},
        {0.85f, 0.55f, 0.2f},
        {0.55f, 0.2f, 0.4f},
        {0.3f, 0.55f, 0.75f},
        {0.85f, 0.35f, 0.25f}
    };

    for (int i = 0; i < shopCount; ++i) {
        float sx = startX + i * (w + gap);
        drawOppositeShop(sx, baseYOpp, w, h, cols[i][0], cols[i][1], cols[i][2]);
    }
}

// ------------------------------------------------------------------
// Trees, Sun, Clouds
// ------------------------------------------------------------------
void drawTree(float x, float y, float scale = 3.0f) {   // default size = 3x bigger
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    // trunk
    glColor3f(0.45f, 0.24f, 0.07f);
    drawRect(-8, 0, 16, 60);  // thicker + taller trunk

    // leaves (bigger circles)
    glColor3f(0.05f, 0.6f, 0.08f);
    drawCircle(0, 70, 35);
    drawCircle(-28, 52, 30);
    drawCircle(28, 52, 30);

    glPopMatrix();
}

// Option A: simple core + soft halo
void drawSunSimple(float cx, float cy) {
    // soft halo (single ring, subtle)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.94f, 0.40f, 0.12f); // faint warm halo
    drawCircle(cx, cy, 80.0f, 48);

    // slightly brighter inner halo
    glColor4f(1.0f, 0.96f, 0.45f, 0.18f);
    drawCircle(cx, cy, 48.0f, 48);

    // core
    glColor3f(1.0f, 0.96f, 0.35f);
    drawCircle(cx, cy, 28.0f, 48);
}


void drawCloud(float cx, float cy, float scale = 1.0f) {
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glScalef(scale, scale, 1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(-30, 0, 22);
    drawCircle(-10, 12, 26);
    drawCircle(10, 0, 20);
    drawCircle(30, 8, 18);
    glPopMatrix();
}

// ------------------------------------------------------------------
// Vehicles (definitions here)
// ------------------------------------------------------------------
// drawCart: second parameter is wheel-center Y
// drawPrivateCar: x = left-most x of car body, wheelCY = wheel center Y
// r,g,b = body color (0..1)
void drawPrivateCar(float x, float wheelCY, float r, float g, float b) {
    // sizing
    float wheelR = 16.0f;           // wheel radius
    float carW = 160.0f;            // car body width
    float carH = 36.0f;             // main body height
    float roofH = 22.0f;            // roof height above body
    // compute platform y relative to wheel center
    float bodyY = wheelCY - wheelR + 6.0f; // body sits slightly above wheels

    // shadow under car
    glColor3f(0.12f, 0.12f, 0.12f);
    drawRect(x + 8, bodyY - 10.0f, carW - 16.0f, 8.0f);

    // main car body (rounded-ish using rect + small top trapezoid)
    glColor3f(r, g, b);
    // lower body
    drawRect(x, bodyY, carW, carH);

    // upper cabin (slanted/rounded roof)
    glBegin(GL_POLYGON);
      glVertex2f(x + carW*0.18f, bodyY + carH);
      glVertex2f(x + carW*0.34f, bodyY + carH + roofH*0.4f);
      glVertex2f(x + carW*0.66f, bodyY + carH + roofH*0.4f);
      glVertex2f(x + carW*0.82f, bodyY + carH);
    glEnd();

    // windows (slightly darker tint)
    glColor3f(0.85f, 0.92f, 0.97f);
    // front window
    drawRect(x + carW*0.36f, bodyY + carH*0.45f, carW*0.22f, carH*0.45f);
    // rear window
    drawRect(x + carW*0.60f, bodyY + carH*0.45f, carW*0.14f, carH*0.45f);

    // window divider lines (black thin)
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_LINES);
      glVertex2f(x + carW*0.36f, bodyY + carH*0.45f + carH*0.225f);
      glVertex2f(x + carW*0.58f, bodyY + carH*0.45f + carH*0.225f);
    glEnd();

    // door line
    glBegin(GL_LINES);
      glVertex2f(x + carW*0.46f, bodyY + 6.0f);
      glVertex2f(x + carW*0.46f, bodyY + carH - 4.0f);
    glEnd();

    // bumper/front & rear with silver color
    glColor3f(0.82f, 0.82f, 0.84f);
    drawRect(x - 6.0f, bodyY + 6.0f, 8.0f, carH - 12.0f);              // front little bumper
    drawRect(x + carW - 2.0f, bodyY + 6.0f, 8.0f, carH - 12.0f);      // rear little bumper

    // headlights (front)
    glColor3f(1.0f, 0.95f, 0.6f);
    drawRect(x + 4.0f, bodyY + carH*0.45f, 8.0f, 10.0f);

    // taillight (rear)
    glColor3f(0.9f, 0.18f, 0.18f);
    drawRect(x + carW - 14.0f, bodyY + carH*0.45f, 8.0f, 10.0f);

    // wheels (centered vertically at wheelCY)
    glColor3f(0.07f, 0.07f, 0.07f);
    drawCircle(x + 30.0f, wheelCY, wheelR);
    drawCircle(x + carW - 30.0f, wheelCY, wheelR);
    // inner rims
    glColor3f(0.35f, 0.35f, 0.35f);
    drawCircle(x + 30.0f, wheelCY, 7.0f);
    drawCircle(x + carW - 30.0f, wheelCY, 7.0f);

    // little roof highlight line
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
      glVertex2f(x + carW*0.22f, bodyY + carH + roofH*0.22f);
      glVertex2f(x + carW*0.78f, bodyY + carH + roofH*0.22f);
    glEnd();
}


// drawBike: second parameter is wheel-center Y (definition has default)
void drawBike(float x, float wheelCY, float scale = 1.0f) {
    glPushMatrix();
    // translate so that wheel centers line up with wheelCY
    glTranslatef(x, wheelCY, 0);
    glScalef(scale, scale, 1.0f);

    // wheels at (0,0) and (60,0)
    glColor3f(0.08f, 0.08f, 0.08f);
    drawCircle(0, 0, 16);
    drawCircle(60, 0, 16);

    // spokes (simple)
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_LINES);
      glVertex2f(0, 0); glVertex2f(0, 9);
      glVertex2f(60, 0); glVertex2f(60, 9);
    glEnd();

    // frame - warm yellow (holud)
    glColor3f(0.95f, 0.75f, 0.12f);
    glBegin(GL_LINES);
      glVertex2f(0, 0); glVertex2f(24, 6);
      glVertex2f(24, 6); glVertex2f(40, 6);
      glVertex2f(40, 6); glVertex2f(60, 0);
      glVertex2f(24, 6); glVertex2f(24, 18);
      glVertex2f(24, 18); glVertex2f(44, 20);
    glEnd();

    // handlebar
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_LINES);
      glVertex2f(44, 20); glVertex2f(54, 26);
      glVertex2f(54, 26); glVertex2f(58, 26);
    glEnd();

    // seat
    glColor3f(0.1f, 0.1f, 0.1f);
    drawRect(18, 22, 18, 6);

    // rider torso
    glColor3f(0.9f, 0.5f, 0.2f);
    drawRect(18, 30, 12, 20);  // body above seat
    // head
    glColor3f(1.0f, 0.9f, 0.75f);
    drawCircle(24, 56, 8);
    // helmet / cap
    glColor3f(0.15f, 0.15f, 0.15f);
    drawRect(16, 62, 16, 4);

    // arms holding handlebar
    glColor3f(0.9f, 0.5f, 0.2f);
    glBegin(GL_LINES);
      glVertex2f(20, 44); glVertex2f(36, 34);
      glVertex2f(28, 44); glVertex2f(50, 30);
    glEnd();

    // small backpack / basket behind seat
    glColor3f(0.85f, 0.55f, 0.15f);
    drawRect(-22, 36, 14, 12);

    glPopMatrix();
}
// seller (simple shopkeeper) - definition (wheel-center style not required)
void drawSeller(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    // body
    glColor3f(0.92f, 0.5f, 0.25f);
    drawRect(-10, 0, 20, 26);

    // head
    glColor3f(1.0f, 0.90f, 0.72f);
    drawCircle(0, 38, 12);

    // cap/hair
    glColor3f(0.18f, 0.18f, 0.18f);
    drawRect(-12, 46, 24, 6);

    // small table in front of seller
    glColor3f(0.65f, 0.4f, 0.2f);
    drawRect(-30, -10, 60, 10);

    glPopMatrix();
}

// ------------------------------------------------------------------
// Scene base & shops row
// ------------------------------------------------------------------
void drawSceneBase() {
    // sky
    glColor3f(0.62f, 0.87f, 0.98f);
    drawRect(0, winH*0.35f, winW, winH*0.65f);

    // distant fields / hills
    glColor3f(0.45f, 0.75f, 0.4f);
    drawRect(0, winH*0.28f, winW, winH*0.10f);
    glColor3f(0.35f, 0.62f, 0.32f);
    drawRect(0, winH*0.15f, winW, winH*0.13f);

    // ground foreground
    glColor3f(0.9f, 0.85f, 0.68f);
    drawRect(0, 0, winW, winH*0.15f);

    // road
    glColor3f(0.28f, 0.28f, 0.28f);
    drawRect(0, winH*0.15f + 20, winW, 68);
    glColor3f(1.0f, 0.9f, 0.25f);
    for (int i = -50; i < winW + 50; i += 80) {
        drawRect(i + 10, winH*0.15f + 50, 50, 6);
    }
}

// draw a row of shops (CENTERED + 7 SHOPS) with shop names and place sellers/customers/goods
void drawShopsRow() {
    // baseY = top of shop area (behind the road)
    float baseY = winH*0.15f + 150; // same as used elsewhere

    int shopCount = 7;
    float w = 160;
    float h = 110;
    float gap = 18;

    float totalWidth = shopCount * w + (shopCount - 1) * gap;
    float startX = (winW - totalWidth) / 2.0f;

    const char* names[7] = {
        "Pabna", "Jamalpur", "Nouga",
        "Chandpur", "Bogura", "Dhaka", "Rongpur"
    };

    const char* goodsType[7] = {
        "bakery", "veg", "fruit", "milk", "cloth", "bakery", "bakery"
    };

    // sidewalk Y: where customers/walkers should walk (just in front of shops, above road)
    float sidewalkY = baseY - 24.0f; // adjust +/- a few pixels if needed

    // draw shops
    for (int i = 0; i < shopCount; ++i) {
        float sx = startX + i * (w + gap);
        // pick colors for variety and draw
        switch (i) {
            case 0: drawShop(sx, baseY, w, h, 0.9f, 0.4f, 0.3f, names[i]); break;
            case 1: drawShop(sx, baseY, w, h, 0.15f, 0.6f, 0.35f, names[i]); break;
            case 2: drawShop(sx, baseY, w, h, 0.2f, 0.5f, 0.8f, names[i]); break;
            case 3: drawShop(sx, baseY, w, h, 0.85f, 0.55f, 0.2f, names[i]); break;
            case 4: drawShop(sx, baseY, w, h, 0.55f, 0.2f, 0.4f, names[i]); break;
            case 5: drawShop(sx, baseY, w, h, 0.3f, 0.55f, 0.75f, names[i]); break;
            default: drawShop(sx, baseY, w, h, 0.85f, 0.35f, 0.25f, names[i]); break;
        }
    }

    // draw sellers at shop fronts, customers on sidewalk, goods in front of shops
    for (int i = 0; i < shopCount; ++i) {
        float sx = startX + i * (w + gap);
        // seller stands near left-front of shop
        drawSeller(sx + 45, baseY - 14, 1.0f);
        // customer now stands/walks on the sidewalk (so they won't go onto the road)
        drawPerson(sx + 105, sidewalkY, 0.9f, true);
        // goods displayed on the ground in front of shop (closer to shop)
        drawGoods(goodsType[i], sx + 18, baseY - 40);
    }
}

// ------------------------------------------------------------------
// Display / reshape / animation
// ------------------------------------------------------------------
// Y position for objects that must stay on the road (adjust if you changed road height)
float roadY = 0.0f; // not strictly necessary but kept for clarity

// display() — pedestrians stay on sidewalk (behind shops), vehicles stay on road
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // --- background / base ---
    drawSceneBase();

    // --- sun & clouds ---
     float cx = winW*0.75f;
    float cy = winH*0.8f;
    float radius = 120.0f;
    float sunx = cx + radius * cosf(sun_angle);
    float suny = cy + radius * sinf(sun_angle);

    // draw sunlight glow & rays (before drawing scene objects so light sits behind)
   drawSunSimple(sunx, suny);
    drawCloud(cloud1_x, winH * 0.86f, 1.0f);
    drawCloud(cloud2_x, winH * 0.92f, 0.85f);

    // --- large trees at edges ---
    drawTree(50, winH*0.15f + 88 + 10, 3.2f);
    drawTree(winW - 50, winH*0.15f + 88 + 10, 3.2f);

    // --- OPPOSITE-SIDE SHOPS (NEW) ---
    drawOppositeShopsRow();

    // --- shops row ---
    drawShopsRow();

    // --- compute bench values (must match drawShopsRow's baseY) ---
    float shopBaseY = winH*0.15f + 150.0f;      // same baseY used inside drawShopsRow()
    // sidewalk Y: people walk here (a bit below shop front)
    float sidewalkY = shopBaseY - 24.0f;        // tweak if you want them closer/further
    // road bottom and wheel baseline (vehicles)
    float roadBottom = winH * 0.15f + 20.0f;
    float roadWheelY = roadBottom + 14.0f;

    // dashed center (approx) so bike can ride on dashes
    float dashedCenterY = winH*0.15f + 50.0f + 3.0f;
    float bikeWheelY = dashedCenterY - 3.0f; // slight tweak

    // birds
    drawBird(bird1_x, winH*0.80f, birdFlap, 1.0f);
    drawBird(bird2_x, winH*0.87f, birdFlap, 1.2f);

    // --- pedestrians on sidewalk (walk left/right, never on road) ---
    drawPerson(walker1_x, sidewalkY, 0.9f, false);
    drawPerson(walker2_x, sidewalkY - 6.0f, 0.9f, true); // slight vertical variety

    // --- vehicles on road (wheels centered at roadWheelY) ---
    drawPrivateCar(cart_x, roadWheelY, 0.17f, 0.18f, 0.72f); // রঙ হিসেবে এখানে blue-ish

    drawBike(bike_x, bikeWheelY, 0.95f);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    winW = w;
    winH = h;
    glViewport(0, 0, winW, winH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// update() — keep walkers wrapped across screen but always on sidewalk X-line
void update(int value) {
    // walkers horizontal movement (they walk along sidewalk)
    walker1_x += WALKER_SPEED;
    if (walker1_x > winW + 60) walker1_x = -120.0f; // re-enter left when off-right

    walker2_x -= WALKER_SPEED * 0.9f;
    if (walker2_x < -160) walker2_x = winW + 80; // re-enter right when off-left

    // cart moves
    cart_x -= CART_SPEED;
    if (cart_x < -260) cart_x = winW + 200;

    // --- bike movement: user control overrides random drifting ---
    if (bike_left_pressed && !bike_right_pressed) {
        bike_v = -BIKE_CONTROL_SPEED;
    } else if (bike_right_pressed && !bike_left_pressed) {
        bike_v = BIKE_CONTROL_SPEED;
    } else {
        // random drift with small accel, friction and clamping
        float r = (float)(rand() % 201 - 100) / 100.0f; // -1.0 .. +1.0
        float accel = r * BIKE_RANDOM_ACCEL;
        bike_v += accel;
        // friction
        bike_v *= BIKE_FRICTION;
        // clamp
        if (bike_v > BIKE_MAX_SPEED) bike_v = BIKE_MAX_SPEED;
        if (bike_v < -BIKE_MAX_SPEED) bike_v = -BIKE_MAX_SPEED;
    }

    bike_x += bike_v;
    // wrapping so it re-enters screen smoothly
    if (bike_x > winW + 360) bike_x = -360;
    if (bike_x < -360) bike_x = winW + 360;

    // clouds & sun
    cloud1_x += CLOUD_SPEED;
    cloud2_x += CLOUD_SPEED * 0.6f;
    if (cloud1_x > winW + 200) cloud1_x = -400;
    if (cloud2_x > winW + 200) cloud2_x = -400;

    sun_angle += SUN_SPEED * 0.004f;
    if (sun_angle > 2.0f * 3.1415926f) sun_angle -= 2.0f * 3.1415926f;
// wing animation
birdFlap += 0.25f;
if (birdFlap > 6.28f) birdFlap = 0;

// movement
bird1_x += 2.2f;
bird2_x += 1.6f;

if (bird1_x > winW + 200) bird1_x = -300;
if (bird2_x > winW + 300) bird2_x = -500;



    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void initGL() {
    glClearColor(0.62f, 0.87f, 0.98f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
}

// special key down (arrow keys)
void specialDown(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) bike_left_pressed = true;
    else if (key == GLUT_KEY_RIGHT) bike_right_pressed = true;
}

// special key up (arrow release)
void specialUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) bike_left_pressed = false;
    else if (key == GLUT_KEY_RIGHT) bike_right_pressed = false;
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL)); // seed RNG for bike random behaviour

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Village Hat-Bazar - Animated Scene");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    // register special key handlers (arrow keys)
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
