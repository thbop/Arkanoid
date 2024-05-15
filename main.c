#include "raylib.h"
#include "raymath.h"
#include "string.h"
#include "stdio.h"

#define WIDTH          600
#define HEIGHT         800
#define PADDLESPEED    5.0f
#define BLOCKCOUNT     256
#define BLOCKCOUNTSQRT 16
#define STARTBALLPOS   (Vector2){ 300.0f, 600.0f }
#define STARTBALLVEL   (Vector2){ 1.0f, 5.0f }
#define STARTPADDLEREC (Rectangle){ 250.0f, 700.0f, 100.0f, 10.0f }
#define STARTBALLS     3


// Create a type symbol for unsigned characters
typedef unsigned char u8;

float Q_rsqrt( float number ) {
    // Don't ask, watch this: https://youtu.be/p8u_k2LIZyo?si=n8mRBL3u3PjkUHsR
    long i;
    float x2, y;
    
    x2 = number * 0.5f;
    y = number;
    i = *( long* ) &y;
    i = 0x5F3759DF - ( i >> 1 );
    y = *(float*) &i;
    y *= ( 1.5f - ( x2 * y*y ) );

    return y;
}

Vector2 Vector2MultiplyValue( Vector2 p, float v ) {
    return (Vector2){ p.x * v, p.y * v };
}

struct {
    u8 states[BLOCKCOUNT];
    Rectangle rec;
} blocks;

struct {
    float radius;
    Vector2 pos; // position
    Vector2 vel; // velocity
} ball;

struct {
    Rectangle rec;
    float xvel; // x velocity
} paddle;

struct {
    int balls, blocks;
    u8 state; // 0 = playing, 1 = win, 2 = lose
} score;

void moveBall() {
    // Move
    ball.pos = Vector2Add( ball.pos, ball.vel );

    // Check wall collisions
    if ( ball.pos.x < ball.radius || ball.pos.x > WIDTH  - ball.radius ) ball.vel.x = -ball.vel.x;
    if ( ball.pos.y < ball.radius || ball.pos.y > HEIGHT - ball.radius ) ball.vel.y = -ball.vel.y;
}


void movePaddle() {
    // Keyboard input
    if      ( IsKeyDown( KEY_RIGHT ) ) paddle.xvel =  PADDLESPEED;
    else if ( IsKeyDown( KEY_LEFT  ) ) paddle.xvel = -PADDLESPEED;
    else                               paddle.xvel =  0.0f;
    
    // Move
    paddle.rec.x += paddle.xvel;

    // Check wall collisions
    if ( paddle.rec.x < 0.0f )                          paddle.rec.x += PADDLESPEED;
    else if ( paddle.rec.x > WIDTH - paddle.rec.width ) paddle.rec.x -= PADDLESPEED;
}


void botMovePaddle() {
    if ( ball.vel.y > 0.0f ) {
        float rmag = Q_rsqrt( ball.vel.x*ball.vel.x + ball.vel.y*ball.vel.y );
        Vector2
            dir = Vector2MultiplyValue( ball.vel, rmag ),
            point = Vector2MultiplyValue( dir, (paddle.rec.y-ball.pos.y) / dir.y );
        
        float
            paddleXCenter = paddle.rec.x + (paddle.rec.width*0.5f),
            impactX = point.x+ball.pos.x;
        if      ( impactX > paddleXCenter+30 ) paddle.rec.x += PADDLESPEED;
        else if ( impactX < paddleXCenter-30 ) paddle.rec.x -= PADDLESPEED;
        
        if ( abs((int)(impactX-paddleXCenter)) < 5 ) paddle.rec.x += PADDLESPEED * GetRandomValue(-1, 1);

        // Check wall collisions
        if ( paddle.rec.x < 0.0f )                          paddle.rec.x += PADDLESPEED;
        else if ( paddle.rec.x > WIDTH - paddle.rec.width ) paddle.rec.x -= PADDLESPEED;

        DrawCircle( (int)(ball.pos.x+point.x), 700, 5, RED );

    }
}

void resetBallPaddle() {
    ball.pos   = STARTBALLPOS;
    ball.vel   = STARTBALLVEL;
    paddle.rec = STARTPADDLEREC;
}

void paddleBallCollide() {
    if ( CheckCollisionCircleRec( ball.pos, ball.radius, paddle.rec ) ) {
        // b.dx = (b.x - p.x+(p.width/2))/10; b.dx should be between -5 and 5 depending on collision point
        ball.vel.x = ( ball.pos.x - ( paddle.rec.x + ( paddle.rec.width * 0.5f ) ) ) * 0.1f;
        ball.vel.y = -ball.vel.y;
    }
}

