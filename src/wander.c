/* stochastic wandering value on [-1,1] with weighted chance adjusted to make it always
 * more likely to 'gravitate' back towards 0
 * randomly moves up or down every time a bang is received; also takes a velocity which
 * determines the maximum change per step
 * evie/Ulrike, 2024
 */

#include "m_pd.h"
#include "stdlib.h"
#include "math.h"

static t_class * wander_class;

typedef struct _wander {
	t_object x_obj;
	t_float current_val;	//current value
	t_float exponent;		//optional exponential curve for weighted chance (defaults to 1)
	t_float velocity;		//current velocity (max. change per step)
} t_wander;

void wander_bang(t_wander *x) {
	//creating the split point to weight the chance of going up or down relative to
	//the current value, optionally scaled exponentially either side of 0
	float threshold = ((copysignf(pow(x->current_val, x->exponent), x->current_val) /
		2.0) + 0.5) * 10000.0;
	int random_num = rand();
	int direction_determinant = random_num % 10000;
	//re-using the same generated number to generate a velocity between 0.0 and velocity[i]
	int velocity_determinant = random_num % 1000000;
	float current_velocity = (velocity_determinant / 1000000.0) * x->velocity;
	if(direction_determinant < threshold) {
		if(x->current_val - current_velocity < -1.0)
		{
			x->current_val = x->current_val + current_velocity;
		}
		else {
			x->current_val = x->current_val - current_velocity;
		}
	}
	else if(direction_determinant > threshold) {
		if(x->current_val + current_velocity > 1.0)
		{
			x->current_val = x->current_val - current_velocity;
		}
		else {
			x->current_val = x->current_val + current_velocity;
		}
	}
	outlet_float(x->x_obj.ob_outlet, x->current_val);
}

void wander_setvelocity(t_wander *x, t_floatarg velocity) {
	if(velocity >= 0.0) {
		x->velocity = velocity;
	}
}

//constructor called when a new instance is made
void *wander_new(t_symbol *s, int argc, t_atom *argv) {
	t_wander *x = (t_wander *) pd_new(wander_class);

	//if a construction argument was passed, set the exponentation value, else set it 
	//  to a default of 1.0
	if(argc == 1) {
		x->exponent = atom_getfloat(argv);
	}
	else if(argc == 2) {
		x->exponent = atom_getfloat(argv);
		x->velocity = atom_getfloat(argv+1);
	}
	else {
		x->exponent = 1.0;
		x->velocity = 0.0;
	}

	//initializing internals
	x->current_val = 0.0;

	//creating a float outlet
	outlet_new(&x->x_obj, &s_float);
	//creating a new inlet for setting velocity
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("velocity"));
	return (void *) x;
}

//called when pure data is loading libraries during startup
void wander_setup(void) {
	wander_class = class_new(gensym("wander"),
		(t_newmethod)wander_new, 0,
        sizeof(t_wander),
        CLASS_DEFAULT, A_GIMME, 0);

	//binding handler functions to their inputs
	class_addbang(wander_class, wander_bang);
	class_addmethod(wander_class, (t_method)wander_setvelocity, gensym("velocity"),
		A_DEFFLOAT, 0);
}