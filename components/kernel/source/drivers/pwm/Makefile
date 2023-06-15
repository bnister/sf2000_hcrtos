THIRDPARTYNUTTXMTD=../../third_party/nuttx/drivers/pwm

_pwm-y := pwm.o

pwm-y := $(addprefix $(THIRDPARTYNUTTXMTD)/,$(_pwm-y))
pwm-y += hc-pwm.o

obj-y += pwm.o
