/*
 * File:	game.c
 * Author:	Michael Nguyen
 *              Tri Hoang
 *              Elias Wang
 * Target PIC:	PIC32MX250F128B
 */

// graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"

//extra libraries and defines
#include <math.h>
#include <stdint.h>

// Threading Library
// config.h sets SYSCLK 40 MHz
#define SYS_FREQ 40000000
#include "pt_cornell_TFT.h"

// Define a struct for balls
typedef struct Ball Ball;
struct Ball {
    int xpos;
    int ypos;
    int xvel;
    int yvel;
    int color;
    uint8_t delay;
    Ball *b;
} ball;

static struct pt pt_calculate, pt_refresh;
char buffer[60];

//Points to the head of the linked list of balls
struct Ball *head;

//Drag divisor to simulate friction between the ball and table
int drag = 1000;
//Scale is used to convert float point notation to fixed point
int scale = 1000;
//Define Ball radius and time between collisions
uint8_t ballradius = 4;
uint8_t delay_master = 10;

//Parameters for the paddle
uint8_t paddle_length = 30;
uint8_t paddle_ypos = 100;
uint8_t paddle_xpos = 6;

//keeps track of the frames per second
uint8_t frames = 0;

//these are used to control when balls are made and how many are made
uint8_t numBalls = 0;
uint8_t maxBalls = 50;
uint8_t ballgen = 0;

int score = 0;
int timeElapsed ;
//============== Create a ball ================//
struct Ball *Ball_create(int xp, int yp, int xv, int yv, int color,  uint8_t d, Ball *bb) {
    
    struct Ball *ba = malloc(sizeof(struct Ball));
    if(ba == NULL)
        return NULL;
    
    ba->xpos = xp*scale;
    ba->ypos = yp*scale;
    ba->xvel = xv*scale;
    ba->yvel = yv*scale;
    ba->color = color;
    ba->delay = d;
    ba->b = bb;
    
    return ba;
}

