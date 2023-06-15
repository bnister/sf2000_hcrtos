#ifndef __PROJECTOR_VMOTOR_R__
#define __PROJECTOR_VMOTOR_R__
#ifdef PROJECTOR_VMOTOR_SUPPORT
#define VMOTOR_RET_SUCCESS 0
#define VMOTOR_RET_ERROR 1

typedef enum VMOTOR_STEP_DIRECTION_E{
    BMOTOR_STEP_FORWARD,
    BMOTOR_STEP_BACKWARD,
}vmotor_direction_e;

int vMotor_init(void);
int vMotor_deinit(void);
int vMotor_set_direction(vmotor_direction_e val);
int vMotor_set_step_time(unsigned int val);
int vMotor_set_step_count(int val);
int vMotor_get_step_count(void);
int vMotor_Roll(void);
int vMotor_Roll_test(void);
int vMotor_Roll_cocus(void);
#endif
#endif
