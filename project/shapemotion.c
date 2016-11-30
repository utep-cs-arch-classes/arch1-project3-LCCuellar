/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdlib.h>
#include "buzzer.h"

#define GREEN_LED BIT6
#define S1DOWN !(p2sw_read() &BIT0)
#define S2DOWN !(p2sw_read() &BIT1)
#define S3DOWN !(p2sw_read() &BIT2)
#define S4DOWN !(p2sw_read() &BIT3)


int score = 0;


AbRect rect9 = {abRectGetBounds, abRectCheck, {9,9}}; /**< 9x9 rectangle */
AbRect rect10 = {abRectGetBounds, abRectCheck, {3,3}}; /**< 10x10 rectangle */
AbRect topField = {abRectGetBounds, abRectCheck,
		   {screenWidth/2-10,9}
};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer topFieldLayer = {
  (AbShape *) &topField,
  {screenWidth/2, 20},
  {0,0},{0,0},
  COLOR_BLACK,
  0
};

Layer botFieldLayer = {
  (AbShape *) &topField,
  {screenWidth/2, screenHeight - 5},
  {0,0},{0,0},
  COLOR_BLACK,
  0
};

Layer fieldLayer = { /*playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2}, /**< center */
  {0,0}, {0,0}, /* last & next pos */
  COLOR_BLACK,
  0,
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle10,
  {(screenWidth/2), (screenHeight-22)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &fieldLayer,
};

Layer enemyLayer1 = {
  (AbShape *)&rect9,
  {screenWidth/2, 15},
  {0,0},{0,0},
  COLOR_RED,
  &fieldLayer,
};

int enemy1Flag = 0;


Layer bullet1 = {
  (AbShape *)&rect10,
  {0,0},
  {0,0},{0,0},
  COLOR_BLACK,
  &fieldLayer,
};
int bulletFlag1 = 0;

Layer bullet2 = {
  (AbShape *)&rect10,
  {0,0},
  {0,0},{0,0},
  COLOR_BLACK,
  &fieldLayer,
};
int bulletFlag2 = 0;

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml0 = { &layer0, {0,0}, 0 };
MovLayer mlEnemy1 = { &enemyLayer1, {5,4}, 0};
MovLayer mlBullet1 = { &bullet1, {0,-6}, 0};
MovLayer mlBullet2 = { &bullet2, {0,-6}, 0};



movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	// P1OUT |= GREEN_LED;
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


int collision(const Layer *l1, const Layer *l2) {
  Region r1; Region r2; Region rU;
  
  abShapeGetBounds(l1->abShape, &(l1->pos), &r1);
  abShapeGetBounds(l2->abShape, &(l2->pos), &r2);
  
  regionIntersect(&rU, &r1, &r2);
  
  int xU1 = rU.topLeft.axes[0];
  int xU2 = rU.botRight.axes[0];
  int yU1 = rU.topLeft.axes[1];
  int yU2 = rU.botRight.axes[1];

  if(xU1 >= xU2 || yU1 >= yU2) {
    return 0; // No Collision
  }
  return 1; // Collision
}
 

int endGame = 0;
int loss = 0;
u_int bgColor = COLOR_BEIGE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT = 0;

  configureClocks();
  lcd_init();
  shapeInit();
  buzzer_init();
  p2sw_init(BIT0 | BIT1 | BIT2 | BIT3);
  
  shapeInit();
  
  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  
  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      //  P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    //  P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
    killBullet(&bullet1, &mlBullet1, &bulletFlag1);
    killBullet(&bullet2, &mlBullet2, &bulletFlag2);
    killEnemy(&enemyLayer1, &mlEnemy1, &enemy1Flag);

    if(killPlayer()) {
      loss = 1;
      endGame = 1;
      drawString5x7(screenWidth/2 - 20, 30, "YOU LOSE", COLOR_RED,COLOR_BEIGE);
    }
    
    if(score >= 8) {
      ml0.next = 0;
      endGame = 1;
      buzzer_set_period(0);
      drawString5x7(screenWidth/2 - 20, 30, "YOU WIN!", COLOR_RED,COLOR_BEIGE);
    }
  }
}


void killBullet(Layer *b, MovLayer *mb, int *flag) {
  
  if(*flag && collision(b , &topFieldLayer)) {
    b->color = COLOR_BEIGE;
    
    mb->velocity.axes[0] = mb->velocity.axes[1] = 0;
    *flag = 0;
  }
}


