//
// Created by daniel on 02.04.20.
//


#define SIMULATION 1

#if SIMULATION
#include "../API/webots/webotsAPI.h"
#else
#include "../API/epuck/epuckAPI.h"
#endif

void explorer(), lover(), stop();

//typedef int bool;
#define STOP 0
#define LOVER 1
#define EXPLORER 2
//#define MSG_LENGTH 5
#define NB_ROBOT 4    //MUST match number of robots on the field. Robots MUST be named starting from 1

// control board, i.e varaibles that control the transitions
int robot[NB_ROBOT];
int robot_count=0;
int init = 1;
int EQ = 0;
int ack = 0;
int go = 0;
int sent = 0;


int STATE = LOVER;
int SAW_OBSTACLE = 0;
int HAS_STOPPED = 0;
int a = 1;
int b = 2;
int c = 3;
int d = 4;
double e = 10.;

char rcv[MSG_LENGTH];
char rcv2[MSG_LENGTH];
char tmp[MSG_LENGTH];
char tmp2[MSG_LENGTH];


void robot_setup() {
    init_robot();
    init_sensors();
    calibrate_prox();
    init_communication();
    // initialize robot[] to 0
    for (int r = 0; r < NB_ROBOT; r++) {
      robot[r] = 0;
    }
}

void robot_loop() {
    int id = get_robot_ID();
    short int prox_values[8];
    int init = 1;
    int EQ = 0;
    int ack = 0;
    int go = 0;
    int sent = 0;

    while (robot_go_on()) {
      //controlles that robot_count doesn't go over the array size
      if(robot_count >= NB_ROBOT){
        robot_count = 0;
      }
      receive_msg(rcv);
      int rcv_id = rcv[0]- 48;
      if (rcv_id <= NB_ROBOT){
        robot[robot_count] = rcv_id;
        robot_count++;
      } else if (strcmp(rcv,"go")==0){
        go=1;
        init = 0;
      }else if (strcmp(rcv,"EQ")==0){
        EQ = 1;
        robot_count++;
      }else if (sent == 1) {
        sent = 0;
        robot[robot_count] = id;
        robot_count++;
      }

        get_prox_calibrated(prox_values);
        double prox_right = (a * prox_values[0] + b * prox_values[1] + c * prox_values[2] + d * prox_values[3]) / e;
        double prox_left = (a * prox_values[7] + b * prox_values[6] + c * prox_values[5] + d * prox_values[4]) / e;
        double ds_right = (NORM_SPEED * prox_right) / MAX_PROX;
        double ds_left = (NORM_SPEED * prox_left) / MAX_PROX;
        double speed_right = bounded_speed(NORM_SPEED - ds_right);
        double speed_left = bounded_speed(NORM_SPEED - ds_left);

        if (STATE == EXPLORER) {
            if (speed_right < NORM_SPEED*0.25 || speed_left < NORM_SPEED*0.25) {
                SAW_OBSTACLE = 1;
                explorer(speed_left, speed_right);
            } else if ((speed_right > NORM_SPEED*0.5 && speed_left > NORM_SPEED*0.5) && SAW_OBSTACLE) {
                SAW_OBSTACLE = 0;
                lover(speed_left, speed_right);
            } else {
                explorer(speed_left, speed_right);
            }

        } else if (STATE == LOVER) {
            if ((prox_right > 80.0 || prox_left > 80.0) && init == 1) {
              for(int i = 0; i< NB_ROBOT; i++){
                if (robot[i] > 0){
                  ack++;
                }
              }
              if (ack == NB_ROBOT - 1) {
                stop();
                sprintf(tmp, "%d", id);
                sprintf(tmp2, "%s", "go");
                send_msg(tmp);
                send_msg(tmp2);
                sent = 1;
                init = 0;
              }else {
              sprintf(tmp, "%d", id);
              send_msg(tmp);
              sent = 1;
              init = 0;
              stop();
              }
            } else if (speed_right < 0.0 && speed_left < 0.0) {
              sprintf(tmp2, "%s", "EQ");
              send_msg(tmp2);
              EQ = 0;
              stop();
              robot_count++;
            } else {
                lover(speed_left, speed_right);
            }
        } else if(STATE == STOP){
          if(go == 1 && robot[0] == id){
            lover(speed_left, speed_right);
            go = 0;
          }
          else if(EQ == 1){
            if (robot_count < (NB_ROBOT - 1)) {
              if (robot[robot_count + 1] == id) {
                explorer(speed_left, speed_right);
              }
            } else if(robot_count == (NB_ROBOT - 1) && robot[0] == id){
              explorer(speed_left, speed_right);
            }
          }
        }
      }
    cleanup_robot();
}

int main(int argc, char **argv) {
#if SIMULATION
#else
    ip = argv[1];
#endif

    robot_setup();
    robot_loop();
}

void explorer(double speed_left, double speed_right) {
    STATE = EXPLORER;
    a = 1;
    b = 2;
    c = 2;
    d = 1;
    e = a + b + c + d;
    for(int i = 0; i < 4; i++){
      disable_rgbled(i);
      enable_rgbled(i, 0x0000ff);
    }

    set_speed(speed_right, speed_left);
}

void lover(double speed_left, double speed_right) {
    STATE = LOVER;
    a = 5;
    b = 3;
    c = 2;
    d = 1;
    e = a + b + c + d;
    for(int i = 0; i < 4; i++){
      disable_rgbled(i);
      enable_rgbled(i, 0xff0000);
    }
    set_speed(speed_left, speed_right);

}

void stop(){
  STATE = STOP;
  for(int i = 0; i < 4; i++){
    disable_rgbled(i);
    enable_rgbled(i, 0x00ff00);
  }
  set_speed(0, 0);
}