//======================= Refresh ========================= //
//Does Ball calculations and Draws the necessary elements on the screen 
static PT_THREAD (protothread_refresh(struct pt *pt))
{
    PT_BEGIN(pt);
    PT_YIELD_TIME_msec(100);
    //waits for the scoreboard to be set up
    while(1) {
        PT_YIELD_TIME_msec(10);
        
        //Generates a new ball at a given interval
        if(ballgen >= 40) {
            int troll1 = -(rand() % 2)-1;
            int troll2 = (rand() % 6) - 3;
            struct Ball *temp = Ball_create(320,120,troll1,troll2,numBalls*1000,0,NULL);
            temp->b = head;
            head = temp;
            ballgen = 0;
            numBalls++;
        }
        else
            ballgen ++;
        
        //collision calculations
        struct Ball *ti = head;
        struct Ball *tj = NULL;
        if(ti != NULL)
            tj = ti->b;
        while(ti !=NULL){
            //Calculates the collisions between every ball
            while(tj != NULL) {
                int rij_x = ti->xpos - tj->xpos;
                int rij_y = ti->ypos - tj->ypos;
                int mag_rij = pow(rij_x,2) + pow(rij_y,2);
                int temp = pow(2*(ballradius*scale),2);
                //Checks if ti and tj are not pointing to the same ball,
                //If they close enough for a collision and there is no collision
                //delay.
                if( ti->delay + tj->delay <= 0 && mag_rij < temp) {
                    int vij_x = ti->xvel - tj->xvel;
                    int vij_y = ti->yvel - tj->yvel;
                    long deltaVi_x = -1*rij_x * (((rij_x * vij_x)+ (rij_y*vij_y)))/mag_rij;
                    long deltaVi_y = -1*rij_y * (((rij_x * vij_x)+ (rij_y*vij_y)))/mag_rij;
                    
                    //Updates the velocity
                    ti->xvel = ti->xvel + deltaVi_x;
                    ti->yvel = ti->yvel + deltaVi_y;
                    
                    tj->xvel = tj->xvel - deltaVi_x;
                    tj->yvel = tj->yvel - deltaVi_y;
                    
                    ti->delay = delay_master;
                    tj->delay = delay_master;
                }
                tj = tj->b;
            }
            
            //checks for wall collisions
            if(ti->xpos >= 320*scale || ti->xpos <= 0) 
                ti->xvel = -1*ti->xvel;
            if(ti->ypos >= 240*scale || ti->ypos <= 35*scale)
                ti->yvel = -1*ti->yvel;
            
            //calculates the drag
            if(ti->xvel > 0)
                ti->xvel = ti->xvel - ti->xvel/drag;
            else
                ti->xvel = ti->xvel + ti->xvel/drag;
            if(ti->yvel > 0)
                ti->yvel = ti->yvel - ti->yvel/drag;
            else
                ti->yvel = ti->yvel - ti->yvel/drag;
            
            // Check for paddle Collisions
            //NOTE: Need to calculate "paddle friction"
            if(abs(ti->xpos/scale - paddle_xpos) < ballradius)
                if(ti->ypos/scale > paddle_ypos && ti->ypos/scale < paddle_ypos + paddle_length)
                    ti->xvel = -1*ti->xvel;
            
            //Decrement the collide delay
            if(ti->delay > 0)
                ti->delay = ti->delay -1;
            

            //iterates through the next set
            ti = ti->b;
            if(ti != NULL)
                tj = ti->b;
            
            //removes the last element if the limit is reached
            if(numBalls > maxBalls && tj->b == NULL) { 
                tft_fillCircle(tj->xpos/scale,tj->ypos/scale,ballradius,ILI9340_BLACK); //erases from the screen
                ti->b = NULL;
                numBalls --;
            }
                
        }
        // Calculates position of the paddle and draw
        //TODO: Calculate paddle position
        tft_drawLine(paddle_xpos,paddle_ypos, paddle_xpos, paddle_ypos + paddle_length, ILI9340_WHITE);
        
        // Now it calculates the new position
	    ti = head;
        tj = head;
        while(ti != NULL){
            //"Clears" the image of the last ball
            tft_fillCircle(ti->xpos/scale,ti->ypos/scale,ballradius,ILI9340_BLACK);
            
            //Updates the new position of the ball
            ti->xpos = ti->xpos + ti->xvel;
            ti->ypos = ti->ypos + ti->yvel;
            
            //ensures the positions are within bounds
            //If the pos is less than 0 then we remove it
            if(ti->xpos > 0) {
                if(ti->xpos > 320*scale)
                    ti->xpos = 320*scale;

                if(ti->ypos > 240*scale)
                    ti->ypos = 240*scale;
                else if(ti->ypos < 35*scale)
                    ti->ypos = 35*scale;

                if(ti->delay != 0)
                     tft_fillCircle(ti->xpos/scale, ti->ypos/scale, ballradius, ILI9340_WHITE);
                else
                    tft_fillCircle(ti->xpos/scale, ti->ypos/scale, ballradius, ti->color);
            }
            else { //REMOVES THE BALL IF IT CROSSES THE BOUNDARY
                if(ti == head)
                    head = head->b;
                else
                    tj->b = ti->b;
                score--;
                numBalls--;
            }
            tj = ti;
            ti = ti->b;
        }
        frames ++;
   }
    PT_END(pt);
} // blink

//==================== Calculate ===================== //
static PT_THREAD (protothread_calculate (struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 1 second
        
        
        int minutes = timeElapsed/60;
        int seconds = timeElapsed%60;
        // draw sys_time
        tft_fillRoundRect(0,10, 320, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
        sprintf(buffer,"%02d:%02d  FPS:%d  Score:%d", minutes,seconds, frames, score);
        tft_writeString(buffer);
        frames = 0;
        PT_YIELD_TIME_msec(1000);
        timeElapsed++ ;
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
}
//===================== Main ======================= //
void main(void) {
    SYSTEMConfigPerformance(PBCLK);

    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;

    PT_setup();
    
    head = NULL;
    
        
    // initialize the threads
    PT_INIT(&pt_calculate);
    PT_INIT(&pt_refresh);
    // initialize the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    
    INTEnableSystemMultiVectoredInt();

    tft_setRotation(1); //240x320 horizontal display
  
    //round-robin scheduler for threads
    while(1) {
        PT_SCHEDULE(protothread_calculate(&pt_calculate));
        PT_SCHEDULE(protothread_refresh(&pt_refresh));
    }
    
} //main

