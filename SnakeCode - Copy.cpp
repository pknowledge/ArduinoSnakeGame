#include <U8g2lib.h>  // LCD Library : https://github.com/olikraus/U8g2_Arduino
// On Arduino IDE, click Sketch/Include library/Manage libraries/ then type Freertos and install Richard Barry's library
#include <Arduino_FreeRTOS.h> 
#include <SPI.h>
#define RIGHT 4 // Pin 2 connect to RIGHT button
#define LEFT 5  // Pin 3 connect to LEFT button
#define UP 6    // Pin 4 connect to UP button
#define DOWN 7  // Pin 5 connect to DOWN button
#define MAX_LENGTH 100  // Max snake's length, need less than 89 because arduino don't have more memory
void TaskDisplayLCD( void *pvParameters );
void TaskHandleButton( void *pvParameters );
TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 9);
//diameter of the gameItemSize
unsigned int gameItemSize = 4;
volatile unsigned int snakeSize = 4;
volatile unsigned int snakeDir = 1;
volatile int SPEED = 1; // Input from button

struct gameItem {
  volatile unsigned int X; // x position
  volatile unsigned int Y;  //y position
};

//array to store all snake body part positions
gameItem snake[MAX_LENGTH];

//snake food item
gameItem snakeFood;

void get_key(){
  if(digitalRead(LEFT) == 0 && snakeDir != 1)
  {
    snakeDir = 0; // left
  }
  else if(digitalRead(RIGHT) == 0 && snakeDir != 0)
  {
    snakeDir = 1; // right
  }
  else if(digitalRead(DOWN) == 0 && snakeDir != 3)
  {
    snakeDir = 2; // down
  }
  else if(digitalRead(UP) == 0 && snakeDir != 2)
  {
    snakeDir = 3; // up
  }
}

void drawGameOver() {
  char _point[]= {' ',' ',' ','\n'}; // put '1' instead of '0' , it will ignore :v
  snakeSize -= 5;
  if (snakeSize < 10)
  {
    _point[2] = snakeSize + '0';
  }
  else if(snakeSize >= 10 && snakeSize < 100)
  {
    _point[1] = (snakeSize / 10) + '0';
    _point[2] = (snakeSize % 10) + '0';
  }
  else if (snakeSize >= 100)
  {
    _point[2] = (snakeSize % 10) + '0';
    snakeSize /= 10;
    _point[0] = (snakeSize / 10) + '0';
    _point[1] = (snakeSize % 10) + '0';
  }
  while(1)
  {
     u8g2.firstPage();
    do {
      u8g2.drawStr(30,20,"--POINT--");//y,x
      u8g2.drawStr(50,40,_point);//y,x
    }   while (u8g2.nextPage() );
  }
}

void drawSnake() {
  for (unsigned int i = 0; i < snakeSize; i++) {
    u8g2.drawFrame(snake[i].X, snake[i].Y, gameItemSize, gameItemSize);
  }
}

void drawFood() {
  u8g2.drawBox(snakeFood.X, snakeFood.Y, gameItemSize, gameItemSize);
}

void spawnSnakeFood() {
  //generate snake Food position and avoid generate on position of snake
  unsigned int i = 1;
  do {
    snakeFood.X = random(2, 126);
    while(snake[i].X == snakeFood.X || i != snakeSize)
    {
      snakeFood.X = random(2, 126);
      i++;
    }    
  } while (snakeFood.X % 4 != 0);
  i = 1;
  do {
    snakeFood.Y = random(2, 62);
    while(snake[i].Y == snakeFood.Y || i != snakeSize)
    {
      snakeFood.Y = random(2, 62);
      i++;
    }    
  } while (snakeFood.Y % 4 != 0);
}

void handleColisions() {
  //check if snake eats food
  if (snake[0].X == snakeFood.X && snake[0].Y == snakeFood.Y) {
    //increase snakeSize
    snakeSize++;
    //regen food
    spawnSnakeFood();
  }

  //check if snake collides with itself
  else {
    for (unsigned int i = 1; i < snakeSize; i++) {
      if (snake[0].X == snake[i].X && snake[0].Y == snake[i].Y) {
        drawGameOver();
      }
    }
  }
  //check for wall collisions
  if ((snake[0].X < 0) || (snake[0].Y < 0) || (snake[0].X > 124) || (snake[0].Y > 60)) {
    drawGameOver();
  }
}

void updateValues() {
  //update all body parts of the snake excpet the head
  unsigned int i;
  for (i = snakeSize - 1; i > 0; i--)
    snake[i] = snake[i - 1];

  //Now update the head
  //move left
  if (snakeDir == 0)
    snake[0].X -= gameItemSize;
    
  //move right
  else if (snakeDir == 1)
    snake[0].X += gameItemSize;

  //move down
  else if (snakeDir == 2)
    snake[0].Y += gameItemSize;

  //move up
  else if (snakeDir == 3) 
    snake[0].Y -= gameItemSize;
}

void playGame() {
  handleColisions();
  updateValues();
  u8g2.firstPage();
  do {
    drawSnake();
    drawFood();
    delay(1);
  } while (u8g2.nextPage());
}

void setup() {
  //Set 4 button pins
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  u8g2.begin(); 
  u8g2.setFont(u8g2_font_ncenB10_tr);
  volatile char ModeSelection[] = {SPEED+'0','\n'};
  while(digitalRead(LEFT) != 0)
  {
    if(digitalRead(UP) == 0)
    {
      while(digitalRead(UP) == 0);
      if(SPEED < 4)
        {
          SPEED++;
          ModeSelection[0] = SPEED + '0';
        }
    }
    else if(digitalRead(DOWN) == 0)
    {
      while(digitalRead(DOWN) == 0);
      if(SPEED > 1 )
          {
            SPEED--;
            ModeSelection[0] = SPEED + '0';
          }
    }
     
    u8g2.firstPage();
    do {
      u8g2.drawStr(17,20,"Speed Mode");//y,x
      u8g2.drawStr(62,40,ModeSelection);//y,x
    }while ( u8g2.nextPage() );
    delay(50);
  }

  SPEED *= 50;
  randomSeed(analogRead(0)); // Change random number base on analog noise
  xTaskCreate(TaskDisplayLCD, "Task1", 100, NULL, 1, &TaskHandle_1);
  xTaskCreate(TaskHandleButton, "Task2", 100, NULL, 1, &TaskHandle_2);
}

void loop() {
}

void TaskDisplayLCD(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    playGame();
    vTaskDelay( SPEED / portTICK_PERIOD_MS ); // load each 200ms
  }
}

void TaskHandleButton(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    get_key();
    vTaskDelay(100/ portTICK_PERIOD_MS ); //load each 100ms
  }
}