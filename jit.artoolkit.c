/* 
	Copyright 2001-2011 - Cycling '74
	Joshua Kit Clayton jkc@cycling74.com	
*/

#include "jit.common.h"
#include "ext_systhread.h"

#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include "AR/config.h"
#include "AR/video.h"
#include "AR/param.h"			// arParamDisp()
#include "AR/ar.h"
#include "AR/gsub_lite.h"

typedef struct _jit_artoolkit 
{
	t_object	ob;
	long		planecount;
	//t_jit_artoolkit_vecdata vd;
	t_systhread_mutex mutex;
} t_jit_artoolkit;

t_jit_err jit_artoolkit_init(void); 
t_jit_err jit_artoolkit_matrix_calc(t_jit_artoolkit *x, void *inputs, void *outputs);

void *_jit_artoolkit_class;

t_jit_artoolkit *jit_artoolkit_new(void);
void jit_artoolkit_free(t_jit_artoolkit *x);
void jit_artoolkit_calculate_ndim(t_jit_artoolkit *x, long dimcount, long *dim, long planecount, 
	t_jit_matrix_info *in1_minfo, char *bip1);

t_jit_err jit_artoolkit_init(void) 
{
	long attrflags=0;
	t_jit_object *attr,*mop;
	
	_jit_artoolkit_class = jit_class_new("jit_artoolkit",(method)jit_artoolkit_new,(method)jit_artoolkit_free,
		sizeof(t_jit_artoolkit),0L);

	//add mop
	mop = jit_object_new(_jit_sym_jit_mop,1,0);
	jit_class_addadornment(_jit_artoolkit_class,mop);
	//add methods
	jit_class_addmethod(_jit_artoolkit_class, (method)jit_artoolkit_matrix_calc, 		"matrix_calc", 		A_CANT, 0L);
	//add attributes	
	attrflags = JIT_ATTR_SET_OPAQUE_USER | JIT_ATTR_GET_DEFER_LOW;

	CLASS_STICKY_CATEGORY(_jit_artoolkit_class,0,"Value");

    CLASS_STICKY_CATEGORY_CLEAR(_jit_artoolkit_class);
	
	jit_class_register(_jit_artoolkit_class);

	return JIT_ERR_NONE;
}

t_jit_err jit_artoolkit_matrix_calc(t_jit_artoolkit *x, void *inputs, void *outputs)
{
	t_jit_err err=JIT_ERR_NONE;
	long in_savelock;
	t_jit_matrix_info in_minfo;
	char *in_bp;
	long i,dimcount,dim[JIT_MATRIX_MAX_DIMCOUNT];
	void *in_matrix;
	
	in_matrix 	= jit_object_method(inputs,_jit_sym_getindex,0);

	if (x&&in_matrix) {
		
		in_savelock = (long) jit_object_method(in_matrix,_jit_sym_lock,1);
		jit_object_method(in_matrix,_jit_sym_getinfo,&in_minfo);
		jit_object_method(in_matrix,_jit_sym_getdata,&in_bp);
		
		if (!in_bp) { err=JIT_ERR_INVALID_INPUT; 	x->planecount = 0; goto out;}
		
		//get dimensions/planecount 
		//3m does this a bit different than how it happens for most other objects
		dimcount    = in_minfo.dimcount;
		for (i=0;i<dimcount;i++) {
			dim[i] = in_minfo.dim[i];
		}		
		
		//calculate
		//jit_artoolkit_precalc(&x->vd, &in_minfo, in_bp);
		jit_parallel_ndim_simplecalc1((method)jit_artoolkit_calculate_ndim, 
			x, dimcount, dim, in_minfo.planecount, &in_minfo, in_bp,
			0 /* flags1 */);
		//jit_artoolkit_mean(&x->vd, &in_minfo);
		//jit_artoolkit_postcalc(x, &x->vd, &in_minfo);
		
	} else {
		return JIT_ERR_INVALID_PTR;
	}
	
out:
	jit_object_method(in_matrix,_jit_sym_lock,in_savelock);
	return err;
}


//recursive function to handle higher dimension matrices, by processing 2D sections at a time 
void jit_artoolkit_calculate_ndim(t_jit_artoolkit *x, long dimcount, long *dim, long planecount, 
	t_jit_matrix_info *in1_minfo, char *bip1)
{

}

t_jit_artoolkit *jit_artoolkit_new(void)
{
	t_jit_artoolkit *x;
		
	if (x=(t_jit_artoolkit *)jit_object_alloc(_jit_artoolkit_class)) {
		x->planecount = 0;
		systhread_mutex_new(&x->mutex,0);
	} else {
		x = NULL;
	}	
	return x;
}

void jit_artoolkit_free(t_jit_artoolkit *x)
{
	systhread_mutex_free(x->mutex);
}
