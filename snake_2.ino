
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <EEPROM.h>


// CONFIGURATION

// FPS to begin moving at
int beginRate = 2;

// # of points between levels
int levelInterval = 3;

// Milliseconds to wait before moving food
int moveFoodTimeout = 300;

// Screen size
int boardWidth = 8;
int boardHeight = 8;

// Snake & Food colors
uint16_t foodColor = LED_YELLOW;
uint16_t snakeColor = LED_GREEN;

// Initial position of the snake
int snakeInitialX = 0;
int snakeInitialY = 0;

// // // 


// Joystick inputs
int joystickXPin = 1;
int joystickYPin = 0;

// EEPROM addresses for high score and
// next random seed
int seedAddr = 0;
int scoreAddr = 10;

// Game is paused
// Used for debugging
boolean paused = false;

// Convenience for snake direciton
enum direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

// X, Y coordinate pair
struct point {
	int x;
	int y;
};

// Linked list to keep track of tail
// plist short for "point list"
struct plist {
	struct point coord;
	struct plist* next;
};

// Find plist length
int plistLength (struct plist* list) {

	if (!list) {
		return 0;
	}

	int counter = 1;

	struct plist item = *list;
	
	while (item.next) {
        counter++;
		item = *(item.next);
	}

	return counter;
}

// Remove the last element from plist
void plistPop (struct plist* list) {
  
	if (!list) {
		return;
	}
	
	struct plist* item = list;
	struct plist* last = item;
	
	while (item->next) {
		last = item;
		item = item->next;
	}
	
	// Deallocate memory for the node
	free(item);
	last->next = NULL;
}

// Add element to beginning of plist
struct plist* plistPush(struct plist* list, struct point item) {

	// Create a new item
    struct plist* newItem;
      
	// Allocate memory for the new item
    newItem = (struct plist*)malloc(sizeof(struct plist));

	// Fill the item with shit
    newItem->coord = item;
    newItem->next = list;
    
    return newItem;
}

// checks if a given point is in a given plist
boolean pointInPlist (struct point item, struct plist* list) {
	if (!list) {
		return false;
	}
	
	struct plist* cur = list;
	boolean i = true;
	
	do {
		if (pointsAreEqual(item, cur->coord)) {
			return true;
		}
		
		if (cur->next) {
			cur = cur->next;
		} else {
			i = false;
		}
	
	} while (i);
	
	return false;
}

boolean pointsAreEqual (struct point pA, struct point pB) {
	return pA.x == pB.x && pA.y == pB.y;
}

int timeout = -1;
long lastMillis;
float currentRate;
int interval;

struct point snakeHead;
struct plist* snakeTail;
int tailLength;
direction direction;
enum direction nextDirection;

int score;
struct point food;


Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();
Adafruit_7segment scoreboard = Adafruit_7segment();

// Seed the random number generator
// Otherwise each time the program runs it will
// use the same sequence of numbers for the food location.
// Saves a random number in EEPROM to seed with next time
void seed () {
	
	// Even if there's nothing at this location,
	// the default value is 255 so we'll be good anyway.
	int seed = EEPROM.read(seedAddr);
	randomSeed(seed);
	seed = seed + random();
	
	if (seed > 2147483646) {
		seed = random();
	}
	
	EEPROM.write(seedAddr, seed);
}


// Set up, or reset the game
void initializeGame () {
	
	// Put the food somewhere
	moveFood();

	score = 0;
	
	direction = RIGHT;
	tailLength = 0;
	
	snakeHead.x = snakeInitialX;
	snakeHead.y = snakeInitialY;
	
	// To do: clear out the entire linked list
	// this leads to a memory leak.
	snakeTail = NULL;
	
	currentRate = beginRate;
	interval = 1000 / currentRate;	
}


void setup () {
	
	Serial.begin(9600);  
	matrix.begin(0x72);
	scoreboard.begin(0x71);
	
	// Lower this value to save battery
	// Raise it to look awesomer
	matrix.setBrightness(10);
	seed();
	initializeGame();
}


void loop () {
	if (paused) {
		return;
	}

	// Routines that need to run every loop,
	// regardless of whether it's a frame
	checkDirection();
	checkTimeout();

	// Game frame
	if (millis() - lastMillis > interval) {
		lastMillis = millis();
		
		changeDirection();
		moveSnake();
	
		if (snakeTouchingSnake()) {
			endGame();
		}
			
		if (snakeTouchingFood()) {
			earnPoint();			
			moveFood();
		}
		
		updateDisplay();
	}
}

// Checks to see if a direction change was requested
// Doesn't necessarily change it
void checkDirection () {
	
	struct point joystick = readJoystick();
	
	// Maybe adjust the thresholds?
	if (joystick.x > 1000) {
		nextDirection = LEFT;
	} else if (joystick.x < 100) {
		nextDirection = RIGHT;
	}

	if (joystick.y > 1000) {
		nextDirection = DOWN;
	} else if (joystick.y < 100) {
		nextDirection = UP;
	}
}

