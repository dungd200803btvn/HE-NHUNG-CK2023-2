
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define FLAP_BUTTON  4
#define BUZZER_PIN 13
#define SPRITE_HEIGHT   14
#define SPRITE_WIDTH    14
static const unsigned char PROGMEM wing_down_bmp[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000011, B11000000,
  B00011111, B11110000,
  B00111111, B00111000,
  B01111111, B11111110,
  B11111111, B11000001,
  B11011111, B01111110,
  B11011111, B01111000,
  B11011111, B01111000,
  B11001110, B01111000,
  B11110001, B11110000,
  B01111111, B11100000,
  B00111111, B11000000,
  B00000111, B00000000,
  B00000000, B00000000,
};

static const unsigned char PROGMEM wing_up_bmp[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000011, B11000000,
  B00011111, B11110000,
  B00111111, B00111000,
  B01110001, B11111110,
  B11101110, B11000001,
  B11011111, B01111110,
  B11011111, B01111000,
  B11111111, B11111000,
  B11111111, B11111000,
  B11111111, B11110000,
  B01111111, B11100000,
  B00111111, B11000000,
  B00000111, B00000000,
  B00000000, B00000000,
};
int GAME_SPEED;
int game_state = 1; // 0 = game over screen, 1 = in game
int score = 0; // current game score
int high_score = 0; // highest score since the nano was reset
int bird_x = (int)display.width() / 4; // birds x position (along) - initialised to 1/4 the way along the screen
int bird_y; // birds y position (down)
int momentum = 0; // how much force is pulling the bird down
int wall_x[2]; // an array to hold the walls x positions
int wall_y[2]; // an array to hold the walls y positions
int wall_gap = 30; // size of the wall wall_gap in pixels
int wall_width = 10; // width of the wall in pixels
int difficulty = 0; // 0: dễ, 1: trung bình, 2: khó
const int game_speeds[3] = {100, 70, 40}; // tốc độ tương ứng với độ khó
unsigned long buttonPressTime = 0; // thời gian bắt đầu nhấn nút
bool selectingDifficulty = true; // trạng thái chọn độ khó
unsigned long lastUpdateTime = 0;
void setup() {
  Serial.begin(9600);
  if (!display.begin(0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SH1106 allocation failed"));
    for (;;); // Don't proceed, loop forever
  } 
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();
  pinMode(FLAP_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0));
  resetGame();
   selectingDifficulty = true; // bắt đầu ở trạng thái chọn độ khó
}

