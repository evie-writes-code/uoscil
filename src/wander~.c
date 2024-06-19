/* stochastic wandering value on [-1,1] with weighted chance adjusted to make it always
 * more likely to 'gravitate' back towards 0
 * randomly moves up or down each sample, takes a velocity which determines the maximum
 * change per sample
 * evie/Ulrike, 2024
 */

#include "m_pd.h"
#include "stdlib.h"
#include "math.h"

static t_class * wander_tilde_class;

typedef struct _wander_tilde {
	t_object x_obj;
	t_float f;		// backup float for storing float type input
					// in case the audio inlet receives float type
					// input by mistake
	t_float current_val;	//current value
	t_float exponent;		//optional exponential curve for weighted chance (defaults to 1)

	//creating 'extra' inlets and outlets beyond the expected single audio inlet
	t_outlet *signal_out;
} t_wander_tilde;

//the workhorse dsp method where operations on the incoming vectors actually happen
t_int *wander_tilde_perform(t_int *w) {
	//for some reason, w[] is indexed from 1, not 0.

	//fetching things as they were defined in the dsp_add function call
	//the class dataspace
	t_wander_tilde *x 		=	(t_wander_tilde *)	(w[1]);
	//the next two signal vectors, in and out
	t_sample *velocity			=	(t_sample *)			(w[2]);
	t_sample *signalout			=	(t_sample *)			(w[3]);
	//the vector size
	int n						=	(int)					(w[4]);

	//looping through the whole vector to do an operation on each sample
	for(int i = 0; i < n; i++) {
		//creating the split point to weight the chance of going up or down relative to
		//the current value, optionally scaled exponentially either side of 0
		float threshold = ((copysignf(pow(x->current_val, x->exponent), x->current_val) /
			2.0) + 0.5) * 10000.0;
		int random_num = rand();
		int direction_determinant = random_num % 10000;
		//re-using the same generated number to generate a velocity between 0.0 and velocity[i]
		int velocity_determinant = random_num % 1000000;
		float current_velocity = (velocity_determinant / 1000000.0) * velocity[i];
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
		signalout[i] = x->current_val;
	}

	//returning the pointer we were passed,
	//+ number of arguments we defined + 1
	return (w+5);
}

//handler for dsp input on the leftmost inlet (the default architecture when
//dealing with a single dsp inlet)
void wander_tilde_dsp(t_wander_tilde *x, t_signal **sp) {
	//adding the 'perform-routine' to the DSP-tree.
	//after the perform function, number of following arguments, and the
	//class dataspace are passed, the order of vector pointers in the signal
	//array is determined here as follows:
		//1: incoming vector, 2: outgoing vector, and 3: vector size
	dsp_add(wander_tilde_perform, 4, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

//destructor called when an instance is destroyed
void wander_tilde_free(t_wander_tilde * x) {
	outlet_free(x->signal_out);
}

//constructor called when a new instance is made
void *wander_tilde_new(t_symbol *s, int argc, t_atom *argv) {
	t_wander_tilde *x = (t_wander_tilde *) pd_new(wander_tilde_class);

	//if a construction argument was passed, set the exponentation value, else set it 
	//  to a default of 1.0
	if(argc > 0) {
		x->exponent = atom_getfloat(argv);
	}
	else {
		x->exponent = 1.0;
	}

	//creating an audio outlet
	x->signal_out = outlet_new(&x->x_obj, &s_signal);

	//initializing internals
	x->current_val = 0.0;

	return (void *) x;
}

//called when pure data is loading libraries during startup
void wander_tilde_setup(void) {
	wander_tilde_class = class_new(gensym("wander~"),
		(t_newmethod)wander_tilde_new,
        (t_method)wander_tilde_free,
        sizeof(t_wander_tilde),
        CLASS_DEFAULT, A_GIMME, 0);

	//binding the dsp handler function to the mandatory symbol "dsp"
	class_addmethod(wander_tilde_class,
		(t_method)wander_tilde_dsp,
		gensym("dsp"),
		A_CANT, 0);
	//do class stuff so we can have a signal inlet, I think
	CLASS_MAINSIGNALIN(wander_tilde_class,
		t_wander_tilde, f);
}