// The snake shouldn't be able to flip 180 degrees
// in one go. Checks if a direction change is valid,
// And changes it if so
void changeDirection () {
	if (nextDirection == LEFT && direction != RIGHT) {
		direction = LEFT;
	}
	
	if (nextDirection == RIGHT && direction != LEFT) {
		direction = RIGHT;
	}
	
	if (nextDirection == UP && direction != DOWN) {
		direction = UP;
	}
	
	if (nextDirection == DOWN && direction != UP) {
		direction = DOWN;
	}
}

// Tail must update first
// Basically, it moves to where the head is now,
// then the head moves.
void moveSnake () {
	moveSnakeTail();
	moveSnakeHead();
}

void moveSnakeHead() {
	snakeHead = alterPointByDirection(snakeHead);
}

void moveSnakeTail () {	
	if (!tailLength) {
		return;
	}
	
	// Add the current snake head to the beginning of the plist
	snakeTail = plistPush(snakeTail, snakeHead);
	
	// Remove any pixels in the tail after the amount
	// required by the level.
	while (plistLength(snakeTail) > tailLength) {
		plistPop(snakeTail);
	}
}

// Moves a given point one pixel in the current direction
struct point alterPointByDirection (struct point item) {
	switch (direction) {
		case UP:
			item.y--;
		break;
		
		case DOWN:
			item.y++;
		break;
		
		case LEFT:
			item.x--;
		break;
		
		case RIGHT:
			item.x++;
		break;
	}
	
			
	if (item.y > (boardHeight - 1)) {
		item.y = 0;
	}
	
	if (item.y < 0) {
		item.y = boardHeight - 1;
	}
	
	
	if (item.x > (boardWidth - 1)) {
		item.x = 0;
	}
	
	if (item.x < 0) {
		item.x = boardWidth - 1;
	}

	return item;
}


boolean snakeTouchingSnake() {
	return pointInPlist(snakeHead, snakeTail);
}

boolean snakeTouchingFood() {
	return pointsAreEqual(snakeHead, food);
}

// The food moves slightly after the snake eats it
// If it moves either right away or on the next frame
// it's jarring and looks wrong
// So it moves a fraction of a second after to give
// a nice little hopping effect
void moveFood () {
	timeout = moveFoodTimeout;
}

void checkTimeout () {
	if (timeout == 0) {
		boolean pointInSnake = false;

		do {
			food.x = random(0, 7);
			food.y = random(0, 7);
			
			// Don't want to move the food to a point
			//currently occupied by part of the snake
			if (pointInPlist(food, snakeTail)) {
				pointInSnake = true;
			} else if (pointsAreEqual(food, snakeHead)) {
				pointInSnake = true;
			} else {
				pointInSnake = false;
			}
		} while (pointInSnake);

		updateDisplay();
	}
	
	if (timeout >= 0) {
		timeout--;
	}
}

void earnPoint () {
	score++;
	
	if (score % levelInterval == 0) {
		currentRate += .5;
		interval = 1000 / currentRate;
		tailLength++;
	}	
}

void updateDisplay () {
	matrix.clear();
	
	drawFood();
	drawSnake();
	
	matrix.writeDisplay();

	scoreboard.print(score);
	scoreboard.writeDisplay();
}

void drawFood () {
	matrix.drawPixel(food.x, food.y, foodColor);
}

void drawSnake () {
	matrix.drawPixel(snakeHead.x, snakeHead.y, snakeColor);
	
	if (snakeTail) {
		struct plist tail = *snakeTail;
		
   		boolean i = true;
		do {
			matrix.drawPixel(tail.coord.x, tail.coord.y, snakeColor);

			if (tail.next) {
				tail = *(tail.next);
			} else {
				i = false;
			}

		} while(i);
	}
}

void endGame () {
//        interrupt();
//        return;
        
	for (int i = 0; i < boardHeight; i++) {
		matrix.drawLine(0, i, 7, i, LED_RED);
		matrix.writeDisplay();
		delay(100);
	}
	
	matrix.blinkRate(1);
	delay(2000);
	matrix.blinkRate(0);

	if (highScore()) {
		saveHighScore();
		displayCongrats();
	}
	
	initializeGame();
}

boolean highScore () {
	int hScore = EEPROM.read(scoreAddr);
	
	if (hScore && hScore != 255) {
		return score > hScore;
	} else {
		return true;
	}
}

void saveHighScore () {
	EEPROM.write(scoreAddr, score);
}

void displayCongrats () {
	matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
	matrix.setTextSize(1);
	matrix.setTextColor(LED_GREEN);
	
	for (int8_t x=7; x>=-100; x--) {
		matrix.clear();
		matrix.setCursor(x,0);
		matrix.print("You rock! High Score!");
		matrix.writeDisplay();
		delay(50);
	}
}

struct point readJoystick () {
	struct point joyval;
	
	joyval.x = analogRead(joystickXPin);
	joyval.y = analogRead(joystickYPin);
	
	return joyval;
}

void interrupt () {
	paused = true;
}