void loop() {
  if (selectingDifficulty) {
    display.clearDisplay();
    boldTextAtCenter(display.height() / 2 - 20, "Start a new game: ");
    boldTextAtCenter(display.height() / 2 - 10, "Select Difficulty:");
    boldTextAtCenter(display.height() / 2, difficulty == 0 ? "Easy" : (difficulty == 1 ? "Medium" : "Hard"));
    display.display();
    if (digitalRead(FLAP_BUTTON) == LOW) {
      if (buttonPressTime == 0) {
        buttonPressTime = millis(); // bắt đầu đếm thời gian nhấn nút
      } else if (millis() - buttonPressTime > 1000) {
        difficulty = (difficulty + 1) % 3; // chuyển sang mức độ khó tiếp theo
        buttonPressTime = millis(); // đặt lại thời gian đếm
      }
    } else {
      if (buttonPressTime != 0) {
        selectingDifficulty = false; // ngừng chọn độ khó và bắt đầu vào game
        GAME_SPEED = game_speeds[difficulty]; // đặt tốc độ game theo độ khó đã chọn
        resetGame(); // khởi động lại game
        game_state = 0; // chuyển sang trạng thái chơi game
      }
      buttonPressTime = 0; // đặt lại thời gian đếm
    }
  } 
  else{
 if (game_state == 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= GAME_SPEED) {
      lastUpdateTime = currentTime;
      display.clearDisplay();
      if (digitalRead(FLAP_BUTTON) == LOW) {
        momentum = -4; // len 4 
      }
      momentum += 1;//giam 1
      bird_y += momentum;
      if (bird_y < 0 ) {
        bird_y = 0;
      }
      if (bird_y > display.height() - SPRITE_HEIGHT) {
        bird_y = display.height() - SPRITE_HEIGHT;
        momentum = -2;
      }
      if (momentum < 0) {
        if (random(2) == 0) {
          display.drawBitmap(bird_x, bird_y, wing_down_bmp, SPRITE_WIDTH, SPRITE_HEIGHT, SH110X_WHITE);
        } else {
          display.drawBitmap(bird_x, bird_y, wing_up_bmp, SPRITE_WIDTH, SPRITE_HEIGHT, SH110X_WHITE);
        }
      } else {
        display.drawBitmap(bird_x, bird_y, wing_up_bmp, SPRITE_WIDTH, SPRITE_HEIGHT, SH110X_WHITE);
      }
      for (int i = 0 ; i < 2; i++) {
        display.fillRect(wall_x[i], 0, wall_width, wall_y[i], SH110X_WHITE);
        display.fillRect(wall_x[i], wall_y[i] + wall_gap, wall_width, display.height() - wall_y[i] + wall_gap, SH110X_WHITE);
        if (wall_x[i] < 0) {
          wall_y[i] = random(0, display.height() - wall_gap);
          wall_x[i] = display.width();
        }
        if (wall_x[i] == bird_x) {
          score++;
         tone(BUZZER_PIN, 440); // A4
    delay(25);
    tone(BUZZER_PIN, 494); // B4
    delay(25);
    tone(BUZZER_PIN, 523); // C5
    delay(25);
    tone(BUZZER_PIN, 587); // D5
    delay(25);
    noTone(BUZZER_PIN);
          high_score = max(score, high_score);
        }
        if (
          (bird_x + SPRITE_WIDTH > wall_x[i] && bird_x < wall_x[i] + wall_width) // level with wall
          &&
          (bird_y < wall_y[i] || bird_y + SPRITE_HEIGHT > wall_y[i] + wall_gap) // not level with the gap
        ) {
          display.display();
          delay(500);
          game_state = 1;
        } 
        wall_x[i] -= 4;
      }
      boldTextAtCenter(0, (String)score);
      display.display();
    }
  } else {
    display.clearDisplay();
    screenWipe(10);
    outlineTextAtCenter(1, "FLAPPY BIRD");
    textAtCenter(display.height() / 2 - 8, "GAME OVER");
    textAtCenter(display.height() / 2, String(score));
    boldTextAtCenter(display.height() - 16, "HIGH SCORE");
    boldTextAtCenter(display.height()  - 8, String(high_score));
    display.display();
    tone(BUZZER_PIN, 392); // G4
    delay(250);
    tone(BUZZER_PIN, 349); // F4
    delay(250);
    tone(BUZZER_PIN, 330); // E4
    delay(250);
    tone(BUZZER_PIN, 294); // D4
    delay(250);
    noTone(BUZZER_PIN);
    delay(2000); // Chờ 2 giây trước khi chuyển sang màn hình chọn độ khó
    // wait while the user stops pressing the button
    while (digitalRead(FLAP_BUTTON) == LOW);
 selectingDifficulty = true;
  while (selectingDifficulty) {
    display.clearDisplay();
    boldTextAtCenter(display.height() / 2 - 20, "Start a new game: ");
    boldTextAtCenter(display.height() / 2 - 10, "Select Difficulty:");
    boldTextAtCenter(display.height() / 2, difficulty == 0 ? "Easy" : (difficulty == 1 ? "Medium" : "Hard"));
    display.display();
    if (digitalRead(FLAP_BUTTON) == LOW) {
      if (buttonPressTime == 0) {
        buttonPressTime = millis(); // bắt đầu đếm thời gian nhấn nút
      } else if (millis() - buttonPressTime > 1000) {
        difficulty = (difficulty + 1) % 3; // chuyển sang mức độ khó tiếp theo
        buttonPressTime = millis(); // đặt lại thời gian đếm
      }
    } else {
      if (buttonPressTime != 0) {
        selectingDifficulty = false; // ngừng chọn độ khó và bắt đầu vào game
        resetGame(); // khởi động lại game
        game_state = 0; // chuyển sang trạng thái chơi game
      }
      buttonPressTime = 0; // đặt lại thời gian đếm
    }
  }
    while (digitalRead(FLAP_BUTTON) == HIGH);
    // start a new game
    screenWipe(10);
    game_state = 0;
  }
  }
}
void screenWipe(int speed) {
  // progressively fill screen with white
  for (int i = 0; i < display.height(); i += speed) {
    display.fillRect(0, i, display.width(), speed, SH110X_WHITE);
    display.display();
  }
  // progressively fill the screen with black
  for (int i = 0; i < display.height(); i += speed) {
    display.fillRect(0, i, display.width(), speed, SH110X_BLACK);
    display.display();
  }
}
void textAt(int x, int y, String txt) {
  display.setCursor(x, y);
  display.print(txt);
}
void textAtCenter(int y, String txt) {
  textAt(display.width() / 2 - txt.length() * 3, y, txt);
}
void outlineTextAtCenter(int y, String txt) {
  int x = display.width() / 2 - txt.length() * 3;
  display.setTextColor(SH110X_WHITE);
  textAt(x - 1, y, txt);
  textAt(x + 1, y, txt);
  textAt(x, y - 1, txt);
  textAt(x, y + 1, txt);
  display.setTextColor(SH110X_BLACK);
  textAt(x, y, txt);
  display.setTextColor(SH110X_WHITE);
}
void boldTextAtCenter(int y, String txt) {
  int x = display.width() / 2 - txt.length() * 3;
  textAt(x, y, txt);
  textAt(x + 1, y, txt);
}
void resetGame() {
  bird_y = display.height() / 2;
  momentum = -4;
  wall_x[0] = display.width();
  wall_y[0] = display.height() / 2 - wall_gap / 2;
  wall_x[1] = display.width() + display.width() / 2;
  wall_y[1] = display.height() / 2 - wall_gap / 1;
  score = 0;
  lastUpdateTime = millis(); // khởi tạo thời gian cập nhật gần nhất
  GAME_SPEED = game_speeds[difficulty]; 
}

