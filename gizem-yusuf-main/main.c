#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#pragma warning(disable: 4244)  /* float to int conversion warning'ini kapat */

/* Screen dimensions */
const int screenWidth = 1024;
const int screenHeight = 600;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    int launched;
    float radius;
    int birdType;  /* 0=Red, 1=Bomber, 2=Bucky, 3=Big, 4=Female */
    int active;
} Bird;

typedef struct {
    Rectangle rect;
    int active;
    Vector2 velocity;
    int falling;
    Rectangle startRect; /* başlangıç konumu */
    float rotation;
    float angularVelocity;
} Block;

typedef struct {
    Vector2 position;
    float radius;
    int active;
    Vector2 velocity;
    int falling;
    int landed;
    int pigType;  /* 0=Normal, 1=Fat, 2=Armor, 3=Handsome, 4=King */
    int health;
} Enemy;

/* Ses yapısı */
typedef struct {
    Sound birdLaunch;
    Sound blockHit;
    Sound enemyHit;
    Sound victory;
    Sound gameOver;
    Sound pigLaugh;
    Sound pigCry;
    Music backgroundMusic;
} GameSounds;

/* Level sistemi */
typedef struct {
    int currentLevel;
    int maxLevel;
    int enemyCount;
    int blockCount;
    Vector2 enemyPositions[4];  /* Maksimum 4 düşman */
    Rectangle blockRects[12];   /* Maksimum 12 blok */
    int blockTypes[12];         /* 0 = blockd, 1 = blocky */
} LevelSystem;

#define MAX_ENEMIES 4           /* Arttırıldı */
#define MAX_BLOCKS 12           /* Arttırıldı */
#define MAX_TRAJECTORY_POINTS 100
#define MAX_BIRDS 6             /* Level 1 için 6 kuş */

Enemy enemies[MAX_ENEMIES];
Block blocks[MAX_BLOCKS];
Bird birds[MAX_BIRDS];          /* Kuş dizisi */
GameSounds sounds;
LevelSystem levelSystem;
int currentBirdIndex = 0;       /* Sıradaki kuş indeksi */

/* Oyun durumu değişkenleri */
int victoryPlayed = 0;
int gameOverPlayed = 0;

const float gravity = 0.41f;
typedef enum { MENU, GAME, LEVEL_COMPLETE } GameState;
GameState currentState = MENU;
float menuTimer = 0.0f;  /* Menü için zamanlayıcı */
float levelCompleteTimer = 0.0f;
const float MENU_DURATION = 2.0f;  /* Menünün ekranda kalma süresi (saniye) */
const float LEVEL_COMPLETE_DURATION = 3.0f;

/* Ses dosyalarını yükleme fonksiyonu */
void LoadGameSounds(GameSounds* gameSounds) {
    InitAudioDevice();
    
    gameSounds->birdLaunch = LoadSound("Sesler/angry-birds-hal-2.mp3");
    gameSounds->blockHit = LoadSound("Sesler/angry-birds-unknown-bird-sound.mp3");
    gameSounds->enemyHit = LoadSound("Sesler/angry-birds-lazer-bird-special-activated.mp3");
    gameSounds->victory = LoadSound("Sesler/victory-angry-birds.mp3");
    gameSounds->gameOver = LoadSound("Sesler/angry-birds-level-failed-1.mp3");
    gameSounds->pigLaugh = LoadSound("Sesler/angry-birds-king-pig-laugh.mp3");
    gameSounds->pigCry = LoadSound("Sesler/king-pig-crying.mp3");
    gameSounds->backgroundMusic = LoadMusicStream("Sesler/angry-birds-theme-song-audiotrimmer.mp3");
    
    /* Arka plan müziğini başlat */
    PlayMusicStream(gameSounds->backgroundMusic);
    SetMusicVolume(gameSounds->backgroundMusic, 0.3f);
}

/* Ses dosyalarını boşaltma fonksiyonu */
void UnloadGameSounds(GameSounds* gameSounds) {
    UnloadSound(gameSounds->birdLaunch);
    UnloadSound(gameSounds->blockHit);
    UnloadSound(gameSounds->enemyHit);
    UnloadSound(gameSounds->victory);
    UnloadSound(gameSounds->gameOver);
    UnloadSound(gameSounds->pigLaugh);
    UnloadSound(gameSounds->pigCry);
    UnloadMusicStream(gameSounds->backgroundMusic);
    CloseAudioDevice();
}

