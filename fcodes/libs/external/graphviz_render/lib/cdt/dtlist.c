#include	<cdt/dthdr.h>
#include	<stddef.h>

/*	List, Deque, Stack, Queue.
**
**	Written by Kiem-Phong Vo (05/25/96)
*/

static void* dtlist(Dt_t* dt, void* obj, int type)
{
	int		lk, sz, ky;
	Dtcompar_f	cmpf;
	Dtdisc_t*	disc;
	Dtlink_t	*r, *t;
	void	*key, *k;

	UNFLATTEN(dt);
	disc = dt->disc; _DTDSC(disc,ky,sz,lk,cmpf);
	dt->type &= ~DT_FOUND;

	if(!obj)
	{	if(type&(DT_LAST|DT_FIRST) )
		{	if((r = dt->data->head) )
			{	if(type&DT_LAST)
					r = r->left;
				dt->data->here = r;
			}
			return r ? _DTOBJ(r,lk) : NULL;
		}
		else if(type&(DT_DELETE|DT_DETACH))
		{	if(!(r = dt->data->head))
				return NULL;
			else	goto dt_delete;
		}
		else if(type&DT_CLEAR)
		{	if(disc->freef || disc->link < 0)
			{	for(r = dt->data->head; r; r = t)
				{	t = r->right;
					if(disc->freef)
						disc->freef(_DTOBJ(r, lk), disc);
					if(disc->link < 0)
						dt->memoryf(dt, r, 0, disc);
				}
			}
			dt->data->head = dt->data->here = NULL;
			dt->data->size = 0;
			return NULL;
		}
		else	return NULL;
	}

	if(type&DT_INSERT)
	{	if (disc->makef && (type&DT_INSERT) && !(obj = disc->makef(obj, disc)))
			return NULL;
		if(lk >= 0)
			r = _DTLNK(obj,lk);
		else
		{	r = dt->memoryf(dt, NULL, sizeof(Dthold_t), disc);
			if(r)
				((Dthold_t*)r)->obj = obj;
			else
			{	if(disc->makef && disc->freef && (type&DT_INSERT))
					disc->freef(obj, disc);
				return NULL;
			}
		}

		/* if(dt->data->type&DT_QUEUE) */
		if((t = dt->data->head) )
		{	t->left->right = r;
			r->left = t->left;
			t->left = r;
		}
		else
		{	dt->data->head = r;
			r->left = r;
		}
		r->right = NULL;

		if(dt->data->size >= 0)
			dt->data->size += 1;

		dt->data->here = r;
		return _DTOBJ(r,lk);
	}

	if((type&DT_MATCH) || !(r = dt->data->here) || _DTOBJ(r,lk) != obj)
	{	key = (type&DT_MATCH) ? obj : _DTKEY(obj,ky,sz);
		for(r = dt->data->head; r; r = r->right)
		{	k = _DTOBJ(r,lk); k = _DTKEY(k,ky,sz);
			if(_DTCMP(dt,key,k,disc,cmpf,sz) == 0)
				break;
		}
	}

	if(!r)
		return NULL;
	dt->type |= DT_FOUND;

	if(type&(DT_DELETE|DT_DETACH))
	{ dt_delete:
		if(r->right)
			r->right->left = r->left;
		if(r == (t = dt->data->head) )
		{	dt->data->head = r->right;
			if(dt->data->head)
				dt->data->head->left = t->left;
		}
		else
		{	r->left->right = r->right;
			if(r == t->left)
				t->left = r->left;
		}

		dt->data->here = r == dt->data->here ? r->right : NULL;
		dt->data->size -= 1;

		obj = _DTOBJ(r,lk);
		if(disc->freef && (type&DT_DELETE))
			disc->freef(obj, disc);
		if(disc->link < 0)
			dt->memoryf(dt, r, 0, disc);
		return obj;
	}
	else if(type&DT_NEXT)
		r = r->right;
	else if(type&DT_PREV)
		r = r == dt->data->head ? NULL : r->left;

	dt->data->here = r;
	return r ? _DTOBJ(r,lk) : NULL;
}

Dtmethod_t _Dtqueue = { dtlist, DT_QUEUE };

Dtmethod_t* Dtqueue = &_Dtqueue;