// u8 blockBallCollide( Rectangle rec ) {
//     u8 state = 1;
//     if ( CheckCollisionCircleRec( ball.pos, ball.radius, rec ) ) {
//         state = 0;
//         if ( ball.pos.y < rec.y || ball.pos.y > rec.y+rec.height ) ball.vel.y = -ball.vel.y;
//         if ( ball.pos.x < rec.x || ball.pos.x > rec.x+rec.width  ) ball.vel.x = -ball.vel.x;
//     }

//     return state;
// }

typedef struct collideData {
    u8 state, xHit, yHit;
} collideData;

collideData blockBallCollide( Rectangle rec, u8 xMask, u8 yMask ) {
    collideData data = { 1, xMask, yMask };
    if ( CheckCollisionCircleRec( ball.pos, ball.radius, rec ) ) {
        data.state = 0;
        if ( ( ball.pos.x < rec.x || ball.pos.x > rec.x+rec.width  ) && !xMask ) { ball.vel.x = -ball.vel.x; data.xHit = 1; }
        if ( ( ball.pos.y < rec.y || ball.pos.y > rec.y+rec.height ) && !yMask ) { ball.vel.y = -ball.vel.y; data.yHit = 1; }
    }

    return data;
}

void updateScore() {
    score.blocks = 0;
    for ( int i = 0; i < BLOCKCOUNT; i++ ) if ( blocks.states[i] ) score.blocks++;

    if ( !score.blocks ) score.state = 1; // win

    if ( ball.pos.y > HEIGHT - 10.0f ) {
        score.balls--;
        resetBallPaddle();
        WaitTime(0.7);
        if ( score.balls <= 0 ) score.state = 2; // lose
    }
}

void update() {
    updateScore();
    if ( !score.state ) {
        // movePaddle();
        moveBall();
        paddleBallCollide();
    }
    
}

void runBlocks() {
    collideData data = { 1, 0, 0 };
    for ( int j = 0; j < BLOCKCOUNTSQRT; j++ ) {
        for ( int i = 0; i < BLOCKCOUNTSQRT; i++ ) {
            if ( blocks.states[j*BLOCKCOUNTSQRT + i] ) {
                Rectangle rec = {
                    blocks.rec.x + i*blocks.rec.width,
                    blocks.rec.y + j*blocks.rec.height,
                    blocks.rec.width,
                    blocks.rec.height
                };
                data = blockBallCollide( rec, data.xHit, data.yHit );
                blocks.states[j*BLOCKCOUNTSQRT + i] = data.state;
                DrawRectangleRec( rec, RED );
                DrawRectangleLinesEx( rec, 1.0f, WHITE );
            }
        }
    }
}

void drawScore() {
    DrawText( TextFormat( "SCORE: %d", BLOCKCOUNT - score.blocks ), 10, 10, 30, WHITE );
    for ( int i = 0; i < score.balls; i++ ) DrawCircle( WIDTH - 56 + i*18, 18, 8, WHITE );
}

void resetGame() {
    score.state = 0;
    score.balls  = STARTBALLS;
    memset( blocks.states, 1, BLOCKCOUNT );
    resetBallPaddle();
}

void endMessage() {
    if ( score.state == 1 ) DrawText( "You Won!",   160, 370, 60, WHITE );
    else                    DrawText( "Game Over!", 130, 370, 60, WHITE );

    DrawText( "Press ENTER to play again", 160, 440, 20, WHITE );
}

void draw() {
    botMovePaddle();
    runBlocks();
    DrawRectangleRec( paddle.rec, BLUE );
    DrawCircleV( ball.pos, ball.radius, WHITE );
    drawScore();
    if ( score.state ) endMessage();
    if ( IsKeyPressed(KEY_ENTER) ) resetGame();
}



int main() {
    InitWindow( WIDTH, HEIGHT, "Arkanoid" );
    SetTargetFPS(1000);

    memset( blocks.states, 1, BLOCKCOUNT );
    blocks.rec   = (Rectangle){ 44.0f, 100.0f, 32.0f, 16.0f };

    ball.radius  = 8.0f;
    ball.pos     = STARTBALLPOS;
    ball.vel     = STARTBALLVEL;

    paddle.rec   = (Rectangle){ 250.0f, 700.0f, 100.0f, 10.0f };

    score.balls  = STARTBALLS;
    score.blocks = BLOCKCOUNT;

    while(!WindowShouldClose()) {
        update();

        BeginDrawing();
            ClearBackground(BLACK);
            draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
