#include	<cdt/dthdr.h>
#include	<stddef.h>

/* 	Make a new dictionary
**
**	Written by Kiem-Phong Vo (5/25/96)
*/

Dt_t* dtopen(Dtdisc_t* disc, Dtmethod_t* meth)
{
	Dt_t*		dt;
	Dtdata_t*	data;

	if(!disc || !meth)
		return NULL;

	/* allocate space for dictionary */
	if(!(dt = malloc(sizeof(Dt_t))))
		return NULL;

	/* initialize all absolutely private data */
	dt->searchf = NULL;
	dt->meth = NULL;
	dt->disc = NULL;
	dtdisc(dt, disc);
	dt->type = DT_MALLOC;
	dt->nview = 0;
	dt->view = dt->walk = NULL;
	dt->user = NULL;

	/* allocate sharable data */
	if (!(data = dt->memoryf(dt, NULL, sizeof(Dtdata_t), disc)))
	{
		free(dt);
		return NULL;
	}

	data->type = meth->type;
	data->here = NULL;
	data->htab = NULL;
	data->ntab = data->size = data->loop = 0;

	dt->data = data;
	dt->searchf = meth->searchf;
	dt->meth = meth;

	return dt;
}