void shootBullet(Layer *b, MovLayer *mb) {
  
  Layer *temp;
  for(temp = &layer0; temp; temp = temp->next) {
    if(temp->next == b) {
      temp->next = b->next;
	  break;
    }
  }
  
  MovLayer *temp2;
  for(temp2 = &ml0; temp2; temp2 = temp2->next) {
    if(temp2->next == mb) {
      temp2->next = mb->next;
      break;
    }
  }
  
  
   Vec2 tempPos = layer0.pos;
   tempPos.axes[1] -= 13;
   b->pos = tempPos;
   b->posNext = tempPos;
   b->color = COLOR_BLACK;

   if(layer0.next == &fieldLayer) {
     b->next = &fieldLayer;
     layer0.next = b;
   }
   else {
     b->next = layer0.next;
     layer0.next = b;
   }

   if(ml0.next) {
     mb->next = ml0.next;
     ml0.next = mb;
   }
   else {
     mb->next = 0;
     ml0.next = mb;
   }
   mb->velocity.axes[1] = -8;
}

void killEnemy(Layer *enemy, MovLayer *me, int *flag) {
  if(*flag && collision(enemy , &botFieldLayer)) {
    enemy->color = COLOR_BEIGE;
    
    me->velocity.axes[0] = me->velocity.axes[1] = 0;
    *flag = 0;
  }

  if(*flag && (bulletFlag1 || bulletFlag2) &&(collision(enemy, &bullet1) || collision(enemy , &bullet2))) {
    enemy->color = COLOR_BEIGE;
    P1OUT = 0;
    me->velocity.axes[0] = me->velocity.axes[1] = 0;
    *flag = 0;

    score += 1;

    buzzer_set_period(2000 - score*100);
  }
}

int killPlayer() {
  if(enemy1Flag && collision(&layer0, &enemyLayer1)) {
    return 1;
  }
  else
    return 0;
}

int offset = 0;

void spawnEnemy(Layer *enemy, MovLayer *me) {
  
  Layer *temp;
  for(temp = &layer0; temp; temp = temp->next) {
    if(temp->next == enemy) {
      temp->next = enemy->next;
      break;
    }
  }
  
  MovLayer *temp2;
  for(temp2 = &ml0; temp2; temp2 = temp2->next) {
    if(temp2->next == me) {
      temp2->next = me->next;
      break;
    }
  }


  int offset;
  int x;
  int y;
  offset += score * 5;
  x = 1 + score;
  y = 1 + score;
  if(offset % 2)
    x *= -1;
  

  switch(score) {
  case 0:
    enemy->color = COLOR_RED;
    break;
  case 1:
    enemy->color = COLOR_ORANGE_RED;
    break;
  case 2:
    enemy->color = COLOR_DARK_ORANGE;
    break;
  case 3:
    enemy->color = COLOR_ORANGE;
    break;
  case 4:
    enemy->color = COLOR_YELLOW;
    break;
  case 5:
    enemy->color = COLOR_GREEN_YELLOW;
    break;
  case 6:
    enemy->color = COLOR_GREEN;
    break;
  case 7:
    enemy->color = COLOR_FOREST_GREEN;
    break;
  default:
    enemy->color = COLOR_BLUE;
  }
  
  Vec2 tempPos = {screenWidth/2 + offset, 20};
  enemy->pos = tempPos;
  enemy->posNext = tempPos;
  
  if(layer0.next == &fieldLayer) {
    enemy->next = &fieldLayer;
    layer0.next = enemy;
  }
  else {
    enemy->next = layer0.next;
    layer0.next = enemy;
  }
  
  if(ml0.next) {
    me->next = ml0.next;
    ml0.next = me;
  }
  else {
    me->next = 0;
    ml0.next = me;
  }
  
  me->velocity.axes[0] = x;
  me->velocity.axes[1] = y;
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  static short enemy1Count = 0;
  count++;
  enemy1Count++;
  if (count == 20 && !endGame) {
    buzzer_set_period(0);
    ml0.velocity.axes[0] = ml0.velocity.axes[1] = 0;
    if (S1DOWN) {
      ml0.velocity.axes[0] = -4;
    }
    if(!bulletFlag1 && S2DOWN) {
      shootBullet(&bullet1, &mlBullet1);
      buzzer_set_period(800);
      bulletFlag1++;
    }
    if (!bulletFlag2 &&  S3DOWN) {
      shootBullet(&bullet2, &mlBullet2);
      buzzer_set_period(850);
      
      bulletFlag2++;
    }
    if(S4DOWN) {
      ml0.velocity.axes[0] = 4;
    }
    
    mlAdvance(&ml0, &fieldFence);
    redrawScreen = 1;
    count = 0;

    if(score == 5)
      mlEnemy1.velocity.axes[0] *= -1;
  }

  if(enemy1Count == 500) {
    if(!enemy1Flag) {
      offset = count;
      if(count % 2)
	offset *= -1;
      spawnEnemy(&enemyLayer1, &mlEnemy1);
      enemy1Flag = 1;
    }
    enemy1Count = 0;
  }

  if(endGame && !loss & (count % 100)) {
    buzzer_set_period(count + 2000);
  }
  if(endGame && loss & (count % 100)) {
    buzzer_set_period(2000 - count);
  }
  if(endGame && count == 1000)
    count = 0;
  
  // P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