/* Level tasarımları */
void InitializeLevel(int level) {
    levelSystem.currentLevel = level;
    
    /* Tüm düşmanları ve blokları temizle */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
    }
    for (int i = 0; i < MAX_BLOCKS; i++) {
        blocks[i].active = 0;
    }
    
    switch (level) {
        case 1: /* Kolay seviye - Tek domuz yüksek kulede */
            levelSystem.enemyCount = 1;
            levelSystem.blockCount = 10;
            
            /* Tek domuz - orta katda platform üzerinde */
            enemies[0] = (Enemy){ {(float)(screenWidth * 0.8f), (float)(screenHeight * 0.53f)}, (float)(screenWidth * 0.025f), 1, {0.0f, 0.0f}, 0, 0, 0, 1 }; /* Normal domuz orta katda */
            
            /* 6 kuş sistemi başlat */
            currentBirdIndex = 0;
            for (int i = 0; i < MAX_BIRDS; i++) {
                birds[i].position = (Vector2){ (float)(screenWidth * 0.05f + i * 30), (float)(screenHeight * 0.85f) };
                birds[i].velocity = (Vector2){0, 0};
                birds[i].launched = 0;
                birds[i].radius = (float)(screenWidth * 0.025f);
                birds[i].birdType = i % 5; /* 0=Red, 1=Bomber, 2=Bucky, 3=Big, 4=Female */
                birds[i].active = 1;
            }
            
            /* İlk kuşu sapana koy */
            birds[0].position = (Vector2){ (float)(screenWidth * 0.15f), (float)(screenHeight * 0.65f) };
            
            /* Bloklar - Resme uygun kule yapısı */
            /* Alt kat - CAM BLOKLAR (kırılgan) */
            blocks[0].rect = (Rectangle){ (float)(screenWidth * 0.775f), (float)(screenHeight * 0.73f), (float)(screenWidth * 0.05f), (float)(screenHeight * 0.05f) };
            blocks[0].startRect = blocks[0].rect;
            blocks[0].active = 1;
            blocks[0].falling = 0;
            blocks[0].velocity = (Vector2){0, 0};
            blocks[0].rotation = 0;
            blocks[0].angularVelocity = 0;
            
            blocks[1].rect = (Rectangle){ (float)(screenWidth * 0.825f), (float)(screenHeight * 0.73f), (float)(screenWidth * 0.05f), (float)(screenHeight * 0.05f) };
            blocks[1].startRect = blocks[1].rect;
            blocks[1].active = 1;
            blocks[1].falling = 0;
            blocks[1].velocity = (Vector2){0, 0};
            blocks[1].rotation = 0;
            blocks[1].angularVelocity = 0;
            
            /* Sol dikey destek tahta */
            blocks[2].rect = (Rectangle){ (float)(screenWidth * 0.78f), (float)(screenHeight * 0.63f), (float)(screenWidth * 0.04f), (float)(screenWidth * 0.1f) };
            blocks[2].startRect = blocks[2].rect;
            blocks[2].active = 1;
            blocks[2].falling = 0;
            blocks[2].velocity = (Vector2){0, 0};
            blocks[2].rotation = 0;
            blocks[2].angularVelocity = 0;
            
            /* Sağ dikey destek tahta */
            blocks[3].rect = (Rectangle){ (float)(screenWidth * 0.82f), (float)(screenHeight * 0.63f), (float)(screenWidth * 0.04f), (float)(screenWidth * 0.1f) };
            blocks[3].startRect = blocks[3].rect;
            blocks[3].active = 1;
            blocks[3].falling = 0;
            blocks[3].velocity = (Vector2){0, 0};
            blocks[3].rotation = 0;
            blocks[3].angularVelocity = 0;
            
            /* Orta platform - domuz için */
            blocks[4].rect = (Rectangle){ (float)(screenWidth * 0.785f), (float)(screenHeight * 0.58f), (float)(screenWidth * 0.03f), (float)(screenHeight * 0.05f) };
            blocks[4].startRect = blocks[4].rect;
            blocks[4].active = 1;
            blocks[4].falling = 0;
            blocks[4].velocity = (Vector2){0, 0};
            blocks[4].rotation = 0;
            blocks[4].angularVelocity = 0;
            
            /* Üst dekoratif tahta */
            blocks[5].rect = (Rectangle){ (float)(screenWidth * 0.795f), (float)(screenHeight * 0.48f), (float)(screenWidth * 0.02f), (float)(screenHeight * 0.1f) };
            blocks[5].startRect = blocks[5].rect;
            blocks[5].active = 1;
            blocks[5].falling = 0;
            blocks[5].velocity = (Vector2){0, 0};
            blocks[5].rotation = 0;
            blocks[5].angularVelocity = 0;
            
            /* Ekstra destekleyici bloklar */
            blocks[6].rect = (Rectangle){ (float)(screenWidth * 0.76f), (float)(screenHeight * 0.78f), (float)(screenWidth * 0.03f), (float)(screenHeight * 0.05f) };
            blocks[6].startRect = blocks[6].rect;
            blocks[6].active = 1;
            blocks[6].falling = 0;
            blocks[6].velocity = (Vector2){0, 0};
            blocks[6].rotation = 0;
            blocks[6].angularVelocity = 0;
            
            blocks[7].rect = (Rectangle){ (float)(screenWidth * 0.86f), (float)(screenHeight * 0.78f), (float)(screenWidth * 0.03f), (float)(screenHeight * 0.05f) };
            blocks[7].startRect = blocks[7].rect;
            blocks[7].active = 1;
            blocks[7].falling = 0;
            blocks[7].velocity = (Vector2){0, 0};
            blocks[7].rotation = 0;
            blocks[7].angularVelocity = 0;
            
            /* Alt cam destekler */
            blocks[8].rect = (Rectangle){ (float)(screenWidth * 0.8f), (float)(screenHeight * 0.68f), (float)(screenWidth * 0.05f), (float)(screenHeight * 0.05f) };
            blocks[8].startRect = blocks[8].rect;
            blocks[8].active = 1;
            blocks[8].falling = 0;
            blocks[8].velocity = (Vector2){0, 0};
            blocks[8].rotation = 0;
            blocks[8].angularVelocity = 0;
            
            blocks[9].rect = (Rectangle){ (float)(screenWidth * 0.805f), (float)(screenHeight * 0.53f), (float)(screenWidth * 0.02f), (float)(screenHeight * 0.05f) };
            blocks[9].startRect = blocks[9].rect;
            blocks[9].active = 1;
            blocks[9].falling = 0;
            blocks[9].velocity = (Vector2){0, 0};
            blocks[9].rotation = 0;
            blocks[9].angularVelocity = 0;
            break;
            
        case 2: /* Orta seviye */
            levelSystem.enemyCount = 2;
            levelSystem.blockCount = 5;
            
            /* Düşmanlar */
            enemies[0] = (Enemy){ {screenWidth * 0.75f, screenHeight * 0.6f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            enemies[1] = (Enemy){ {screenWidth * 0.85f, screenHeight * 0.7f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            
            /* Tek kuş sistemi */
            currentBirdIndex = 0;
            birds[0].position = (Vector2){ screenWidth * 0.15f, screenHeight * 0.65f };
            birds[0].velocity = (Vector2){0, 0};
            birds[0].launched = 0;
            birds[0].radius = screenWidth * 0.025f;
            birds[0].birdType = 0; /* Red kuş */
            birds[0].active = 1;
            for (int i = 1; i < MAX_BIRDS; i++) {
                birds[i].active = 0;
            }
            
            /* Bloklar - Kule yapısı */
            for (int i = 0; i < 5; i++) {
                blocks[i].rect = (Rectangle){ 
                    screenWidth * 0.8f, 
                    screenHeight * (0.75f - i * 0.08f), 
                    screenWidth * 0.045f, 
                    screenHeight * 0.08f 
                };
                blocks[i].startRect = blocks[i].rect;
                blocks[i].active = 1;
                blocks[i].falling = 0;
                blocks[i].velocity = (Vector2){0, 0};
                blocks[i].rotation = 0;
                blocks[i].angularVelocity = 0;
            }
            break;
            
        case 3: /* Zor seviye */
            levelSystem.enemyCount = 2;
            levelSystem.blockCount = 8;
            
            /* Düşmanlar */
            enemies[0] = (Enemy){ {screenWidth * 0.72f, screenHeight * 0.5f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            enemies[1] = (Enemy){ {screenWidth * 0.88f, screenHeight * 0.5f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            
            /* Tek kuş sistemi */
            currentBirdIndex = 0;
            birds[0].position = (Vector2){ screenWidth * 0.15f, screenHeight * 0.65f };
            birds[0].velocity = (Vector2){0, 0};
            birds[0].launched = 0;
            birds[0].radius = screenWidth * 0.025f;
            birds[0].birdType = 0; /* Red kuş */
            birds[0].active = 1;
            for (int i = 1; i < MAX_BIRDS; i++) {
                birds[i].active = 0;
            }
            
            /* Bloklar - Piramit yapısı */
            /* Alt sıra */
            for (int i = 0; i < 4; i++) {
                blocks[i].rect = (Rectangle){ 
                    screenWidth * (0.7f + i * 0.05f), 
                    screenHeight * 0.7f, 
                    screenWidth * 0.045f, 
                    screenHeight * 0.1f 
                };
                blocks[i].startRect = blocks[i].rect;
                blocks[i].active = 1;
                blocks[i].falling = 0;
                blocks[i].velocity = (Vector2){0, 0};
                blocks[i].rotation = 0;
                blocks[i].angularVelocity = 0;
            }
            /* Üst sıra */
            for (int i = 4; i < 8; i++) {
                blocks[i].rect = (Rectangle){ 
                    screenWidth * (0.72f + (i-4) * 0.05f), 
                    screenHeight * 0.6f, 
                    screenWidth * 0.045f, 
                    screenHeight * 0.1f 
                };
                blocks[i].startRect = blocks[i].rect;
                blocks[i].active = 1;
                blocks[i].falling = 0;
                blocks[i].velocity = (Vector2){0, 0};
                blocks[i].rotation = 0;
                blocks[i].angularVelocity = 0;
            }
            break;
            
        case 4: /* Çok zor seviye */
            levelSystem.enemyCount = 3;
            levelSystem.blockCount = 10;
            
            /* Düşmanlar */
            enemies[0] = (Enemy){ {screenWidth * 0.7f, screenHeight * 0.4f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            enemies[1] = (Enemy){ {screenWidth * 0.8f, screenHeight * 0.6f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            enemies[2] = (Enemy){ {screenWidth * 0.9f, screenHeight * 0.7f}, screenWidth * 0.025f, 1, {0.0f, 0.0f}, 0, 0, 0, 1 };
            
            /* Tek kuş sistemi */
            currentBirdIndex = 0;
            birds[0].position = (Vector2){ screenWidth * 0.15f, screenHeight * 0.65f };
            birds[0].velocity = (Vector2){0, 0};
            birds[0].launched = 0;
            birds[0].radius = screenWidth * 0.025f;
            birds[0].birdType = 0; /* Red kuş */
            birds[0].active = 1;
            for (int i = 1; i < MAX_BIRDS; i++) {
                birds[i].active = 0;
            }
            
            /* Bloklar - Karmaşık yapı */
            for (int i = 0; i < 10; i++) {
                if (i < 5) {
                    /* Sol kule */
                    blocks[i].rect = (Rectangle){ 
                        screenWidth * 0.75f, 
                        screenHeight * (0.75f - i * 0.06f), 
                        screenWidth * 0.04f, 
                        screenHeight * 0.06f 
                    };
                } else {
                    /* Sağ kule */
                    blocks[i].rect = (Rectangle){ 
                        screenWidth * 0.85f, 
                        screenHeight * (0.75f - (i-5) * 0.06f), 
                        screenWidth * 0.04f, 
                        screenHeight * 0.06f 
                    };
                }
                blocks[i].startRect = blocks[i].rect;
                blocks[i].active = 1;
                blocks[i].falling = 0;
                blocks[i].velocity = (Vector2){0, 0};
                blocks[i].rotation = 0;
                blocks[i].angularVelocity = 0;
            }
            break;
    }
}

int main(void) {
    InitWindow(screenWidth, screenHeight, "Angry Birds - Raylib");
    SetTargetFPS(60);

    /* Level sistemini başlat */
    levelSystem.currentLevel = 1;
    levelSystem.maxLevel = 4;
    InitializeLevel(1);

    /* Arka planlar - Her seviye için ayrı */
    Texture2D backgrounds[4];
    
    /* Level 1 arka planı */
    Image bgImage1 = LoadImage("Backgrounds/Level1_Background.jpg");
    ImageResize(&bgImage1, screenWidth, screenHeight);
    backgrounds[0] = LoadTextureFromImage(bgImage1);
    UnloadImage(bgImage1);
    
    /* Level 2 arka planı */
    Image bgImage2 = LoadImage("Backgrounds/Level2_Background.png");
    ImageResize(&bgImage2, screenWidth, screenHeight);
    backgrounds[1] = LoadTextureFromImage(bgImage2);
    UnloadImage(bgImage2);
    
    /* Level 3 arka planı */
    Image bgImage3 = LoadImage("Backgrounds/Level3_Background.png");
    ImageResize(&bgImage3, screenWidth, screenHeight);
    backgrounds[2] = LoadTextureFromImage(bgImage3);
    UnloadImage(bgImage3);
    
    /* Level 4 arka planı */
    Image bgImage4 = LoadImage("Backgrounds/Level4_Background.png");
    ImageResize(&bgImage4, screenWidth, screenHeight);
    backgrounds[3] = LoadTextureFromImage(bgImage4);
    UnloadImage(bgImage4);

    /* Menu arka planı */
    Image menuImg = LoadImage("menu.png");
    ImageResize(&menuImg, screenWidth, screenHeight);
    Texture2D menuBackground = LoadTextureFromImage(menuImg);
    UnloadImage(menuImg);

    /* Zemin resmi */
    Image groundImg = LoadImage("ground.png");
    ImageResize(&groundImg, screenWidth, screenHeight * 0.25f); /* Zemini ekran genişliğine göre ayarla */
    Texture2D ground = LoadTextureFromImage(groundImg);
    UnloadImage(groundImg);

    /* Diğer dokuları yükle ve boyutlandır */
    Image slingImg = LoadImage("sling.png");
    ImageResize(&slingImg, screenWidth * 0.1f, screenHeight * 0.2f);
    Texture2D slingTexture = LoadTextureFromImage(slingImg);
    UnloadImage(slingImg);

    /* Blok dokularını yükle ve boyutlandır */
    Image blockImg1 = LoadImage("blockd.png");
    ImageResize(&blockImg1, screenWidth * 0.045f, screenHeight * 0.2f);
    Texture2D blockTexture1 = LoadTextureFromImage(blockImg1);
    UnloadImage(blockImg1);

    Image blockImg2 = LoadImage("blocky.png");
    ImageResize(&blockImg2, screenWidth * 0.14f, screenHeight * 0.12f);
    Texture2D blockTexture2 = LoadTextureFromImage(blockImg2);
    UnloadImage(blockImg2);

    /* Kuş dokularını yükle - Level 1 için farklı kuş türleri */
    Texture2D birdTextures[5];
    
    /* Red kuş */
    Image redBirdImg = LoadImage("BirdsImages/RedNormal.png");
    ImageResize(&redBirdImg, screenWidth * 0.05f, screenWidth * 0.05f);
    birdTextures[0] = LoadTextureFromImage(redBirdImg);
    UnloadImage(redBirdImg);
    
    /* Bomber kuş */
    Image bomberBirdImg = LoadImage("BirdsImages/BomberNormal.png");
    ImageResize(&bomberBirdImg, screenWidth * 0.05f, screenWidth * 0.05f);
    birdTextures[1] = LoadTextureFromImage(bomberBirdImg);
    UnloadImage(bomberBirdImg);
    
    /* Bucky kuş */
    Image buckyBirdImg = LoadImage("BirdsImages/BuckyNormal.png");
    ImageResize(&buckyBirdImg, screenWidth * 0.05f, screenWidth * 0.05f);
    birdTextures[2] = LoadTextureFromImage(buckyBirdImg);
    UnloadImage(buckyBirdImg);
    
    /* Big kuş */
    Image bigBirdImg = LoadImage("BirdsImages/BigNormal.png");
    ImageResize(&bigBirdImg, screenWidth * 0.06f, screenWidth * 0.06f);
    birdTextures[3] = LoadTextureFromImage(bigBirdImg);
    UnloadImage(bigBirdImg);
    
    /* Female kuş */
    Image femaleBirdImg = LoadImage("BirdsImages/FemaleNormal.png");
    ImageResize(&femaleBirdImg, screenWidth * 0.05f, screenWidth * 0.05f);
    birdTextures[4] = LoadTextureFromImage(femaleBirdImg);
    UnloadImage(femaleBirdImg);

    /* Domuz dokularını yükle - Level 1 için farklı domuz türleri */
    Texture2D pigTextures[5];
    
    /* Normal domuz */
    Image normalPigImg = LoadImage("PigsImages/NormalPipAwake.png");
    ImageResize(&normalPigImg, screenWidth * 0.05f, screenWidth * 0.05f);
    pigTextures[0] = LoadTextureFromImage(normalPigImg);
    UnloadImage(normalPigImg);
    
    /* Fat domuz */
    Image fatPigImg = LoadImage("PigsImages/FatPigAwake.png");
    ImageResize(&fatPigImg, screenWidth * 0.06f, screenWidth * 0.06f);
    pigTextures[1] = LoadTextureFromImage(fatPigImg);
    UnloadImage(fatPigImg);
    
    /* Armor domuz */
    Image armorPigImg = LoadImage("PigsImages/ArmorPigAwake.png");
    ImageResize(&armorPigImg, screenWidth * 0.05f, screenWidth * 0.05f);
    pigTextures[2] = LoadTextureFromImage(armorPigImg);
    UnloadImage(armorPigImg);
    
    /* Handsome domuz */
    Image handsomePigImg = LoadImage("PigsImages/HandsomePigNormal.png");
    ImageResize(&handsomePigImg, screenWidth * 0.05f, screenWidth * 0.05f);
    pigTextures[3] = LoadTextureFromImage(handsomePigImg);
    UnloadImage(handsomePigImg);
    
    /* King domuz */
    Image kingPigImg = LoadImage("PigsImages/KingPigAwake.png");
    ImageResize(&kingPigImg, screenWidth * 0.07f, screenWidth * 0.07f);
    pigTextures[4] = LoadTextureFromImage(kingPigImg);
    UnloadImage(kingPigImg);

    /* Sapan ve kuş için temel değişkenler */
    Vector2 slingAnchor = { screenWidth * 0.15f, screenHeight * 0.65f };  /* Sapanın sabit noktası */
    Vector2 birdStartPos = { slingAnchor.x, slingAnchor.y };             /* Kuşun başlangıç pozisyonu */
    float maxStretch = screenWidth * 0.15f;                              /* Maksimum çekme mesafesi */
    float elasticForce = 0.4f;                                          /* Sapan elastiklik kuvveti - basitleştirildi */
    int isDragging = 0;                                                 /* Sürükleme durumu */
    const float gravityStrength = 0.4f;                                /* Yerçekimi kuvveti */
    const float groundHeight = screenHeight * 0.25f;                    /* Zemin yüksekliği */

    /* Bird dizisi InitializeLevel'da başlatılıyor */

    /* Bloklar artık InitializeLevel fonksiyonu ile başlatılıyor */

    const int maxLives = 3;
    int lives = maxLives;
    int gameOver = 0;
    int score = 0;

    LoadGameSounds(&sounds);

    while (!WindowShouldClose()) {
        /* Arka plan müziğini güncelle */
        UpdateMusicStream(sounds.backgroundMusic);
        
        if (currentState == MENU) {
            menuTimer += GetFrameTime();
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(menuBackground, 0, 0, WHITE);
            EndDrawing();
            if (menuTimer >= MENU_DURATION) {
                currentState = GAME;
            }
            continue;
        }

        if (currentState == LEVEL_COMPLETE) {
            levelCompleteTimer += GetFrameTime();
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(backgrounds[levelSystem.currentLevel - 1], 0, 0, WHITE);
            DrawTexture(ground, 0, screenHeight - (int)(screenHeight * 0.25f), WHITE);
            
            DrawText(TextFormat("LEVEL %d COMPLETE!", levelSystem.currentLevel), 
                screenWidth/2 - 150, screenHeight/2 - 50, 30, DARKGREEN);
            
            if (levelSystem.currentLevel < levelSystem.maxLevel) {
                DrawText(TextFormat("Next Level: %d", levelSystem.currentLevel + 1), 
                    screenWidth/2 - 100, screenHeight/2, 20, BLUE);
            } else {
                DrawText("ALL LEVELS COMPLETE!", 
                    screenWidth/2 - 120, screenHeight/2, 20, GOLD);
            }
            
            EndDrawing();
            
            if (levelCompleteTimer >= LEVEL_COMPLETE_DURATION) {
                levelCompleteTimer = 0.0f;
                if (levelSystem.currentLevel < levelSystem.maxLevel) {
                    levelSystem.currentLevel++;
                    InitializeLevel(levelSystem.currentLevel);
                    
                    /* Kuş sistemini resetle */
                    currentBirdIndex = 0;
                    isDragging = 0;
                    lives = maxLives;
                    
                    /* Ses durumlarını resetle */
                    victoryPlayed = 0;
                    gameOverPlayed = 0;
                    
                    currentState = GAME;
                } else {
                    /* Tüm seviyeler tamamlandı, başa dön */
                    levelSystem.currentLevel = 1;
                    InitializeLevel(1);
                    
                    currentBirdIndex = 0;
                    isDragging = 0;
                    lives = maxLives;
                    score = 0;
                    gameOver = 0;
                    
                    /* Ses durumlarını resetle */
                    victoryPlayed = 0;
                    gameOverPlayed = 0;
                    
                    currentState = MENU;
                    menuTimer = 0.0f;
                }
            }
            continue;
        }

        /* === GÜNCELLEME === */
        /* Düşen blokların düşmanlara çarpması */
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if (blocks[i].active && blocks[i].falling) {
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (enemies[j].active && !enemies[j].falling &&
                        CheckCollisionCircleRec(enemies[j].position, enemies[j].radius, blocks[i].rect)) {
                        enemies[j].active = 0;  /* Düşmanı direkt öldür */
                        score += 50;
                        
                        /* Düşman ölüm sesi */
                        PlaySound(sounds.pigCry);
                    }
                }
            }
        }

        /* Düşman fiziği artık gerekli değil - direkt ölüyorlar */

        /* Artık gerekli değil - düşmanlar direkt ölüyor */

        /* Şu anki kuş var mı ve fırlatılmadı mı kontrol et */
        if (currentBirdIndex < MAX_BIRDS && birds[currentBirdIndex].active && !birds[currentBirdIndex].launched) {
            /* Kuş sürükleme mantığı */
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                if (CheckCollisionPointCircle(mousePos, birds[currentBirdIndex].position, birds[currentBirdIndex].radius * 1.5f)) {
                    isDragging = 1;
                }
            }

            if (isDragging) {
                /* Kuşu fareyle hareket ettir */
                birds[currentBirdIndex].position = GetMousePosition();

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    isDragging = 0;
                    /* Fırlatma hızı hesaplaması */
                    birds[currentBirdIndex].velocity.x = (slingAnchor.x - birds[currentBirdIndex].position.x) * elasticForce;
                    birds[currentBirdIndex].velocity.y = (slingAnchor.y - birds[currentBirdIndex].position.y) * elasticForce;
                    birds[currentBirdIndex].launched = 1;
                    
                    /* Kuş fırlatma sesi */
                    PlaySound(sounds.birdLaunch);
                }
            } else {
                /* Sapandaki kuşun sabit pozisyonu */
                birds[currentBirdIndex].position = birdStartPos;
            }
        }

        /* Fırlatılmış kuşların hareketi */
        if (currentBirdIndex < MAX_BIRDS && birds[currentBirdIndex].active && birds[currentBirdIndex].launched) {
            /* Yerçekimi ve hareket */
            birds[currentBirdIndex].velocity.y += gravityStrength;
            birds[currentBirdIndex].position.x += birds[currentBirdIndex].velocity.x;
            birds[currentBirdIndex].position.y += birds[currentBirdIndex].velocity.y;

            /* Zemin kontrolü veya ekran dışına çıkma */
            if (birds[currentBirdIndex].position.y > screenHeight - 100 || 
                birds[currentBirdIndex].position.x > screenWidth || 
                birds[currentBirdIndex].position.x < 0) {
                
                /* Sıradaki kuşa geç */
                currentBirdIndex++;
                
                if (currentBirdIndex < MAX_BIRDS && birds[currentBirdIndex].active) {
                    /* Sıradaki kuşu sapana koy */
                    birds[currentBirdIndex].position = birdStartPos;
                    birds[currentBirdIndex].velocity = (Vector2){0, 0};
                    birds[currentBirdIndex].launched = 0;
                } else {
                    /* Kuş kalmadı */
                    if (levelSystem.currentLevel == 1) {
                        gameOver = 1; /* Level 1'de tüm kuşlar bitince game over */
                    } else {
                        lives--;
                        if (lives <= 0) {
                            gameOver = 1;
                        } else {
                            /* Tek kuş sistemi resetle */
                            currentBirdIndex = 0;
                            birds[0].position = birdStartPos;
                            birds[0].velocity = (Vector2){0, 0};
                            birds[0].launched = 0;
                        }
                    }
                }
            }
        }

        /* Kuş ve blok çarpışmaları */
        if (currentBirdIndex < MAX_BIRDS && birds[currentBirdIndex].active && birds[currentBirdIndex].launched) {
            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (blocks[i].active && !blocks[i].falling && 
                    CheckCollisionCircleRec(birds[currentBirdIndex].position, birds[currentBirdIndex].radius, blocks[i].rect)) {
                    blocks[i].falling = 1;
                    blocks[i].velocity = (Vector2){ (float)(GetRandomValue(-3, 3)), -4.0f };
                    blocks[i].angularVelocity = (float)(GetRandomValue(-20, 20)) / 10.0f;
                    score += 10;
                    
                    /* Blok vuruş sesi */
                    PlaySound(sounds.blockHit);
                    
                    /* Kuşun hızını azalt */
                    birds[currentBirdIndex].velocity.x *= 0.3f;
                    birds[currentBirdIndex].velocity.y *= 0.3f;
                }
            }

            /* Kuş ve düşman çarpışmaları */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && !enemies[i].falling && 
                    CheckCollisionCircles(birds[currentBirdIndex].position, birds[currentBirdIndex].radius, enemies[i].position, enemies[i].radius)) {
                    
                    /* Domuz tipine göre hasar ver */
                    enemies[i].health--;
                    if (enemies[i].health <= 0) {
                        enemies[i].active = 0;  /* Düşmanı öldür */
                        score += 50;
                        PlaySound(sounds.pigCry);  /* Ölüm sesi çal */
                    } else {
                        PlaySound(sounds.pigLaugh);  /* Hasar sesi */
                    }
                }
            }
        }

        /* Update falling blocks */
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if (blocks[i].active && blocks[i].falling) {
                blocks[i].velocity.y += gravity;
                blocks[i].rect.y += blocks[i].velocity.y;
                blocks[i].rect.x += blocks[i].velocity.x;
                blocks[i].rotation += blocks[i].angularVelocity;

                float groundY = screenHeight - 250.0f;
                if (blocks[i].rect.y + blocks[i].rect.height >= groundY) {
                    blocks[i].rect.y = groundY - blocks[i].rect.height;
                    blocks[i].velocity.y = 0;
                    blocks[i].velocity.x = 0;
                    blocks[i].angularVelocity = 0;
                }
            }
        }

        /* R tuşu ile leveli resetle */
        if (IsKeyPressed(KEY_R)) {
            /* Mevcut leveli yeniden başlat */
            InitializeLevel(levelSystem.currentLevel);
            
            currentBirdIndex = 0;
            isDragging = 0;
            lives = maxLives;
            gameOver = 0;
            
            /* Ses durumlarını resetle */
            victoryPlayed = 0;
            gameOverPlayed = 0;
            
            /* Domuz gülüş sesi */
            PlaySound(sounds.pigLaugh);
        }

        /* === ÇIZIM === */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTexture(backgrounds[levelSystem.currentLevel - 1], 0, 0, WHITE);
        DrawTexture(ground, 0, screenHeight - (int)(screenHeight * 0.25f), WHITE);

        /* Sapan lastiklerini çiz */
        if (currentBirdIndex < MAX_BIRDS && birds[currentBirdIndex].active && !birds[currentBirdIndex].launched && isDragging) {
            DrawLineEx(slingAnchor, birds[currentBirdIndex].position, 3, BROWN);
            
            /* Fırlatma yönünü gösteren ok */
            Vector2 direction = {
                slingAnchor.x - birds[currentBirdIndex].position.x,
                slingAnchor.y - birds[currentBirdIndex].position.y
            };
            float distance = sqrtf(direction.x * direction.x + direction.y * direction.y);
            
            if (distance > 10) {
                Vector2 arrowStart = birds[currentBirdIndex].position;
                Vector2 arrowEnd = {
                    birds[currentBirdIndex].position.x + (direction.x * 0.3f),
                    birds[currentBirdIndex].position.y + (direction.y * 0.3f)
                };
                
                /* Ana ok çizgisi */
                DrawLineEx(arrowStart, arrowEnd, 3, RED);
                
                /* Ok uçları */
                float arrowLength = 15.0f;
                float angle = 0.5f;
                Vector2 normalizedDir = {
                    direction.x / distance,
                    direction.y / distance
                };
                
                Vector2 arrowLeft = {
                    arrowEnd.x + arrowLength * (cosf(angle) * normalizedDir.x - sinf(angle) * normalizedDir.y),
                    arrowEnd.y + arrowLength * (sinf(angle) * normalizedDir.x + cosf(angle) * normalizedDir.y)
                };
                
                Vector2 arrowRight = {
                    arrowEnd.x + arrowLength * (cosf(-angle) * normalizedDir.x - sinf(-angle) * normalizedDir.y),
                    arrowEnd.y + arrowLength * (sinf(-angle) * normalizedDir.x + cosf(-angle) * normalizedDir.y)
                };
                
                DrawLineEx(arrowEnd, arrowLeft, 3, RED);
                DrawLineEx(arrowEnd, arrowRight, 3, RED);
            }
        }

        /* Sapan direğini çiz */
        DrawTexture(slingTexture, 
            (int)(slingAnchor.x - slingTexture.width/2), 
            (int)(slingAnchor.y - slingTexture.height/2), 
            WHITE
        );

        /* Blokları çiz */
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if (blocks[i].active) {
                DrawTexturePro(
                    (i % 2 == 0) ? blockTexture1 : blockTexture2,
                    (Rectangle){ 0, 0, blocks[i].rect.width, blocks[i].rect.height },
                    blocks[i].rect,
                    (Vector2){ blocks[i].rect.width/2, blocks[i].rect.height/2 },
                    blocks[i].rotation,
                    WHITE
                );
            }
        }

        /* Düşmanları çiz */
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                DrawTexture(pigTextures[enemies[i].pigType], 
                    (int)(enemies[i].position.x - enemies[i].radius), 
                    (int)(enemies[i].position.y - enemies[i].radius), 
                    WHITE);
            }
        }

        /* Kuşları çiz */
        for (int i = 0; i < MAX_BIRDS; i++) {
            if (birds[i].active) {
                DrawTexture(birdTextures[birds[i].birdType],
                    (int)(birds[i].position.x - birds[i].radius),
                    (int)(birds[i].position.y - birds[i].radius),
                    WHITE
                );
            }
        }

        /* UI'ı çiz */
        DrawText(TextFormat("Level: %d/%d", levelSystem.currentLevel, levelSystem.maxLevel), 20, 20, 20, DARKBLUE);
        DrawText(TextFormat("Score: %d", score), 20, 50, 20, BLACK);
        
        if (levelSystem.currentLevel == 1) {
            /* Level 1 için kuş sayısı göster */
            int remainingBirds = 0;
            for (int i = currentBirdIndex; i < MAX_BIRDS; i++) {
                if (birds[i].active) remainingBirds++;
            }
            DrawText(TextFormat("Birds: %d", remainingBirds), 20, 80, 20, BLACK);
        } else {
            DrawText(TextFormat("Lives: %d", lives), 20, 80, 20, BLACK);
        }
        
        DrawText("R - Reset Level", 20, 110, 20, GRAY);

        /* Oyun durumu kontrolleri */
        int allEnemiesDead = 1;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                allEnemiesDead = 0;
                break;
            }
        }

        if (allEnemiesDead && !gameOver && !victoryPlayed) {
            PlaySound(sounds.victory);
            victoryPlayed = 1;
            currentState = LEVEL_COMPLETE;
        }

        if (gameOver && !gameOverPlayed) {
            PlaySound(sounds.gameOver);
            gameOverPlayed = 1;
        }

        if (gameOver) {
            DrawText("Game Over!", screenWidth/2 - 100, screenHeight/2, 40, RED);
            DrawText("Press R to restart", screenWidth/2 - 100, screenHeight/2 + 50, 20, GRAY);
        }

        EndDrawing();
    }

    /* Cleanup */
    /* Kuş texture'larını unload et */
    for (int i = 0; i < 5; i++) {
        UnloadTexture(birdTextures[i]);
    }
    
    /* Domuz texture'larını unload et */
    for (int i = 0; i < 5; i++) {
        UnloadTexture(pigTextures[i]);
    }
    
    UnloadTexture(blockTexture1);
    UnloadTexture(blockTexture2);
    UnloadTexture(ground);
    
    /* Tüm arka planları unload et */
    for (int i = 0; i < 4; i++) {
        UnloadTexture(backgrounds[i]);
    }
    
    UnloadTexture(menuBackground);
    UnloadTexture(slingTexture);

    UnloadGameSounds(&sounds);

    CloseWindow();
    return 0;
}
