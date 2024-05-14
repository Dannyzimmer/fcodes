#include	<cdt/dthdr.h>

/*	Walk a dictionary and all dictionaries viewed through it.
**	userf:	user function
**
**	Written by Kiem-Phong Vo (5/25/96)
*/

int dtwalk(Dt_t *dt, int (*userf)(void*, void*), void *data) {
	void	*obj, *next;
	Dt_t*	walk;
	int		rv;

	for(obj = dtfirst(dt); obj; )
	{	if(!(walk = dt->walk) )
			walk = dt;
		next = dtnext(dt,obj);
		if ((rv = userf(obj, data)) < 0)
			return rv;
		obj = next;
	}
	return 0;
}
